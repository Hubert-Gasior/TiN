#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <stdio.h>

#include "types.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"

#define DEVICE_NAME "ESP32-Speedometer"
#define RECV_BUFF_SIZE 32

/*******************************************************
 * @brief   Send data to the connected client via notification.
 * 
 * @details Creates a message buffer from flat byte array and attempts to send it as a notification
 *          through the active connection.
 * 
 * @param   buffer - A constant uint8 pointer to the data to be send.
 * @param   size -  A constant uint32 value representing the number of bytes to be send.
 * 
 * @return  An esp_err_t value ESP_OK on success.
 *******************************************************/
esp_err_t BtSend(uint8 const *buffer, uint32 const size);

/*******************************************************
 * @brief   GATT access callback for receiving data.
 * 
 * @details TODO
 * 
 * @param   connectionHandle - A constant uint16 value representing the handle of the connection.
 * @param   attributeHandle - A constant uint16 value representing the handle of the characteristics attribute.
 * @param   ctxt - A pointer to ble_gatt_access_ctxt structure describing the GATT access context.
 * @param   arg - A void pointer to the user-defined argument passed to the callback function.
 * 
 * @return  An esp_err_t value ESP_OK on success.
 *******************************************************/
esp_err_t BtRecv(uint16 const connectionHandle, uint16 const attributeHandle, struct ble_gatt_access_ctxt *ctxt, void *arg);

/*******************************************************
 * @brief   Initializes the Bluetooth LE stack and services.
 * 
 * @details Sets up NVS flash, initilizes the NimBLE host stack,
 *          configures security parameters and registers GAP/GATT services and characteristics.
 * 
 * @param   None
 * 
 * @return  An esp_err_t value ESP_OK on success, ESP_FAIL if NVS or NimBLE init fails.
 *******************************************************/
esp_err_t BtInit(void);

/*******************************************************
 * @brief   Configures and starts BLE advertising.
 * 
 * @details Sets up advertising fields including device flags and name,
 *          configures connection/discovery modes, and starts the advertising process indefinitely.
 * 
 * @param   None
 * 
 * @return  An esp_err_t value ESP_OK on success, ESP_FAIL if field setting or start fails.
 *******************************************************/
esp_err_t BtConfigureAdvertising(void);

/*******************************************************
 * @brief   Synchronous callback to start advertising.
 * 
 * @details Acts as wrapper for BtConfigureAdvertising to ve called when the host stack is synchronized.
 * 
 * @param   None
 * 
 * @return  None
 *******************************************************/
void BtStartAdvertising(void);

/*******************************************************
 * @brief   The main NimBle host stack task.
 * 
 * @details Executes the NimBLE event loop and handles stack deinitialization when the task terminates.
 * 
 * @param   arg - A void pointer to task arguments (unused).
 * 
 * @return  None
 *******************************************************/
void BtTask(void *arg);

/*******************************************************
 * @brief   Launches the Bluetooth service task.
 * 
 * @details Initializes the FreeRTOS task that runs the NimBLE host stack loop.
 * 
 * @param   None
 * 
 * @return  None
 *******************************************************/
void StartBtService(void);

/*******************************************************
 * @brief   Checks if bluetooth connection is currently active
 * 
 * @details Validates the state of the connection by comparing the activeConnection handle against
 *          the defined invalid handle value
 * 
 * @param   None
 * 
 * @return  True if the device is connected, false otherwise.
 *******************************************************/
bool IsConnected(void);

// added for testability
esp_err_t BtGapEventHandler(struct ble_gap_event *event, void *arg);

#endif /* BLUETOOTH_H */