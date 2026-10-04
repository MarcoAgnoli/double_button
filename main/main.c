#include <stdio.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define BUTTON1_GPIO     GPIO_NUM_4
#define BUTTON2_GPIO     GPIO_NUM_6
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

static void configure_buttons(void)
{
    gpio_config_t button1_config = {
        .pin_bit_mask = 1ULL << BUTTON1_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,    // pull-up già nel circuito
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE        // alto -> basso = pressione
    };

    gpio_config_t button2_config = {
        .pin_bit_mask = 1ULL << BUTTON2_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // pull-up non nel circuito qui
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE        // alto -> basso = pressione
    };

    ESP_ERROR_CHECK(gpio_config(&button1_config));
    ESP_ERROR_CHECK(gpio_config(&button2_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON1_GPIO, button_isr_handler, (void *)BUTTON1_GPIO));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON2_GPIO, button_isr_handler, (void *)BUTTON2_GPIO));
}

void app_main(void)
{
    configure_led();
    configure_buttons();
}