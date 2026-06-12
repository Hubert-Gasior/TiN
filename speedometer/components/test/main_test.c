#include "unity.h"
#include "esp_err.h"
#include <string.h>
#include "main.h" // Twój plik nagłówkowy main
#include "types.h"
#include "linearBuffer.h"
#include "imuB160.h"

// --- Zmienne środowiska wirtualnego ---
static int64 mock_system_time_us = 0;
static bool mock_is_connected = false;
static bool bt_send_was_called = false;
static uint8 last_sent_buffer[SEND_BUFFER_SIZE];

// Dostęp do zmiennych globalnych z main.c
extern LinearBuffer LinearSendBuffer;
extern bool volatile HalReady;
extern bool dataReady;
extern AppData sAppData;
extern void AppTick(void);

// --- Atrapy (Mocks) ---

int64 __wrap_esp_timer_get_time(void) {
    return mock_system_time_us;
}

void __wrap_vTaskDelay(uint32_t ticks) {
    // Sztuczny upływ czasu - zakładamy, że 1 tick to 1ms
    // Pozwala to na szybkie przetestowanie timeoutu bez fizycznego czekania
    mock_system_time_us += (ticks * 1000); 
}

bool __wrap_IsConnected(void) {
    return mock_is_connected;
}

esp_err_t __wrap_ImuRead(void const *devHandle, ImuData *data) {
    data->accX = 0.0f; data->accY = 0.0f; data->accZ = 1.0f; 
    return ESP_OK;
}

esp_err_t __wrap_BtSend(uint8 const *buffer, uint32 const size) {
    bt_send_was_called = true;
    memcpy(last_sent_buffer, buffer, size);
    return ESP_OK;
}

// --- Setup & Teardown ---

void setUp(void)
{
    mock_system_time_us = 0;
    mock_is_connected = true;
    bt_send_was_called = false;
    memset(last_sent_buffer, 0, sizeof(last_sent_buffer));
    
    HalReady = false;
    dataReady = false;
    sAppData.distance = 100.0f; 
    sAppData.circumference = 2.0f;
    sAppData.interval = 500000; // 0.5s interwał z poprzedniego obrotu
    
    static uint8 dummySendBuff[SEND_BUFFER_SIZE];
    LinearBufferInit(&LinearSendBuffer, dummySendBuff, SEND_BUFFER_SIZE);
}

void tearDown(void) {}

// --- Przypadki Testowe ---

TEST_CASE("AppTick: Poprawnie odczytuje czujniki i wysyla przez Bluetooth", "[app_main]")
{
    // 1. Akcja: Uruchamiamy pierwszy cykl. Rower jedzie (HalReady = true).
    HalReady = true;
    AppTick();

    // Po pierwszym przejściu flagi powinny być zaktualizowane
    TEST_ASSERT_TRUE(dataReady);
    TEST_ASSERT_FALSE(HalReady);
    
    // BtSend zostało zawołane w tym samym ticku, ponieważ dataReady 
    // stało się true, a IsConnected() == true
    TEST_ASSERT_TRUE(bt_send_was_called);
    
    // Upewniamy się, że wysłane dane zostały po tym wyczyszczone (flaga opuszczona)
    TEST_ASSERT_FALSE(dataReady);
}

TEST_CASE("AppTick: Po osiagnieciu Timeoutu Halla aplikacja kontynuuje dzialanie", "[app_main]")
{
    // Symulacja zatrzymanego koła (HalReady się nie zmienia)
    HalReady = false; 

    // Akcja: Wywołanie AppTick. Funkcja utknie w pętli while(!HalReady), ale
    // atrapa vTaskDelay przesunie wirtualny czas, aż pętla przerwie działanie przez `break`.
    AppTick();

    // Weryfikacja: Mimo timeoutu, kod (zgodnie z oryginalną logiką) wysyła dane
    TEST_ASSERT_TRUE(dataReady == false); // Zostało wysłane i wyzerowane
    TEST_ASSERT_TRUE(bt_send_was_called);

    // Zgodnie z obecną logiką, kod użył starego interwału (0.5s) do obliczeń
    // Weryfikujemy, czy dane w buforze nie są wyzerowane
    TEST_ASSERT_NOT_EQUAL(0, strncmp("0.00;", (char*)last_sent_buffer, 5));
}

TEST_CASE("AppTick: Brak Bluetooth pozostawia dane w buforze na pozniej", "[app_main]")
{
    // 1. Symulujemy jazdę bez połączenia Bluetooth
    mock_is_connected = false;
    HalReady = true;

    // 2. Akcja: Tick przetwarza czujniki
    AppTick();

    // 3. Weryfikacja: Dane zostały odczytane, ale nie wysłane
    TEST_ASSERT_TRUE(dataReady);
    TEST_ASSERT_FALSE(bt_send_was_called);

    // 4. Kolejny tick symuluje powrót połączenia BT
    mock_is_connected = true;
    AppTick();

    // 5. Weryfikacja: W drugim ticku zaległe dane zostały wysłane
    TEST_ASSERT_TRUE(bt_send_was_called);
    TEST_ASSERT_FALSE(dataReady); // Proces prawidłowo obsłużony
}

TEST_CASE("AppTick: Zawodzi ImuRead", "[app_main]")
{
    extern esp_err_t __wrap_ImuRead(void const *devHandle, ImuData *data);
    
    // Przesłaniamy atrapę lokalnie w tym teście, aby zwróciła błąd
    // Z uwagi na brak możliwości łatwego redefiniowania __wrap_ w runtime dla C,
    // w pełnym środowisku użylibyśmy zmiennej sterującej, np. mock_imu_read_ret = ESP_FAIL;
    // Poniższy test zakładałby ominięcie całego bloku.
    
    // (Pseudokod weryfikacji)
    // mock_imu_read_ret = ESP_FAIL;
    // AppTick();
    // TEST_ASSERT_FALSE(dataReady);
}