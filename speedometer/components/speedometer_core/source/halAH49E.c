#include "halAH49E.h"

#ifdef SP_DEBUG
#define SP_HAL_DEBUG
#endif

#ifdef SP_HAL_DEBUG
static const char *HAL_TAG = "Hal";
#endif

static volatile int64 nowTime = 0;
static volatile int64 previousTime = 0;
extern volatile bool HalReady;

/*******************************************************
 * @brief   Interrupt Service Routine for HAL GPIO
 * 
 * @details This function is executed in the IRAM.
 *          it implements software debouncing by checking the time
 *          difference since the last trigger. If the interval is sufficient,
 *          it updates the total distance and the time interval betweenn the measurments. 
 * 
 * @param   arg - A void pointer to the user data (expected AppData*) used to
 *          update distance and timing information.
 * 
 * @return  none
 *******************************************************/
static void IRAM_ATTR HalISR(void* arg)
{
    int64 now = esp_timer_get_time();
    int64 diff = now - previousTime;
    if (diff >= MIN_HAL_MEASURMENT_INTERVAL_US)
    {
        AppData *appData = (AppData *)arg;
        appData->distance += appData->circumference;
        appData->interval = diff;
        previousTime = now;
        HalReady = true;
    }
}

esp_err_t HalInit(gpio_num_t inputPin, AppData *appData)
{
    gpio_config_t config = {
        .intr_type = GPIO_INTR_POSEDGE,
        .pin_bit_mask = (1ULL << inputPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .pull_down_en = 1,
    };

    esp_err_t gpioConfRetVal = gpio_config(&config);
    #ifdef SP_HAL_DEBUG
    if (ESP_OK != gpioConfRetVal)
    {
        ESP_LOGE(HAL_TAG, "Error: Failed to configure the HAL gpio pin");
    }
    #endif

    esp_err_t gpioInstallIsrServRetVal = gpio_install_isr_service(0);
    #ifdef SP_HAL_DEBUG
    if (ESP_OK !=  gpioInstallIsrServRetVal)
    {
        ESP_LOGE(HAL_TAG, "Error: Failed to install HAL isr service");
    }
    #endif

    return gpio_isr_handler_add(inputPin, HalISR, (void*)appData);
}