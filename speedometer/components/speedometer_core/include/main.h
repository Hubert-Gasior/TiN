#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "esp_log.h"
#include "esp_timer.h"

#define SEND_BUFFER_SIZE 256
#define MEASURMENT_INTERVAL_MS 1000
#define ONE_SECOND_INTERVAL_MS 1000
#define HAL_TIMEOUT_US 2630000
#define DIAMETER_M 0.7112f
#define M_PER_US_TO_KM_PER_H_CONVERSION_CONSTANT 3600000.0f
#define ERROR_LED GPIO_NUM_0
#define MEASURMENT_LED GPIO_NUM_1
#define CONNECTION_LED GPIO_NUM_2

typedef struct 
{
    float32 circumference;
    float32 pitch;
    volatile float32 distance;
    volatile int64 interval;
} AppData;

#endif /* MAIN_H */