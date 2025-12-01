#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2s_audio.h"
#include "gpio_button.h"

static const char *TAG = "APPLICATION";

static int16_t buffer[65536];
static size_t count = 65536;

void application_button_boot_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Boot (GPIO %d) Pressed! - Executing action A.", gpio_num);
    i2s_audio_read_pcm16_data(buffer, count);
    ESP_LOGI(TAG, "Success reading %d samples!", count);
}

void application_button_up_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Up (GPIO %d) Pressed! - Executing action B.", gpio_num);
    i2s_audio_test_pcm16_data(buffer, count);
    ESP_LOGI(TAG, "Success testing %d samples!", count);
}

void application_button_down_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Down (GPIO %d) Pressed! - Executing action C.", gpio_num);
    i2s_audio_play_pcm16_data(buffer, count);
    ESP_LOGI(TAG, "Success playing %d samples!", count);
}

esp_err_t application_init(void)
{
    gpio_button_set_callback_func(0, application_button_boot_callback);
    gpio_button_set_callback_func(1, application_button_up_callback);
    gpio_button_set_callback_func(2, application_button_down_callback);    

    return ESP_OK;
}
