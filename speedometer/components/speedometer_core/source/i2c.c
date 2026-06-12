#include "i2c.h"

#ifdef SP_DEBUG
#define SP_I2C_DEBUG
#endif

#ifdef SP_I2C_DEBUG
static const char *I2C_TAG = "I2C";
#endif

esp_err_t I2cInit(i2c_master_dev_handle_t *devHandle) 
{
    i2c_master_bus_config_t busConfig = 
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL,
        .sda_io_num = SDA,
        .glitch_ignore_cnt = 7,
    };

    i2c_master_bus_handle_t busHandle;
    esp_err_t i2cNewMasterBusRetVal = i2c_new_master_bus(&busConfig, &busHandle);
    #ifdef SP_I2C_DEBUG
    if (ESP_OK != i2cNewMasterBusRetVal)
    {
        ESP_LOGE(I2C_TAG, "Error: Failed to create a new I2C master bus");
    }
    #endif

    i2c_device_config_t devConfig = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMU_ADDR,
        .scl_speed_hz = 100000,
    };
    
    return i2c_master_bus_add_device(busHandle, &devConfig, devHandle);
}

esp_err_t I2cSend(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const size)
{
    esp_err_t const i2cTransmitRetVal = i2c_master_transmit(*devHandle, sendBuff, size, -1);
    if (ESP_OK != i2cTransmitRetVal)
    {
        #ifdef SP_I2C_DEBUG
        ESP_LOGE(I2C_TAG, "Error: Failed to send the buffer");
        #endif
        return i2cTransmitRetVal;
    }

    return ESP_OK;
}

esp_err_t I2cSendRead(i2c_master_dev_handle_t const *devHandle, uint8 const *sendBuff, uint32 const sendBuffSize, uint8 *readBuff, uint32 const readBuffSize)
{
    esp_err_t  i2cSendReadRetVal = i2c_master_transmit_receive(*devHandle, sendBuff, sendBuffSize, readBuff, readBuffSize, -1);
    if (ESP_OK != i2cSendReadRetVal)
    {
        #ifdef SP_I2C_DEBUG
        ESP_LOGE(I2C_TAG, "Error: Failed to send and read from device");
        #endif
        return i2cSendReadRetVal;
    }

    return ESP_OK;
}