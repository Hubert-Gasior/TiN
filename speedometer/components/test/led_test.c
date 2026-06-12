#include "unity.h"
#include "driver/gpio.h"
#include "led.h" // Twój plik nagłówkowy

// --- Zmienne środowiska testowego (Mock State) ---
static gpio_config_t mock_gpio_config_params;
static int mock_gpio_config_call_count = 0;
static esp_err_t mock_gpio_config_return_val = ESP_OK;

static gpio_num_t mock_gpio_set_level_pin = GPIO_NUM_NC;
static uint32_t mock_gpio_set_level_val = 0;
static int mock_gpio_set_level_call_count = 0;

// --- Atrapy (Mocks) funkcji z driver/gpio.h ---

esp_err_t __wrap_gpio_config(const gpio_config_t *pGPIOConfig)
{
    mock_gpio_config_call_count++;
    if (pGPIOConfig != NULL)
    {
        // Kopiujemy parametry, aby móc je zweryfikować w teście
        mock_gpio_config_params = *pGPIOConfig;
    }
    return mock_gpio_config_return_val;
}

esp_err_t __wrap_gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    mock_gpio_set_level_call_count++;
    mock_gpio_set_level_pin = gpio_num;
    mock_gpio_set_level_val = level;
    return ESP_OK;
}

// --- Konfiguracja środowiska testowego ---

void setUp(void)
{
    // Resetowanie stanu atrap przed każdym testem
    mock_gpio_config_call_count = 0;
    mock_gpio_config_return_val = ESP_OK;
    
    mock_gpio_set_level_call_count = 0;
    mock_gpio_set_level_pin = GPIO_NUM_NC;
    mock_gpio_set_level_val = 0;
}

void tearDown(void)
{
    // Sprzątanie po teście (w tym przypadku niepotrzebne)
}

// --- Przypadki testowe ---

TEST_CASE("LedInit poprawnie konfiguruje pin GPIO jako wyjscie", "[led]")
{
    gpio_num_t testPin = GPIO_NUM_2; // Przykładowy pin LED wbudowanej na ESP32

    esp_err_t ret = LedInit(testPin);

    TEST_ASSERT_EQUAL_INT(ESP_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_gpio_config_call_count);
    
    // Weryfikacja parametrów przekazanych do struktury gpio_config_t
    TEST_ASSERT_EQUAL_UINT64((1ULL << testPin), mock_gpio_config_params.pin_bit_mask);
    TEST_ASSERT_EQUAL_INT(GPIO_MODE_OUTPUT, mock_gpio_config_params.mode);
    TEST_ASSERT_EQUAL_INT(GPIO_PULLUP_DISABLE, mock_gpio_config_params.pull_up_en);
    TEST_ASSERT_EQUAL_INT(GPIO_PULLDOWN_DISABLE, mock_gpio_config_params.pull_down_en);
    TEST_ASSERT_EQUAL_INT(GPIO_INTR_DISABLE, mock_gpio_config_params.intr_type);
}

TEST_CASE("LedInit przekazuje blad jesli gpio_config zawiedzie", "[led]")
{
    // Symulacja błędu ze strony sprzętowego API
    mock_gpio_config_return_val = ESP_ERR_INVALID_ARG;

    esp_err_t ret = LedInit(GPIO_NUM_2);

    // Funkcja powinna zwrócić ten sam błąd
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, ret);
}

TEST_CASE("LedEnable ustawia stan wysoki na odpowiednim pinie", "[led]")
{
    gpio_num_t testPin = GPIO_NUM_5;

    LedEnable(testPin);

    // Sprawdzamy czy wywołano sprzętowe ustawienie stanu
    TEST_ASSERT_EQUAL_INT(1, mock_gpio_set_level_call_count);
    // Sprawdzamy czy to właściwy pin
    TEST_ASSERT_EQUAL_INT(testPin, mock_gpio_set_level_pin);
    // Sprawdzamy czy poziom logiczny to 1 (włączony)
    TEST_ASSERT_EQUAL_UINT32(1, mock_gpio_set_level_val);
}

TEST_CASE("LedDisable ustawia stan niski na odpowiednim pinie", "[led]")
{
    gpio_num_t testPin = GPIO_NUM_4;

    LedDisable(testPin);

    // Sprawdzamy czy wywołano sprzętowe ustawienie stanu
    TEST_ASSERT_EQUAL_INT(1, mock_gpio_set_level_call_count);
    // Sprawdzamy czy to właściwy pin
    TEST_ASSERT_EQUAL_INT(testPin, mock_gpio_set_level_pin);
    // Sprawdzamy czy poziom logiczny to 0 (wyłączony)
    TEST_ASSERT_EQUAL_UINT32(0, mock_gpio_set_level_val);
}