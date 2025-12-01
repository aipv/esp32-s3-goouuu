#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "gpio_button.h"

static const char *TAG = "MAIN";

void app_main(void)
{    
    if (gpio_button_init() != ESP_OK) {
        ESP_LOGE(TAG, "gpio_button_init()!");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}