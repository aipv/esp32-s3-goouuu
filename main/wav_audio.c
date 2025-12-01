#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wav_audio.h"

static const char *TAG = "WAV_AUDIO";

static char default_header[WAV_AUDIO_HEADER_SIZE];

esp_err_t wav_audio_init(void)
{
    RiffChunk *riff = (RiffChunk *)(default_header);
    FmtChunk *fmt = (FmtChunk *)(default_header + 12);
    DataChunk *data = (DataChunk *)(default_header + 36);

    memcpy(riff->chunkId, "RIFF", 4);
    riff->chunkSize = WAV_AUDIO_DEFAULT_TRUNK_SIZE;
    memcpy(riff->format, "WAVE", 4);

    memcpy(fmt->subchunk1Id, "fmt ", 4);
    fmt->subchunk1Size = 16;
    fmt->audioFormat = 1;
    fmt->numChannels = WAV_AUDIO_NUM_OF_CHANNELS;
    fmt->sampleRate = WAV_AUDIO_SAMPLE_RATE;
    fmt->byteRate = WAV_PCM16_BYTE_RATE;
    fmt->blockAlign = WAV_PCM16_BLOCK_ALIGN;
    fmt->bitsPerSample = WAV_PCM16_BITS_PER_SAMPLE;

    memcpy(data->subchunk2Id, "data", 4);
    data->subchunk2Size = WAV_AUDIO_DEFAULT_DATA_SIZE;
    ESP_LOGI(TAG, "wav_audio_init() Success!");
    return ESP_OK;
}

esp_err_t wav_audio_default_header(char *buffer)
{
    memcpy(buffer, default_header, WAV_AUDIO_HEADER_SIZE);
    return ESP_OK;
}

esp_err_t wav_audio_data_process(char *input, char *output)
{
    int32_t *iptr = (int32_t *)(input);
    int16_t *optr = (int16_t *)(output + WAV_AUDIO_HEADER_SIZE);
    for (int i = 0; i < WAV_AUDIO_DEFAULT_SAMPLE; i++)
    {
        int32_t value = iptr[i] >> 12;
        optr[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
    }
    memcpy(output, default_header, WAV_AUDIO_HEADER_SIZE);
    return ESP_OK;
}
