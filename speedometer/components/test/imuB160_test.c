#include "unity.h"
#include "esp_err.h"
#include <string.h>
#include "imuB160.h" // Twój plik nagłówkowy

// --- Zmienne środowiska testowego (Mock State) ---
static int mock_i2c_send_call_count = 0;
static esp_err_t mock_i2c_send_return_val = ESP_OK;
static uint8 mock_last_send_buff[2][10];

static esp_err_t mock_i2c_send_read_return_val = ESP_OK;
static uint8 mock_fake_imu_registers[12];
static uint8 mock_last_start_reg = 0;

// --- Atrapy (Mocks) funkcji z i2c.h ---
// Przedrostek __wrap_ informuje linker, by użył tych funkcji zamiast oryginalnych

esp_err_t __wrap_I2cSend(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const size)
{
    // Zapisujemy wywołania, aby móc je zweryfikować w teście
    if (mock_i2c_send_call_count < 2) 
    {
        memcpy(mock_last_send_buff[mock_i2c_send_call_count], sendBuff, size);
    }
    mock_i2c_send_call_count++;
    
    return mock_i2c_send_return_val;
}

esp_err_t __wrap_I2cSendRead(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const sendBuffSize, uint8 *readBuff, uint32 const readBuffSize)
{
    // Zapisujemy, o jaki rejestr zapytał moduł IMU
    mock_last_start_reg = sendBuff[0];
    
    if (ESP_OK != mock_i2c_send_read_return_val)
    {
        return mock_i2c_send_read_return_val;
    }

    // Podstawiamy nasze spreparowane do testów dane
    memcpy(readBuff, mock_fake_imu_registers, readBuffSize);
    return ESP_OK;
}

// --- Konfiguracja środowiska testowego ---

void setUp(void)
{
    // Resetowanie stanu przed każdym testem
    mock_i2c_send_call_count = 0;
    mock_i2c_send_return_val = ESP_OK;
    mock_i2c_send_read_return_val = ESP_OK;
    memset(mock_last_send_buff, 0, sizeof(mock_last_send_buff));
    memset(mock_fake_imu_registers, 0, sizeof(mock_fake_imu_registers));
    mock_last_start_reg = 0;
}

void tearDown(void)
{
}

// --- Przypadki testowe ---

TEST_CASE("ImuWakeUp wysyla poprawne komendy wlaczenia Accel i Gyro", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL; // Nie potrzebujemy prawdziwego uchwytu
    
    esp_err_t ret = ImuWakeUp(&dummyHandle);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    
    // Sprawdzamy, czy I2cSend zostało wywołane dokładnie 2 razy
    TEST_ASSERT_EQUAL_INT(2, mock_i2c_send_call_count);
    
    // Weryfikacja payloadu dla akcelerometru
    TEST_ASSERT_EQUAL_HEX8(0x7E, mock_last_send_buff[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, mock_last_send_buff[0][1]);

    // Weryfikacja payloadu dla żyroskopu
    TEST_ASSERT_EQUAL_HEX8(0x7E, mock_last_send_buff[1][0]);
    TEST_ASSERT_EQUAL_HEX8(0x15, mock_last_send_buff[1][1]);
}

TEST_CASE("ImuWakeUp przerywa i zwraca blad jesli wlaczenie zawiedzie", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    
    // Symulujemy błąd na magistrali I2C
    mock_i2c_send_return_val = ESP_FAIL;
    
    esp_err_t ret = ImuWakeUp(&dummyHandle);

    // Powinno zwrócić błąd już po pierwszym wywołaniu I2cSend
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_i2c_send_call_count); // Drugie wywołanie nie powinno się odbyć
}

TEST_CASE("ImuRead poprawnie parsuje surowe bity na wartosci zmiennoprzecinkowe", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    ImuData data = {0};

    // 1. Gyro X: Ustawiamy wartość 1640, co po podzieleniu przez 16.4f powinno dać 100.0f
    // 1640 = 0x0668 (Little Endian -> LSB: 0x68, MSB: 0x06)
    mock_fake_imu_registers[0] = 0x68; 
    mock_fake_imu_registers[1] = 0x06; 
    
    // 2. Gyro Y: Ustawiamy ujemną wartość -1640, co powinno dać -100.0f
    // -1640 jako int16 to 0xF998 (Little Endian -> LSB: 0x98, MSB: 0xF9)
    mock_fake_imu_registers[2] = 0x98; 
    mock_fake_imu_registers[3] = 0xF9; 

    // 3. Accel Z: Ustawiamy wartość 16384, co po podzieleniu przez 16384.0f powinno dać 1.0f
    // 16384 = 0x4000 (Little Endian -> LSB: 0x00, MSB: 0x40)
    mock_fake_imu_registers[10] = 0x00; 
    mock_fake_imu_registers[11] = 0x40; 

    esp_err_t ret = ImuRead(&dummyHandle, &data);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0x0C, mock_last_start_reg); // Sprawdzamy czy czyta od poprawnego rejestru

    // Ponieważ to floaty, używamy makra z określoną tolerancją błędu (0.01f)
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, data.gyroX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -100.0f, data.gyroY);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.gyroZ); // Puste dane na Z powinny dać 0.0f
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.accX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.accY);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, data.accZ);
}