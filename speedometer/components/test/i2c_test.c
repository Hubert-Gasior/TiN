#include "unity.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c.h"

static i2c_master_dev_handle_t testDevHandle = NULL;
static bool isInitialized = false;

void setUp(void)
{
    ;
}

void tearDown(void)
{
    ;
}

TEST_CASE("I2cInit initializes the bus and returns the device handle", "[i2c]")
{
    if (!isInitialized) 
    {
        esp_err_t ret = I2cInit(&testDevHandle);
        
        TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
        TEST_ASSERT_NOT_NULL(testDevHandle);
        isInitialized = true;
    }
    else
    {
        TEST_PASS_MESSAGE("I2C bus was already initialized in a previous test.");
    }
}

TEST_CASE("I2cSend sends data without error", "[i2c]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t dummyData[] = {0x00}; 
    esp_err_t ret = I2cSend(&testDevHandle, dummyData, sizeof(dummyData));
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("I2cSendRead sends and receives data from the device", "[i2c]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t sendData[] = {0x0F};
    uint8_t readBuff[1] = {0};
    
    esp_err_t ret = I2cSendRead(&testDevHandle, sendData, sizeof(sendData), readBuff, sizeof(readBuff));
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("I2cInit: Behavior on multiple initialization attempts", "[i2c][init]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    i2c_master_dev_handle_t dummyHandle = NULL;
    esp_err_t ret = I2cInit(&dummyHandle);
    
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

TEST_CASE("I2cSend: Resilience to invalid parameters (NULL, zero size)", "[i2c][send]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t dummyData[] = {0x00};

    esp_err_t retZero = I2cSend(&testDevHandle, dummyData, 0);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, retZero); 
}

TEST_CASE("I2cSendRead: Correct sequential read of multiple bytes from IMU", "[i2c][recv]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t targetRegister[] = {0x28}; 
    uint8_t axisData[6] = {0};

    esp_err_t ret = I2cSendRead(&testDevHandle, targetRegister, sizeof(targetRegister), axisData, sizeof(axisData));
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("I2cSendRead: Resilience to invalid receive buffers", "[i2c][recv]")
{
    if (!isInitialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, I2cInit(&testDevHandle));
        isInitialized = true;
    }

    uint8_t sendData[] = {0x0F};
    
    uint8_t dummyRead[1];
    esp_err_t retZeroRead = I2cSendRead(&testDevHandle, sendData, sizeof(sendData), dummyRead, 0);
    
    TEST_ASSERT_NOT_EQUAL(ESP_OK, retZeroRead);
}