#include "unity.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "bluetooth.h" // Twój plik nagłówkowy

// Flaga zapobiegająca wielokrotnej inicjalizacji stosu NimBLE w trakcie jednej sesji testowej
static bool is_bt_initialized = false;

// --- Konfiguracja środowiska testowego ---

void setUp(void)
{
    // Brak konieczności resetowania - testujemy na ciągłym stanie sprzętu
}

void tearDown(void)
{
    // Po teście sprzęt pozostaje w swoim stanie
}

// --- Przypadki testowe ---

TEST_CASE("Poczatkowy stan IsConnected to false", "[bluetooth][on-target]")
{
    // Zmienna activeConnection powinna domyślnie wynosić 0xFFFF
    TEST_ASSERT_FALSE(IsConnected());
}

TEST_CASE("BtInit inicjalizuje fizycznie NVS i stos NimBLE", "[bluetooth][on-target]")
{
    if (!is_bt_initialized) 
    {
        esp_err_t ret = BtInit();
        // Na fizycznym urządzeniu powinno przejść bez błędu
        TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
        is_bt_initialized = true;
    } 
    else 
    {
        // Jeśli testy są uruchamiane ponownie, ignorujemy podwójną inicjalizację
        TEST_PASS_MESSAGE("Bluetooth zostal juz zainicjalizowany w tej sesji.");
    }
}

TEST_CASE("BtConfigureAdvertising uruchamia zglaszanie (Advertising) na radiu", "[bluetooth][on-target]")
{
    // Upewniamy się, że stos jest gotowy
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    // Wywołanie faktycznej konfiguracji na układzie radiowym
    esp_err_t ret = BtConfigureAdvertising();
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("BtSend zwraca ESP_FAIL, gdy nie ma fizycznego polaczenia", "[bluetooth][on-target]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    uint8_t testData[] = {0x01, 0x02, 0x03};
    
    // Ponieważ żadne urządzenie (np. telefon) nie połączyło się fizycznie z ESP32,
    // activeConnection wynosi 0xFFFF. Prawdziwa funkcja ble_gattc_notify_custom 
    // z NimBLE odrzuci to żądanie, a my oczekujemy, że BtSend prawidłowo zwróci błąd.
    esp_err_t ret = BtSend(testData, sizeof(testData));

    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);
}

TEST_CASE("BtGapEventHandler poprawnie aktualizuje stan polaczenia", "[bluetooth][on-target]")
{
    // Możemy symulować zdarzenia sprzętowe poprzez bezpośrednie wywołanie handlera.
    // Dzięki temu testujemy Twoją logikę bez potrzeby posiadania drugiego urządzenia Bluetooth obok.
    struct ble_gap_event event = {0};
    
    // 1. Symulujemy udane połączenie
    event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.status = 0; // 0 oznacza sukces w API NimBLE
    event.connect.conn_handle = 123; // Fikcyjny uchwyt

    BtGapEventHandler(&event, NULL);
    TEST_ASSERT_TRUE(IsConnected());

    // 2. Symulujemy rozłączenie (sprzątamy po teście)
    // To przywróci activeConnection do 0xFFFF i odpali BtConfigureAdvertising
    event.type = BLE_GAP_EVENT_DISCONNECT;
    BtGapEventHandler(&event, NULL);
    
    TEST_ASSERT_FALSE(IsConnected());
}

TEST_CASE("BtGapEventHandler wymusza restart ogłaszania po błędzie połączenia", "[bluetooth][on-target]")
{
    struct ble_gap_event event = {0};
    
    // Symulujemy nieudane połączenie (status inny niż 0)
    event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.status = 1; // Błąd połączenia

    // To wywołanie powinno zresetować activeConnection i wywołać BtConfigureAdvertising.
    // Ponieważ kod działa na sprzęcie, jeśli BtConfigureAdvertising zwróci błąd, test obleje.
    esp_err_t ret = BtGapEventHandler(&event, NULL);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    TEST_ASSERT_FALSE(IsConnected());
}

// --- Testy obciążeniowe i symulacje skrajnych przypadków (Stress Tests) ---

TEST_CASE("Symulacja: Szybkie zrywanie i wznawianie polaczenia (Flaky Connection)", "[bluetooth][stress]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    struct ble_gap_event connectEvent = {0};
    connectEvent.type = BLE_GAP_EVENT_CONNECT;
    
    struct ble_gap_event disconnectEvent = {0};
    disconnectEvent.type = BLE_GAP_EVENT_DISCONNECT;
    
    // Symulujemy bardzo niestabilne środowisko - 1000 szybkich połączeń i rozłączeń
    for (int i = 0; i < 1000; i++)
    {
        // Urządzenie się łączy
        connectEvent.connect.status = 0;
        connectEvent.connect.conn_handle = i; // Unikalny uchwyt dla każdej iteracji
        BtGapEventHandler(&connectEvent, NULL);
        TEST_ASSERT_TRUE(IsConnected());

        // Urządzenie natychmiast zrywa połączenie
        BtGapEventHandler(&disconnectEvent, NULL);
        TEST_ASSERT_FALSE(IsConnected());
    }
    
    // Upewniamy się, że po tej "burzy" urządzenie jest gotowe na stabilne połączenie
    connectEvent.connect.conn_handle = 9999;
    BtGapEventHandler(&connectEvent, NULL);
    TEST_ASSERT_TRUE(IsConnected());
    
    // Sprzątanie
    BtGapEventHandler(&disconnectEvent, NULL);
}

TEST_CASE("Symulacja: Wiele urzadzen probuje polaczyc sie jednoczesnie", "[bluetooth][stress]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    struct ble_gap_event eventDevA = {0};
    eventDevA.type = BLE_GAP_EVENT_CONNECT;
    
    struct ble_gap_event eventDevB = {0};
    eventDevB.type = BLE_GAP_EVENT_CONNECT;
    
    struct ble_gap_event disconnectEvent = {0};
    disconnectEvent.type = BLE_GAP_EVENT_DISCONNECT;

    // Urządzenie A łączy się pomyślnie
    eventDevA.connect.status = 0;
    eventDevA.connect.conn_handle = 10;
    BtGapEventHandler(&eventDevA, NULL);
    TEST_ASSERT_TRUE(IsConnected());

    // W tym samym czasie Urządzenie B próbuje się połączyć
    eventDevB.connect.status = 0;
    eventDevB.connect.conn_handle = 20;
    BtGapEventHandler(&eventDevB, NULL);
    
    // Aplikacja wciąż widzi, że jest połączona
    TEST_ASSERT_TRUE(IsConnected());

    // Próba wysłania danych (nie zcrashuje systemu, ale poleci do urządzenia B)
    uint8_t dummyData[] = {0xAA};
    BtSend(dummyData, sizeof(dummyData)); 

    // Sprzątamy (rozłączamy oba urządzenia)
    BtGapEventHandler(&disconnectEvent, NULL);
}

TEST_CASE("Symulacja: Powrot polaczenia po wczesniejszym nieudanym polaczeniu", "[bluetooth][stress]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    struct ble_gap_event event = {0};
    event.type = BLE_GAP_EVENT_CONNECT;
    
    // 1. Ktoś próbuje się połączyć, ale proces autoryzacji/parowania zawodzi
    event.connect.status = 5; // Błąd z NimBLE np. timeout
    event.connect.conn_handle = 0;
    BtGapEventHandler(&event, NULL);
    
    // Urządzenie powinno natychmiast wrócić do trybu rozłączonego
    TEST_ASSERT_FALSE(IsConnected());

    // 2. Po chwili to samo urządzenie próbuje ponownie i tym razem się udaje
    event.connect.status = 0; 
    event.connect.conn_handle = 42;
    BtGapEventHandler(&event, NULL);
    
    // Urządzenie powinno działać poprawnie
    TEST_ASSERT_TRUE(IsConnected());
    
    // Rozłączamy, żeby posprzątać
    event.type = BLE_GAP_EVENT_DISCONNECT;
    BtGapEventHandler(&event, NULL);
}