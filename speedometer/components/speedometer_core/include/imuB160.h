#ifndef IMUB160_H_
#define IMUB160_H_

#include "i2c.h"
#include "esp_log.h"

typedef struct 
{
    float32 gyroX;
    float32 gyroY;
    float32 gyroZ;

    float32 accX;
    float32 accY;
    float32 accZ;
} ImuData;

/*******************************************************
 * @brief   Wakes up the IMU sensors from suspend mode.
 * 
 * @details Sends two consecutive commands (0x11 and 0x15) to the sensor's command register 0x&e to transition both the accelerometer
 *          and the gyroscope into normal power mode.
 * 
 * @param   devHandle - A constant pointer to the i2c_master_dev_handle_t structure storing of the initialized I2C device.
 * 
 * @return  esp_err_t - ESP_OK on success, or the error code of the failed transmission.
 *******************************************************/
esp_err_t ImuWakeUp(i2c_master_dev_handle_t const *devHandle);

/*******************************************************
 * @brief   Reads, parses and scales accelerometer and gyroscope data.
 * 
 * @details Performs a burst read transaction od 12 bytes starting from the gyroscope data register (0x0C). The function automatically parses the received
 *          LSB and MSB bytes into 16-bit signed integers for all three axes of both sensors and then scale to physical units:
 *          - Gyroscope: scaled by 1/16.4 (for +/- 2000 [deg/s] range).
 *          - Accelerometer: scaled by 1/16384 (for +/- 2 [g] range).
 * 
 * @param   devHandle - A constant pointer to the i2c_master_dev_handle_t structure storing the initialized I2C device.
 * @param   data - Pointer to the ImuData structure where the results will be stored.
 * 
 * @return  esp_err_t - ESP_OK on successful read and parse, error code otherwise.
 *******************************************************/
esp_err_t ImuRead(i2c_master_dev_handle_t const *devHandle, ImuData *data);

#endif /* IMUB160_H_ */