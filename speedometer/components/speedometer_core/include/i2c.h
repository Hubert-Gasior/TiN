#ifndef I2C_H
#define I2C_H

#include "types.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "driver/gpio.h"

#define SDA GPIO_NUM_8
#define SCL GPIO_NUM_9
#define IMU_ADDR 0x69

/*******************************************************
 * @brief   Initializes the I2C master bus and registers the IMU device
 * 
 * @details Configures the I2C master bus with specified pins, clock source and  timing.
 *          Upon successfull creation, it registers the IMU (BMI160) as a slave device on the bus using the definied IMU_ADDR
 * 
 * @param   devHandle - A pointer to the i2c_master_dev_handle_t structure that will store the registered I2C device
 * 
 * @return  esp_err_t - ESP_OK on success, or an error code from the I2C driver
 *******************************************************/
esp_err_t I2cInit(i2c_master_dev_handle_t *devHandle);

/*******************************************************
 * @brief   Transmits a data buffer to an I2C slave device.
 * 
 * @details Encapsulates the i2c_master_transmit function
 * 
 * @param   devHandle - A constant pointer to the i2c_master_dev_handle_t structure storing the device handle
 * @param   sendBuff - A pointer to the constatnt buffer containing data to send.
 * @param   size - The number of bytes to be transmited
 * 
 * @return  esp_err_t - ESP_OK on successm or error code on failure.
 *******************************************************/
esp_err_t I2cSend(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const size);

/*******************************************************
 * @brief   Executes a combined I2C write-then-read transaction.
 * 
 * @details Encapsulates the i2c_master_transmit_receive function.
 * 
 * @param   devHandle - A constant pointer to the i2c_master_dev_handle_t structure storing the device handle.
 * @param   sendBuff - A pointer to a constant buffer containing data to be sent.
 * @param   sendBuffSize - The number of bytes to transmit.
 * @param   readBuff - A pointer to the buffer where received data will be stored.
 * @param   readBuffSize - The number of bytes to be read from the device.
 * 
 * @return  esp_err_t - ESP_OK in success, or error code on failure.
 *******************************************************/
esp_err_t I2cSendRead(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const sendBuffSize, uint8 *readBuff, uint32 const readBuffSize);

#endif /* I2C_H */