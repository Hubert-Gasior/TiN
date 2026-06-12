#ifndef LED_H
#define LED_H

#include "types.h"

#include "driver/gpio.h"
#include "esp_log.h"

/*******************************************************
 * @brief   Initializes A GPIO pin for LED control
 * 
 * @details Configures the specyfied GPIO pin as a push-pull output. Internal pull-up and pull-down resistors are disabled,
 *          and interrupts are deactivated for this pin.
 * 
 * @param   ledGpio - The GPIO number to be configured as the LED output.
 * 
 * @return  An esp_err_t value ESP_OK on success, or an error code from the gpio_config function.
 *******************************************************/
esp_err_t LedInit(gpio_num_t ledGpio);

/*******************************************************
 * @brief   Turns the LED on.
 * 
 * @details Sets the logic level of the specified GPIO pin to high
 * 
 * @param   ledGpio - The GPIO number associated with the LED.
 * 
 * @return  None
 *******************************************************/
void LedEnable(gpio_num_t ledGpio);

/*******************************************************
 * @brief   Truns the LED off.
 * 
 * @details Sets the logic level of the specified GPIO pin to low
 * 
 * @param   ledGpio - The GPIO number associated with the LED.  
 * 
 * @return  None
 *******************************************************/
void LedDisable(gpio_num_t ledGpio);

#endif /* LED_H*/