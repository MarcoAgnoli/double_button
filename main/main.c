#include <stdio.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUTTON_GPIO     GPIO_NUM_4
#define LED_GPIO        GPIO_NUM_5
#define DEBOUNCE_US     200000   // 200 ms

static volatile bool status = false;
static volatile int64_t last_press_us = 0;

static void IRAM_ATTR manage_led(void)
{
    status = !status;
    gpio_set_level(LED_GPIO, status);
}

static void IRAM_ATTR button_isr_handler(void *argument)
{
    int64_t now = esp_timer_get_time();
    if (now - last_press_us > DEBOUNCE_US) {
        last_press_us = now;
        manage_led();
    }
}

static void configure_led(void)
{
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led_config));
    gpio_set_level(LED_GPIO, 0);   // parte spento
}

static void configure_button(void)
{
    gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,    // pull-up già nel circuito
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE        // alto -> basso = pressione
    };
    ESP_ERROR_CHECK(gpio_config(&button_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL));
}

void app_main(void)
{
    configure_led();
    configure_button();
}