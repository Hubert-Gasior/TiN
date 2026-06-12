#ifndef HAL_AH49E_H_
#define HAL_AH49E_H_

#include "types.h"
#include "main.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "esp_log.h"

#define HAL_GPIO GPIO_NUM_10
#define MIN_HAL_MEASURMENT_INTERVAL_US 80000

/*******************************************************
 * @brief  Initializes the GPIO for the HAL sensor 
 * 
 * @details Configures specific GPIO pin as an input with a pull-down resistor
 *          and a positive edge interrupt. It also installs the global GPIO ISR
 *          service and attaches a interrupt handler to the pin.
 * 
 * @param   inputPin - the GPIO number to be configured as input.
 * @param   appData - A pointer to the AppData structure passed to the ISR handler.
 * 
 * @return  An esp_err_t value ESP_OK on success, or an error code on fail.
 *******************************************************/
esp_err_t HalInit(gpio_num_t inputPin, AppData *appData);

#endif /* HAL_AH49E_H_ */