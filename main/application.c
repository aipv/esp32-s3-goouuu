#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2s_audio.h"
#include "wav_audio.h"
#include "gpio_button.h"
#include "network_socket.h"

static const char *TAG = "APPLICATION";

#define APP_WAV_HEADER_SIZE        44

static size_t count = 19200;
static char data_buffer[131072];
static char send_buffer[65580];
int32_t *pcm_data = (int32_t *)(data_buffer);
int16_t *pcm16_data = (int16_t *)(send_buffer + 44);

void application_create_wav_audio_header(int count, char *output)
{
    RiffChunk *riff = (RiffChunk *)(output);
    FmtChunk *fmt = (FmtChunk *)(output + 12);
    DataChunk *data = (DataChunk *)(output + 36);

    memcpy(riff->chunkId, "RIFF", 4);
    riff->chunkSize = count * 2 + 36;
    memcpy(riff->format, "WAVE", 4);

    memcpy(fmt->subchunk1Id, "fmt ", 4);
    fmt->subchunk1Size = 16;
    fmt->audioFormat = 1;
    fmt->numChannels = 1;
    fmt->sampleRate = 16000;
    fmt->byteRate = 32000;
    fmt->blockAlign = 2;
    fmt->bitsPerSample = 16;

    memcpy(data->subchunk2Id, "data", 4);
    data->subchunk2Size = count * 2;
}

void application_audio_data_process(int count, char *input, char *output)
{
    int32_t *iptr = (int32_t *)(input);
    int16_t *optr = (int16_t *)(output + APP_WAV_HEADER_SIZE);
    for (int i = 0; i < count; i++)
    {
        int32_t value = iptr[i] >> 12;
        optr[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
    }
}

void application_button_boot_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Boot (GPIO %d) Pressed! - Executing action A.", gpio_num);
    i2s_audio_read_data_safe(pcm_data, count);
    ESP_LOGI(TAG, "Success recorded %d samples!", count);
    i2s_audio_play_pcm24_data(pcm_data, count);
    ESP_LOGI(TAG, "Success played %d samples!", count);
}

void application_button_up_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Up (GPIO %d) Pressed! - Executing action B.", gpio_num);
    i2s_audio_play_pcm24_data(pcm_data, count);
    ESP_LOGI(TAG, "Success played %d samples!", count);
}

void application_button_down_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Down (GPIO %d) Pressed! - Executing action C.", gpio_num);
    application_audio_data_process(count, data_buffer, send_buffer);
    application_create_wav_audio_header(count, send_buffer);
    network_socket_data_publish(send_buffer, count * 2 + 44);
    ESP_LOGI(TAG, "Success sent %d bytes!", count * 2 + 44);
}

esp_err_t application_init(void)
{
    gpio_button_set_callback_func(0, application_button_boot_callback);
    gpio_button_set_callback_func(1, application_button_up_callback);
    gpio_button_set_callback_func(2, application_button_down_callback);    

    return ESP_OK;
}
