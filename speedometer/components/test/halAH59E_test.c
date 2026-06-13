#include "unity.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "halAH49E.h"

#define TEST_GPIO_PIN GPIO_NUM_4 

volatile bool HalReady = false;

void setUp(void)
{
    HalReady = false;
    gpio_uninstall_isr_service();
}

void tearDown(void)
{
    gpio_isr_handler_remove(TEST_GPIO_PIN);
    gpio_uninstall_isr_service();
    gpio_reset_pin(TEST_GPIO_PIN);
}

static void trigger_interrupt()
{
    gpio_set_level(TEST_GPIO_PIN, 1);
    esp_rom_delay_us(100); // Time for ISR to execute
    gpio_set_level(TEST_GPIO_PIN, 0);
    esp_rom_delay_us(50);
}

TEST_CASE("HalInit initializes the pin and installs the ISR service without error", "[hal]")
{
    AppData appData = {0};
    esp_err_t ret = HalInit(TEST_GPIO_PIN, &appData);
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}
 
TEST_CASE("Initialization: Calling the function again does not crash the system", "[hal][init]")
{
    AppData appData = {0};

    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));

    esp_err_t ret = HalInit(TEST_GPIO_PIN, &appData);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

TEST_CASE("Debouncing: Filtering contact bounce (intervals too short)", "[hal][debouncing]")
{
    AppData appData = { .distance = 0, .circumference = 100, .interval = 0 };
    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));
    
    gpio_set_direction(TEST_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(TEST_GPIO_PIN, 0);
    esp_rom_delay_us(100);

    trigger_interrupt();
    TEST_ASSERT_EQUAL_INT(100, appData.distance);

    for (int i = 0; i < 5; i++)
    {
        esp_rom_delay_us(MIN_HAL_MEASURMENT_INTERVAL_US / 5);
        trigger_interrupt();
    }

    // Distance should not increase because the total time was shorter than MIN_HAL_MEASURMENT_INTERVAL_US
    TEST_ASSERT_EQUAL_INT(100, appData.distance);

    // 3. Waiting for the full required interval
    esp_rom_delay_us(MIN_HAL_MEASURMENT_INTERVAL_US + 1000);
    trigger_interrupt();

    // Now the interrupt should be considered valid
    TEST_ASSERT_EQUAL_INT(200, appData.distance);
}

TEST_CASE("Distance accuracy: Incrementing by wheel circumference", "[hal][distance]")
{
    AppData appData = { .distance = 0, .circumference = 2150, .interval = 0 };
    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));
    
    gpio_set_direction(TEST_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(TEST_GPIO_PIN, 0);

    trigger_interrupt();
    
    for (int i = 0; i < 10; i++)
    {
        esp_rom_delay_us(MIN_HAL_MEASURMENT_INTERVAL_US + 500);
        trigger_interrupt();
    }

    uint32_t expectedDistance = 11 * appData.circumference;
    TEST_ASSERT_EQUAL_INT(expectedDistance, appData.distance);
}

TEST_CASE("Time interval calculation accuracy", "[hal][timing]")
{
    AppData appData = { .distance = 0, .circumference = 100, .interval = 0 };
    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));
    
    gpio_set_direction(TEST_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(TEST_GPIO_PIN, 0);

    trigger_interrupt();
    const int target_delay_us = 50000;
    
    TEST_ASSERT_GREATER_THAN(MIN_HAL_MEASURMENT_INTERVAL_US, target_delay_us);

    esp_rom_delay_us(target_delay_us);
    trigger_interrupt();

    TEST_ASSERT_INT_WITHIN(500, target_delay_us, appData.interval);
}

TEST_CASE("Triggering a massive amount of interrupts (Stress test)", "[hal][stress]")
{
    AppData appData = { .distance = 0, .circumference = 10, .interval = 0 };
    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));
    
    gpio_set_direction(TEST_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(TEST_GPIO_PIN, 0);

    trigger_interrupt();
    
    uint32_t currentDistance = appData.distance;
    HalReady = false;

    for (int i = 0; i < 10000; i++)
    {
        gpio_set_level(TEST_GPIO_PIN, 1);
        gpio_set_level(TEST_GPIO_PIN, 0);
    }
    esp_rom_delay_us(2000); 

    TEST_ASSERT_EQUAL_INT(currentDistance, appData.distance);
    TEST_ASSERT_FALSE(HalReady);
}