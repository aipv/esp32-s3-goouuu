#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wav_audio.h"
#include "i2s_audio.h"
#include "gpio_button.h"
#include "wifi_station.h"
#include "application.h"

static const char *TAG = "MAIN";

static void check_esp_err(esp_err_t err, const char* msg)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s failed: %s (0x%x)", msg, esp_err_to_name(err), err);
        abort();
    }
}

void app_main(void)
{
    check_esp_err(wav_audio_init(), "wav_audio_init()");
    check_esp_err(gpio_button_init(), "gpio_button_init()");
    check_esp_err(i2s_audio_mic_init(), "i2s_audio_mic_init()");
    check_esp_err(i2s_audio_spk_init(), "i2s_audio_spk_init()");
    check_esp_err(wifi_station_init(), "wifi_station_init()");
    check_esp_err(application_init(), "application_init()");

    check_esp_err(gpio_button_start(), "gpio_button_start()");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}