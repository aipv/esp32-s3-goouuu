#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2s_audio.h"
#include "wav_audio.h"
#include "gpio_button.h"
#include "network_socket.h"

static const char *TAG = "APPLICATION";

static size_t count = 32768;
static char data_buffer[131072];
static char send_buffer[65580];
int32_t *pcm_data = (int32_t *)(data_buffer);
int16_t *pcm16_data = (int16_t *)(send_buffer + 44);

void application_button_boot_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Boot (GPIO %d) Pressed! - Executing action A.", gpio_num);
    i2s_audio_read_data_safe(pcm_data, count);
    ESP_LOGI(TAG, "Success reading %d samples!", count);
    for (int i = 0; i < 32; i++)
        ESP_LOGW(TAG, "%08x %08x %08x %08x %08x %08x %08x %08x", pcm_data[i*8], pcm_data[i*8 + 1], pcm_data[i*8 + 2], 
            pcm_data[i*8 + 3], pcm_data[i*8 + 4], pcm_data[i*8 + 5], pcm_data[i*8 + 6], pcm_data[i*8 + 7]);
    for (int i = 4000; i < 4032; i++)
        ESP_LOGW(TAG, "%08x %08x %08x %08x %08x %08x %08x %08x", pcm_data[i*8], pcm_data[i*8 + 1], pcm_data[i*8 + 2], 
            pcm_data[i*8 + 3], pcm_data[i*8 + 4], pcm_data[i*8 + 5], pcm_data[i*8 + 6], pcm_data[i*8 + 7]);
}

void application_button_up_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Up (GPIO %d) Pressed! - Executing action B.", gpio_num);
    i2s_audio_play_pcm24_data(pcm_data, count);
    ESP_LOGI(TAG, "Success playing %d samples!", count);
    //i2s_audio_test_pcm16_data(pcm16_data, count);
    //ESP_LOGI(TAG, "Success testing %d samples!", count);
}

void application_button_down_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Down (GPIO %d) Pressed! - Executing action C.", gpio_num);
    //i2s_audio_play_pcm16_data(pcm16_data, count);
    //ESP_LOGI(TAG, "Success playing %d samples!", count);
    wav_audio_data_process(data_buffer, send_buffer);
    for (int i = 0; i < 32; i++)
        ESP_LOGW(TAG, "%d %d %d %d %d %d %d %d", pcm16_data[i*8], pcm16_data[i*8 + 1], pcm16_data[i*8 + 2], 
            pcm16_data[i*8 + 3], pcm16_data[i*8 + 4], pcm16_data[i*8 + 5], pcm16_data[i*8 + 6], pcm16_data[i*8 + 7]);
    for (int i = 4000; i < 4032; i++)
        ESP_LOGW(TAG, "%d %d %d %d %d %d %d %d", pcm16_data[i*8], pcm16_data[i*8 + 1], pcm16_data[i*8 + 2], 
            pcm16_data[i*8 + 3], pcm16_data[i*8 + 4], pcm16_data[i*8 + 5], pcm16_data[i*8 + 6], pcm16_data[i*8 + 7]);
    network_socket_data_publish(send_buffer, WAV_AUDIO_DEFAULT_FILE_SIZE);
    ESP_LOGI(TAG, "Success sending %d bytes!", WAV_AUDIO_DEFAULT_FILE_SIZE);
}

esp_err_t application_init(void)
{
    gpio_button_set_callback_func(0, application_button_boot_callback);
    gpio_button_set_callback_func(1, application_button_up_callback);
    gpio_button_set_callback_func(2, application_button_down_callback);    

    return ESP_OK;
}
