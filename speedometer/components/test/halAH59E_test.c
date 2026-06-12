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

// --- Przypadki testowe ---

TEST_CASE("HalInit inicjalizuje pin i instaluje usluge ISR bez bledu", "[hal]")
{
    AppData appData = {0};
    
    esp_err_t ret = HalInit(TEST_GPIO_PIN, &appData);
    
    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
}

TEST_CASE("HalISR aktualizuje AppData i poprawnie dziala debouncing", "[hal]")
{
    // 1. Przygotowanie struktury
    AppData appData = {
        .distance = 0,
        .circumference = 200, // Przykładowy obwód koła w mm
        .interval = 0
    };
    
    // Inicjalizacja Twoją funkcją
    TEST_ASSERT_EQUAL_INT(ESP_OK, HalInit(TEST_GPIO_PIN, &appData));
    
    // Trik testowy: rekonfigurujemy pin na INPUT_OUTPUT, aby móc nim sterować z kodu
    gpio_set_direction(TEST_GPIO_PIN, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(TEST_GPIO_PIN, 0);
    esp_rom_delay_us(100); // Chwila na stabilizację stanu
    
    // 2. Pierwsze wyzwolenie przerwania (zbocze narastające)
    gpio_set_level(TEST_GPIO_PIN, 1);
    esp_rom_delay_us(100); // Czekamy, aż ISR się wykona
    
    // Sprawdzamy, czy przerwanie zadziałało poprawnie
    TEST_ASSERT_TRUE(HalReady);
    TEST_ASSERT_EQUAL_INT(200, appData.distance); // Dystans wzrósł o obwód
    
    // Resetujemy flagę do kolejnego kroku
    HalReady = false;
    gpio_set_level(TEST_GPIO_PIN, 0);
    
    // 3. Test debouncingu (wyzwalamy przerwanie ZBYT SZYBKO)
    // Zakładamy czas mniejszy niż MIN_HAL_MEASURMENT_INTERVAL_US
    esp_rom_delay_us(100); 
    gpio_set_level(TEST_GPIO_PIN, 1);
    esp_rom_delay_us(100); // Czekamy na potencjalne wykonanie ISR
    
    // Flaga powinna pozostać false, a dystans się nie zmienić
    TEST_ASSERT_FALSE(HalReady);
    TEST_ASSERT_EQUAL_INT(200, appData.distance); 
    
    // 4. Test ponownego wyzwolenia po poprawnym czasie
    gpio_set_level(TEST_GPIO_PIN, 0);
    // Symulujemy upływ wymaganego czasu
    esp_rom_delay_us(MIN_HAL_MEASURMENT_INTERVAL_US + 1000); 
    
    // Ponowne wyzwolenie
    gpio_set_level(TEST_GPIO_PIN, 1);
    esp_rom_delay_us(100);
    
    // Tym razem powinno zarejestrować obrót
    TEST_ASSERT_TRUE(HalReady);
    TEST_ASSERT_EQUAL_INT(400, appData.distance); // Dystans ponownie wzrósł
}