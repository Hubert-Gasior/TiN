#include "bluetooth.h"

#ifdef SP_DEBUG
#define SP_BT_DEBUG
#endif

#ifdef SP_BT_DEBUG
static const char *BT_TAG = "Bluetooth_LE";
#endif

static uint16 activeConnection = 0xFFFF;
static uint16 sendCallback;
static uint8 recvBuffer[RECV_BUFF_SIZE];

static const ble_uuid128_t SPEEDOMETER_UUID = BLE_UUID128_INIT(0xc9, 0x6d, 0x41, 0xef, 0x81, 0x9d, 0x42, 0xaa, 0xb2, 0x78, 0x89, 0x22, 0x77, 0x06, 0xde, 0xe6);
static const ble_uuid128_t SEND_UUID = BLE_UUID128_INIT(0xff, 0x4b, 0x69, 0x11, 0x64, 0x47, 0x49, 0x82, 0xb0, 0x38, 0xca, 0xc8, 0xb8, 0x37, 0x93, 0x73);
static const ble_uuid128_t RECV_UUID = BLE_UUID128_INIT(0x27, 0x3e, 0xc0, 0x27, 0x70, 0xcb, 0x4b, 0x22, 0x9b, 0x61, 0x12, 0xbb, 0x93, 0xc4, 0x48, 0x19);

static const struct ble_gatt_chr_def servicesCharacteristics[] =
{
    {
        .uuid = &SEND_UUID.u,
        .access_cb = BtRecv,
        .flags = BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &sendCallback,
    },
    {
        .uuid = &RECV_UUID.u,
        .access_cb = BtRecv,
        .flags = BLE_GATT_CHR_F_WRITE,
        .arg = recvBuffer,
    },
    {0}
};

static const struct ble_gatt_svc_def services[] =
{
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &SPEEDOMETER_UUID.u,
        .characteristics = servicesCharacteristics,
    },
    {0}
};

/*******************************************************
 * @brief   Handles Bluetooth GAP events.
 * 
 * @details This callback manages connection state changes, restarts advertising upon disconnection or failed connection,
 *          logs MTU updates.
 * 
 * @param   event - A pointer to ble_gap_event structure describing the GAP events.
 * @param   arg - A void pointer to a user-defined argument passed to the handler.
 * 
 * @return  Always esp_err_t value ESP_OK  
 *******************************************************/

 //Erased static for testability
esp_err_t BtGapEventHandler(struct ble_gap_event *event, void *arg)
{
    switch(event->type)
    {
        case BLE_GAP_EVENT_CONNECT:
            if (0 == event->connect.status)
            {
                activeConnection = event->connect.conn_handle;
            #ifdef SP_BT_DEBUG
                ESP_LOGI(BT_TAG, "Bluetooth Connected");
            #endif
            }
            else
            {
                activeConnection = 0xFFFF;
                BtConfigureAdvertising();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
        #ifdef SP_BT_DEBUG
            ESP_LOGI(BT_TAG, "Bluetooth Disconnected");
        #endif
            activeConnection = 0xFFFF;
            BtConfigureAdvertising();
            break;
        case BLE_GAP_EVENT_MTU:
        #ifdef SP_BT_DEBUG
            ESP_LOGI(BT_TAG, "MTU changed: %d", event->mtu.value);
        #endif
            break;
        default:
            break;
    }

    return ESP_OK;
}

esp_err_t BtSend(uint8 const *buffer, uint32 const size)
{
    struct os_mbuf *sendBuffer = ble_hs_mbuf_from_flat(buffer, size);
    if (ESP_OK != ble_gattc_notify_custom(activeConnection, sendCallback, sendBuffer))
    {
    #ifdef SP_BT_DEBUG
        ESP_LOGW(BT_TAG, "Warning: Failed to send message");
    #endif
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t BtRecv(uint16 const connectionHandle, uint16 const attributeHandle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    //TODO
    return ESP_OK;
}

esp_err_t BtInit(void)
{
    esp_err_t const initNvsRetVal = nvs_flash_init();
    if (ESP_OK != initNvsRetVal)
    {
        nvs_flash_erase();
    #ifdef SP_BT_DEBUG
        ESP_LOGE(BT_TAG, "Error: Failed to initialize NVS during initialization");
    #endif
        return ESP_FAIL;
    }

    esp_err_t const nimblePortInitRetVal = nimble_port_init();
    if(ESP_OK != nimblePortInitRetVal)
    {
    #ifdef SP_BT_DEBUG
        ESP_LOGE(BT_TAG, "Error: Failed to initailize port for NimBLE");
    #endif
        return ESP_FAIL;
    }

    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 0;

    ble_gatts_reset(); 
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_gatts_count_cfg(services);
    ble_gatts_add_svcs(services);
    ble_hs_cfg.sync_cb = BtStartAdvertising;

    return ESP_OK;
}

esp_err_t BtConfigureAdvertising(void)
{
    struct ble_gap_adv_params advertiseParameters;
    struct ble_hs_adv_fields advertiseFields;

    memset(&advertiseFields, 0, sizeof(advertiseFields));
    advertiseFields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    advertiseFields.name = (uint8 *)DEVICE_NAME;
    advertiseFields.name_len = strlen(DEVICE_NAME);
    advertiseFields.name_is_complete = 1;

    esp_err_t setFieldsRetVal = ble_gap_adv_set_fields(&advertiseFields);
    if (ESP_OK != setFieldsRetVal)
    {
    #ifdef SP_BT_DEBUG
        ESP_LOGE(BT_TAG, "Error: while setting advertisment fields: %d", setFieldsRetVal);
    #endif
        return ESP_FAIL;
    }

    memset(&advertiseParameters, 0, sizeof(advertiseParameters));
    advertiseParameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertiseParameters.disc_mode = BLE_GAP_DISC_MODE_GEN; 


    esp_err_t startAdvertising = ble_gap_adv_start(BLE_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &advertiseParameters, BtGapEventHandler, NULL);
    if (ESP_OK != startAdvertising)
    {
    #ifdef SP_BT_DEBUG
        ESP_LOGE(BT_TAG, "Error: Failed to start advertising");
    #endif
    return ESP_FAIL;
    }
    return ESP_OK;
}

void BtStartAdvertising(void)
{
#ifdef SP_BT_DEBUG
    ESP_LOGI(BT_TAG, "Starting advertising");
#endif
    BtConfigureAdvertising();
}

void BtTask(void *arg)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void StartBtService(void)
{
    nimble_port_freertos_init(BtTask);
}

bool IsConnected(void)
{
    return (activeConnection != 0xFFFF);
}