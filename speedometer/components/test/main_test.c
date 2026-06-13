#include "unity.h"
#include "esp_err.h"
#include <string.h>

#include "main.h" 
#include "types.h"
#include "linearBuffer.h"
#include "imuB160.h"

extern float32 CalculateVelocity(AppData const *appData);
extern float32 CalculatePitch(ImuData const *imuData);

static int64 mock_system_time_us = 0;
static bool mock_is_connected = false;
static bool bt_send_was_called = false;
static uint8 last_sent_buffer[SEND_BUFFER_SIZE];

static esp_err_t mock_imu_read_ret = ESP_OK;
static esp_err_t mock_bt_send_ret = ESP_OK;

extern LinearBuffer LinearSendBuffer;
extern bool volatile HalReady;
extern bool dataReady;
extern AppData sAppData;
extern void AppTick(void);

int64 __wrap_esp_timer_get_time(void) 
{
    return mock_system_time_us;
}

void __wrap_vTaskDelay(uint32_t ticks) 
{
    mock_system_time_us += (ticks * 1000);
}

bool __wrap_IsConnected(void) 
{
    return mock_is_connected;
}

esp_err_t __wrap_ImuRead(void const *devHandle, ImuData *data) 
{
    if (mock_imu_read_ret != ESP_OK) 
    {
        return mock_imu_read_ret;
    }

    data->accX = 0.0f; 
    data->accY = 0.0f; 
    data->accZ = 1.0f; 
    return ESP_OK;
}

esp_err_t __wrap_BtSend(uint8 const *buffer, uint32 const size) 
{
    bt_send_was_called = true;
    if (mock_bt_send_ret != ESP_OK) 
    {
        return mock_bt_send_ret;
    }
    memcpy(last_sent_buffer, buffer, size);
    return ESP_OK;
}

void setUp(void)
{
    mock_system_time_us = 0;
    mock_is_connected = true;
    bt_send_was_called = false;
    mock_imu_read_ret = ESP_OK;   
    mock_bt_send_ret = ESP_OK;    
    memset(last_sent_buffer, 0, sizeof(last_sent_buffer));
    
    HalReady = false;
    dataReady = false;
    sAppData.distance = 100.0f; 
    sAppData.circumference = 2.0f;
    sAppData.interval = 500000;
    
    static uint8 dummySendBuff[SEND_BUFFER_SIZE];
    LinearBufferInit(&LinearSendBuffer, dummySendBuff, SEND_BUFFER_SIZE);
}

void tearDown(void) 
{
    ;
}

TEST_CASE("AppTick: Correctly reads sensors and sends via Bluetooth", "[app_main]")
{
    HalReady = true;
    AppTick();

    TEST_ASSERT_TRUE(dataReady);
    TEST_ASSERT_FALSE(HalReady);
    TEST_ASSERT_TRUE(bt_send_was_called);
    TEST_ASSERT_FALSE(dataReady);
}

TEST_CASE("AppTick: Application continues execution after Hall sensor timeout", "[app_main]")
{
    HalReady = false;

    AppTick();
    TEST_ASSERT_TRUE(dataReady == false); 
    TEST_ASSERT_TRUE(bt_send_was_called);
    TEST_ASSERT_NOT_EQUAL(0, strncmp("0.00;", (char*)last_sent_buffer, 5));
}

TEST_CASE("AppTick: Lack of Bluetooth leaves data in the buffer for later", "[app_main]")
{
    mock_is_connected = false;
    HalReady = true;

    AppTick();

    TEST_ASSERT_TRUE(dataReady);
    TEST_ASSERT_FALSE(bt_send_was_called);
    mock_is_connected = true;
    AppTick();

    TEST_ASSERT_TRUE(bt_send_was_called);
    TEST_ASSERT_FALSE(dataReady);
}

TEST_CASE("CalculateVelocity: Protection against division by zero and negative time", "[math]")
{
    AppData testData = { .circumference = 2.0f, .interval = 0 };
    
    float32 velZero = CalculateVelocity(&testData);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, velZero);

    testData.interval = -500;
    float32 velNeg = CalculateVelocity(&testData);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, velNeg);
}

TEST_CASE("CalculatePitch: Behavior in extreme positions (Gimbal Lock / Freefall)", "[math]")
{
    ImuData testImu = {0};

    testImu.accX = 1.0f; testImu.accY = 0.0f; testImu.accZ = 0.0f;
    float32 pitchDown = CalculatePitch(&testImu);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -90.0f, pitchDown);

    testImu.accX = -1.0f; testImu.accY = 0.0f; testImu.accZ = 0.0f;
    float32 pitchUp = CalculatePitch(&testImu);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 90.0f, pitchUp);

    testImu.accX = 0.0f; testImu.accY = 0.0f; testImu.accZ = 0.0f;
    float32 pitchFreefall = CalculatePitch(&testImu);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, pitchFreefall);
}

TEST_CASE("AppTick: ImuRead fails (I2C bus failure)", "[app_main]")
{
    mock_imu_read_ret = ESP_FAIL;
    HalReady = true;

    AppTick();

    TEST_ASSERT_FALSE(dataReady);
    TEST_ASSERT_FALSE(bt_send_was_called);
}

TEST_CASE("AppTick: Packet rejection by Bluetooth (Queuing for retransmission)", "[app_main]")
{
    HalReady = true;
    mock_is_connected = true; 
    
    mock_bt_send_ret = ESP_FAIL; 

    AppTick();

    TEST_ASSERT_TRUE(bt_send_was_called);
    TEST_ASSERT_TRUE(dataReady);
}