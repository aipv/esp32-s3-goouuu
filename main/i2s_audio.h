#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdio.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2S_AUDIO_MIC_GPIO_WS       GPIO_NUM_4
#define I2S_AUDIO_MIC_GPIO_SCK      GPIO_NUM_5
#define I2S_AUDIO_MIC_GPIO_DIN      GPIO_NUM_6
#define I2S_AUDIO_MIC_BIT_WIDTH     I2S_DATA_BIT_WIDTH_16BIT
#define I2S_AUDIO_MIC_SLOT_MODE     I2S_SLOT_MODE_STEREO
#define I2S_AUDIO_MIC_SAMPLE_RATE   16000

#define I2S_AUDIO_SPK_GPIO_DOUT     GPIO_NUM_7
#define I2S_AUDIO_SPK_GPIO_BCLK     GPIO_NUM_15
#define I2S_AUDIO_SPK_GPIO_LRCK     GPIO_NUM_16
#define I2S_AUDIO_SPK_BIT_WIDTH     I2S_DATA_BIT_WIDTH_16BIT
#define I2S_AUDIO_SPK_SLOT_MODE     I2S_SLOT_MODE_STEREO
#define I2S_AUDIO_SPK_SAMPLE_RATE   16000

esp_err_t i2s_audio_mic_init(void);
esp_err_t i2s_audio_spk_init(void);

esp_err_t i2s_audio_read_pcm16_data(int16_t *buffer, int samples);
esp_err_t i2s_audio_play_pcm16_data(int16_t *buffer, int samples);
esp_err_t i2s_audio_dual_pcm16_data(int16_t *buffer, int samples);
esp_err_t i2s_audio_test_pcm16_data(int16_t *buffer, int samples);

esp_err_t i2s_audio_read_pcm24_data(int32_t *buffer, int samples);
esp_err_t i2s_audio_play_pcm24_data(int32_t *buffer, int samples);
esp_err_t i2s_audio_dual_pcm24_data(int32_t *buffer, int samples);
esp_err_t i2s_audio_test_pcm24_data(int32_t *buffer, int samples);

esp_err_t i2s_audio_read_data_safe(int32_t *buffer, int total_samples);

#endif // I2S_AUDIO_H