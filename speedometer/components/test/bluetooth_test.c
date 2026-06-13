#include "unity.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "bluetooth.h"

static bool is_bt_initialized = false;

void setUp(void)
{
    ;
}

void tearDown(void)
{
    ;
}

TEST_CASE("Initial IsConnected state is false", "[bluetooth][on-target]")
{
    TEST_ASSERT_FALSE(IsConnected());
}

TEST_CASE("BtInit physically initializes NVS and NimBLE stack", "[bluetooth][on-target]")
{
    if (!is_bt_initialized) 
    {
        esp_err_t ret = BtInit();
        TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
        is_bt_initialized = true;
    } 
    else 
    {
        TEST_PASS_MESSAGE("Bluetooth is already initialized in this session.");
    }
}

TEST_CASE("BtConfigureAdvertising starts radio advertising", "[bluetooth][on-target]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    esp_err_t ret = BtConfigureAdvertising();
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("BtSend returns ESP_FAIL when there is no physical connection", "[bluetooth][on-target]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    uint8_t testData[] = {0x01, 0x02, 0x03};
    esp_err_t ret = BtSend(testData, sizeof(testData));
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);
}

TEST_CASE("BtGapEventHandler correctly updates connection state", "[bluetooth][on-target]")
{
    struct ble_gap_event event = {0};
    
    event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.status = 0;
    event.connect.conn_handle = 123;

    BtGapEventHandler(&event, NULL);
    TEST_ASSERT_TRUE(IsConnected());

    event.type = BLE_GAP_EVENT_DISCONNECT;
    BtGapEventHandler(&event, NULL);
    
    TEST_ASSERT_FALSE(IsConnected());
}

TEST_CASE("BtGapEventHandler forces advertising restart after connection error", "[bluetooth][on-target]")
{
    struct ble_gap_event event = {0};
    
    event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.status = 1;

    esp_err_t ret = BtGapEventHandler(&event, NULL);
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    TEST_ASSERT_FALSE(IsConnected());
}

TEST_CASE("Simulation: Fast connection drop and resume (Flaky Connection)", "[bluetooth][stress]")
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
    
    for (int i = 0; i < 1000; i++)
    {
        connectEvent.connect.status = 0;
        connectEvent.connect.conn_handle = i;
        BtGapEventHandler(&connectEvent, NULL);
        TEST_ASSERT_TRUE(IsConnected());

        BtGapEventHandler(&disconnectEvent, NULL);
        TEST_ASSERT_FALSE(IsConnected());
    }
    
    connectEvent.connect.conn_handle = 9999;
    BtGapEventHandler(&connectEvent, NULL);
    TEST_ASSERT_TRUE(IsConnected());
    
    BtGapEventHandler(&disconnectEvent, NULL);
}

TEST_CASE("Simulation: Multiple devices attempt to connect simultaneously", "[bluetooth][stress]")
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

    eventDevA.connect.status = 0;
    eventDevA.connect.conn_handle = 10;
    BtGapEventHandler(&eventDevA, NULL);
    TEST_ASSERT_TRUE(IsConnected());

    eventDevB.connect.status = 0;
    eventDevB.connect.conn_handle = 20;
    BtGapEventHandler(&eventDevB, NULL);
    
    TEST_ASSERT_TRUE(IsConnected());

    uint8_t dummyData[] = {0xAA};
    BtSend(dummyData, sizeof(dummyData)); 

    BtGapEventHandler(&disconnectEvent, NULL);
}

TEST_CASE("Simulation: Connection recovery after previous failed connection", "[bluetooth][stress]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    struct ble_gap_event event = {0};
    event.type = BLE_GAP_EVENT_CONNECT;
    
    event.connect.status = 5;
    event.connect.conn_handle = 0;
    BtGapEventHandler(&event, NULL);
    
    TEST_ASSERT_FALSE(IsConnected());

    event.connect.status = 0; 
    event.connect.conn_handle = 42;
    BtGapEventHandler(&event, NULL);
    
    TEST_ASSERT_TRUE(IsConnected());
    
    event.type = BLE_GAP_EVENT_DISCONNECT;
    BtGapEventHandler(&event, NULL);
}

TEST_CASE("Sending a packet larger than MTU", "[bluetooth][mtu]")
{
    if (!is_bt_initialized) 
    {
        TEST_ASSERT_EQUAL_INT(ESP_OK, BtInit());
        is_bt_initialized = true;
    }

    struct ble_gap_event connectEvent = {0};
    connectEvent.type = BLE_GAP_EVENT_CONNECT;
    connectEvent.connect.status = 0;
    connectEvent.connect.conn_handle = 100;
    BtGapEventHandler(&connectEvent, NULL);

    uint8_t oversizedData[600] = {0xAA}; 
    esp_err_t ret = BtSend(oversizedData, sizeof(oversizedData));
    
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, ret);

    struct ble_gap_event disconnectEvent = {0};
    disconnectEvent.type = BLE_GAP_EVENT_DISCONNECT;
    BtGapEventHandler(&disconnectEvent, NULL);
}