#include "unity.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c.h" // Twój plik nagłówkowy

// Globalny uchwyt urządzenia dla testów sekwencyjnych
static i2c_master_dev_handle_t testDevHandle = NULL;
static bool isInitialized = false;

// --- Konfiguracja środowiska testowego ---

void setUp(void)
{
    // Jeśli moduł posiadałby funkcję I2cDeinit(), 
    // inicjalizacja (I2cInit) odbywałaby się tutaj dla KAŻDEGO testu.
}

void tearDown(void)
{
    // Tutaj powinno znaleźć się wywołanie zwalniające zasoby:
    // np. i2c_master_bus_rm_device(testDevHandle) 
    // oraz i2c_del_master_bus(busHandle)
}

// --- Przypadki testowe ---

TEST_CASE("I2cInit inicjalizuje magistrale i zwraca uchwyt urzadzenia", "[i2c]")
{
    // Wywołujemy inicjalizację tylko raz dla całej sesji testowej
    if (!isInitialized) 
    {
        esp_err_t ret = I2cInit(&testDevHandle);
        
        TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
        TEST_ASSERT_NOT_NULL(testDevHandle);
        isInitialized = true;
    }
    else
    {
        TEST_PASS_MESSAGE("Magistrala I2C zostala juz zainicjalizowana w poprzednim tescie.");
    }
}

TEST_CASE("I2cSend wysyla dane bez bledu (wymaga podlaczonego sprzetu)", "[i2c]")
{
    // Zabezpieczenie na wypadek uruchomienia tylko tego konkretnego testu
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    // Przykładowy bufor do wysłania (np. wybudzenie IMU, adres rejestru 0x00)
    uint8_t dummyData[] = {0x00}; 
    
    // UWAGA: Wynik tej funkcji zależy od fizycznego sprzętu. 
    // Jeśli układ IMU (IMU_ADDR) jest podłączony do SDA/SCL, zwróci ESP_OK.
    // Jeśli fizycznego układu nie ma, I2C nie otrzyma potwierdzenia (ACK) 
    // i funkcja zwróci błąd (zazwyczaj ESP_FAIL lub ESP_ERR_TIMEOUT).
    esp_err_t ret = I2cSend(&testDevHandle, dummyData, sizeof(dummyData));
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("I2cSendRead wysyla i odbiera dane z urzadzenia", "[i2c]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t sendData[] = {0x0F}; // Przykładowy rejestr "WHO_AM_I" w wielu IMU
    uint8_t readBuff[1] = {0};
    
    esp_err_t ret = I2cSendRead(&testDevHandle, sendData, sizeof(sendData), readBuff, sizeof(readBuff));
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    
    // Jeśli znasz przewidywaną wartość (np. ID chipu to 0x69), możesz dodać asercję:
    // TEST_ASSERT_EQUAL_HEX8(0x69, readBuff[0]);
}