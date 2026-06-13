#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "math.h"
#include "unity.h"

#include "main.h"
#include "bluetooth.h"
#include "halAH49E.h"
#include "imuB160.h"
#include "i2c.h"
#include "linearBuffer.h"
#include "types.h"

#ifdef SP_DEBUG
#define SP_MAIN_DEBUG
#endif

#ifdef SP_MAIN_DEBUG
static const char *MAIN_TAG = "Main";
#endif

AppData sAppData = {0};
i2c_master_dev_handle_t I2cHandle;
ImuData sImuData = {0};
uint8 SendBuffer[SEND_BUFFER_SIZE];
LinearBuffer LinearSendBuffer;
bool volatile HalReady = false;
bool dataReady;

/*******************************************************
 * @brief   Calculates the circumference of a wheel
 * 
 * @details Computes  the circumference based on the provided diameter
 *          and stores the result in the AppData structure.
 * 
 * @param   appData - A pointer to the AppData structure where the result is stored.
 * @param   diameter - a float32 value representing the diameter of the wheel.
 * 
 * @return  none
 *******************************************************/
void CalculateCircumference(AppData *appData, float32 diameter);

/*******************************************************
 * @brief   Calculates the current velocity.
 * 
 * @details Calculates the velocity based on the last record interval and wheel circumference. the result is converted to km/h.
 * 
 * @param   appData - A constant pointer to the AppData structure containing measurment data.
 * 
 * @return  A float32 value representing the velocity. Returns -1.0f if the interval is invalid.
 *******************************************************/
float32 CalculateVelocity(AppData const *appData);

/*******************************************************
 * @brief   Calculates the pitch angle from IMU accelerometer data.
 * 
 * @details Uses the atan2 function to calculate the pitch anfle in radians and then converts it to degrees.
 * 
 * @param   imuData - A constant pointer to the ImuData structure containing raw sensor values.
 * 
 * @return  A float32 value representing the pitch angle in degrees.
 *******************************************************/
float32 CalculatePitch(ImuData const *imuData);

void AppTick(void);

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n================================================\n");
    printf("starting Unit Test...\n");
    printf("================================================\n\n");

    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();

    while(true)
    {
        AppTick();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void CalculateCircumference(AppData *appData, float32 diameter)
{
  appData->circumference = M_PI * diameter;
}

float32 CalculateVelocity(AppData const *appData)
{
  if (0 >= appData->interval)
  {
    return -1.0f;
  }

  return (appData->circumference/(float32)appData->interval) * M_PER_US_TO_KM_PER_H_CONVERSION_CONSTANT;
}

float32 CalculatePitch(ImuData const *imuData)
{
  return (atan2(-imuData->accX, sqrt((imuData->accY * imuData->accY) + (imuData->accZ * imuData->accZ))) * (180.0f/M_PI));
}

void AppTick(void)
{
    if (false == dataReady)
    {
        //Set Measurement Led
        vTaskDelay(pdMS_TO_TICKS(MEASURMENT_INTERVAL_MS));
        esp_err_t imuReadRetVal = ImuRead(&I2cHandle, &sImuData);

        if (ESP_OK == imuReadRetVal)
        {
            int64 halTimeoutStart = esp_timer_get_time();
            while (false == HalReady)
            {
                vTaskDelay(pdMS_TO_TICKS(50));
                int64 halTimeoutInterval = esp_timer_get_time();
                if ((halTimeoutInterval - halTimeoutStart) >= HAL_TIMEOUT_US)
                {
                    ESP_LOGE(MAIN_TAG, "Error: Failed to read from hal sensor");
                    break;
                }
            }
            float32 velocity = CalculateVelocity(&sAppData);
            float32 pitch = CalculatePitch(&sImuData);
            InsertFloat(&LinearSendBuffer, velocity);
            InsertFloat(&LinearSendBuffer, sAppData.distance);
            InsertFloat(&LinearSendBuffer, pitch);
            //Disable Measurement Led
            HalReady = false;
            dataReady = true;
        }
    }

    if (true == IsConnected() && true == dataReady)
    {
        //Enable send Led
        esp_err_t btSendRetVal = BtSend(LinearSendBuffer.beginning, SEND_BUFFER_SIZE);
        if (ESP_OK == btSendRetVal)
        {
            ClearLinearBuffer(&LinearSendBuffer);
            dataReady = false;
            //Disable Error Led and send Led
        }
        else 
        {
            ESP_LOGW(MAIN_TAG, "Warning: Failed to send message");
            //Enable Error Led
        }
    }
}