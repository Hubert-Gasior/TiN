#include "imuB160.h"

#ifdef SP_DEBUG
#define SP_IMU_DEBUG
#endif

#ifdef SP_IMU_DEBUG
static const char *IMU_TAG = "IMU";
#endif

esp_err_t ImuWakeUp(i2c_master_dev_handle_t const *devHandle)
{
    uint8 accEnable[2] = {0x7E, 0x11};
    uint8 gyroEnable[2]  = {0x7E, 0x15};

    esp_err_t const accEnableRetVal = I2cSend(devHandle, accEnable, sizeof(accEnable));
    if (ESP_OK != accEnableRetVal)
    {
        #ifdef SP_IMU_DEBUG
        ESP_LOGE(IMU_TAG, "Error: Failed to enable the accelerometer");
        #endif
        return accEnableRetVal;
    }

    esp_err_t const gyroEnableRetVal = I2cSend(devHandle, gyroEnable, sizeof(gyroEnable));
    if (ESP_OK != gyroEnableRetVal)
    {
        #ifdef SP_IMU_DEBUG
        ESP_LOGE(IMU_TAG, "Error: Failed to enable the gyroscope");
        #endif
        return gyroEnableRetVal;
    }

    return ESP_OK;
}

esp_err_t ImuRead(i2c_master_dev_handle_t const *devHandle, ImuData *data)
{
    uint8 startReg = 0xC;
    uint8 rawData[12];

    esp_err_t readImu = I2cSendRead(devHandle, &startReg, sizeof(startReg), rawData, sizeof(rawData));
    if (ESP_OK != readImu)
    {
        #ifdef SP_IMU_DEBUG
        ESP_LOGE(IMU_TAG, "Error: Failed to read IMU");
        #endif
        return readImu;
    }

    data->gyroX = ((int16)(rawData[1] << 8) | rawData[0]) / 16.4f;
    data->gyroY = ((int16)(rawData[3] << 8) | rawData[2]) / 16.4f;
    data->gyroZ = ((int16)(rawData[5] << 8) | rawData[4]) / 16.4f;

    data->accX = ((int16)(rawData[7] << 8) | rawData[6]) / 16384.0f;
    data->accY = ((int16)(rawData[9] << 8) | rawData[8]) / 16384.0f;
    data->accZ = ((int16)(rawData[11] << 8) | rawData[10]) / 16384.0f;

    return ESP_OK;
}