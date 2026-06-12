#include "led.h"

#ifdef SP_DEBUG
#define SP_LED_DEBUG
#endif

#ifdef SP_LED_DEBUG
static const char *LED_TAG = "Led";
#endif

esp_err_t LedInit(gpio_num_t ledGpio)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << ledGpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    return gpio_config(&conf);
}

void LedEnable(gpio_num_t ledGpio)
{
    gpio_set_level(ledGpio, 1);
}

void LedDisable(gpio_num_t ledGpio)
{
    gpio_set_level(ledGpio, 0);
}