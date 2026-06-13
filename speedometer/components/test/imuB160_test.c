#include "unity.h"
#include "esp_err.h"
#include <string.h>
#include "imuB160.h"

static int mock_i2c_send_call_count = 0;
static esp_err_t mock_i2c_send_return_val = ESP_OK;
static uint8 mock_last_send_buff[2][10];

static esp_err_t mock_i2c_send_read_return_val = ESP_OK;
static uint8 mock_fake_imu_registers[12];
static uint8 mock_last_start_reg = 0;

esp_err_t __wrap_I2cSend(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const size)
{
    if (mock_i2c_send_call_count < 2) 
    {
        memcpy(mock_last_send_buff[mock_i2c_send_call_count], sendBuff, size);
    }
    mock_i2c_send_call_count++;
    
    return mock_i2c_send_return_val;
}

esp_err_t __wrap_I2cSendRead(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const sendBuffSize, uint8 *readBuff, uint32 const readBuffSize)
{
    mock_last_start_reg = sendBuff[0];
    
    if (ESP_OK != mock_i2c_send_read_return_val)
    {
        return mock_i2c_send_read_return_val;
    }

    memcpy(readBuff, mock_fake_imu_registers, readBuffSize);
    return ESP_OK;
}

void setUp(void)
{
    mock_i2c_send_call_count = 0;
    mock_i2c_send_return_val = ESP_OK;
    mock_i2c_send_read_return_val = ESP_OK;
    memset(mock_last_send_buff, 0, sizeof(mock_last_send_buff));
    memset(mock_fake_imu_registers, 0, sizeof(mock_fake_imu_registers));
    mock_last_start_reg = 0;
}

void tearDown(void)
{
    ;
}

TEST_CASE("ImuWakeUp sends correct commands to enable Accel and Gyro", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    
    esp_err_t ret = ImuWakeUp(&dummyHandle);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    
    TEST_ASSERT_EQUAL_INT(2, mock_i2c_send_call_count);
    
    TEST_ASSERT_EQUAL_HEX8(0x7E, mock_last_send_buff[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x11, mock_last_send_buff[0][1]);

    TEST_ASSERT_EQUAL_HEX8(0x7E, mock_last_send_buff[1][0]);
    TEST_ASSERT_EQUAL_HEX8(0x15, mock_last_send_buff[1][1]);
}

TEST_CASE("ImuWakeUp aborts and returns an error if enabling fails", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    mock_i2c_send_return_val = ESP_FAIL;

    esp_err_t ret = ImuWakeUp(&dummyHandle);

    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_i2c_send_call_count);
}

TEST_CASE("ImuRead correctly parses raw bits into floating-point values", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    ImuData data = {0};

    mock_fake_imu_registers[0] = 0x68; 
    mock_fake_imu_registers[1] = 0x06; 

    mock_fake_imu_registers[2] = 0x98; 
    mock_fake_imu_registers[3] = 0xF9; 

    mock_fake_imu_registers[10] = 0x00; 
    mock_fake_imu_registers[11] = 0x40; 

    esp_err_t ret = ImuRead(&dummyHandle, &data);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    TEST_ASSERT_EQUAL_HEX8(0x0C, mock_last_start_reg); 

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, data.gyroX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -100.0f, data.gyroY);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.gyroZ);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.accX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, data.accY);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, data.accZ);
}

TEST_CASE("ImuRead returns an error and does not modify data if I2C read fails", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;

    ImuData data = {
        .gyroX = 10.0f, .gyroY = 10.0f, .gyroZ = 10.0f,
        .accX = 10.0f, .accY = 10.0f, .accZ = 10.0f
    };

    mock_i2c_send_read_return_val = ESP_FAIL;
    esp_err_t ret = ImuRead(&dummyHandle, &data);

    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, data.accX);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, data.gyroZ);
}

TEST_CASE("ImuRead correctly processes extreme type boundaries (e.g., -32768 and 32767)", "[imu]")
{
    i2c_master_dev_handle_t dummyHandle = NULL;
    ImuData data = {0};

    mock_fake_imu_registers[6] = 0x00; 
    mock_fake_imu_registers[7] = 0x80; 

    mock_fake_imu_registers[4] = 0xFF;
    mock_fake_imu_registers[5] = 0x7F; 

    esp_err_t ret = ImuRead(&dummyHandle, &data);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);  
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -2.0f, data.accX);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1997.98f, data.gyroZ);
}