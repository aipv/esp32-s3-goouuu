#include <stdio.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2s_audio.h"

static const char *TAG = "I2S_AUDIO";

static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

// ================== 错误检查 ==================
static void check_esp_err(esp_err_t err, const char* msg)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s failed: %s (0x%x)", msg, esp_err_to_name(err), err);
        abort();
    }
}

esp_err_t i2s_audio_mic_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    check_esp_err(i2s_new_channel(&chan_cfg, NULL, &rx_handle), "i2s_new_channel_rx");

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(I2S_AUDIO_MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_AUDIO_MIC_BIT_WIDTH, I2S_AUDIO_MIC_SLOT_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_MIC_GPIO_SCK,
            .ws   = I2S_AUDIO_MIC_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_AUDIO_MIC_GPIO_DIN,
        },
    };
    check_esp_err(i2s_channel_init_std_mode(rx_handle, &std_cfg), "i2s_channel_init_std_mode_rx");
    ESP_LOGI(TAG, "i2s_audio_mic_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_spk_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    check_esp_err(i2s_new_channel(&chan_cfg, &tx_handle, NULL), "i2s_new_channel_tx");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_AUDIO_SPK_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_AUDIO_SPK_BIT_WIDTH, I2S_AUDIO_SPK_SLOT_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SPK_GPIO_BCLK,
            .ws = I2S_AUDIO_SPK_GPIO_LRCK,
            .dout = I2S_AUDIO_SPK_GPIO_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };

    check_esp_err(i2s_channel_init_std_mode(tx_handle, &std_cfg), "i2s_channel_init_std_mode_tx");
    ESP_LOGI(TAG, "i2s_audio_spk_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_read_pcm16_data(int16_t *buffer, int samples)
{
    size_t bytes_read = 0;
    size_t bytes_to_read = (size_t)samples * sizeof(int16_t);

    check_esp_err(i2s_channel_enable(rx_handle), "i2s_channel_enable_rx");
    check_esp_err(i2s_channel_read(rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(1000)), "i2s_channel_read");
    if (bytes_read != bytes_to_read)
    {
        ESP_LOGW(TAG, "Read data: expected %u bytes, got %u bytes", bytes_to_read, bytes_read);
        return ESP_FAIL;
    }
    check_esp_err(i2s_channel_disable(rx_handle), "i2s_channel_disable_rx");
    return ESP_OK;
}

esp_err_t i2s_audio_play_pcm16_data(int16_t *buffer, int samples)
{
    size_t bytes_written = 0;
    size_t size_bytes = (size_t)samples * sizeof(int16_t);

    check_esp_err(i2s_channel_enable(tx_handle), "i2s_channel_enable_tx");
    check_esp_err(i2s_channel_write(tx_handle, (const void *)buffer, size_bytes, &bytes_written, pdMS_TO_TICKS(1000)), "i2s_channel_write");
    if (bytes_written != size_bytes)
    {
        ESP_LOGW(TAG, "Write data: Wrote %u bytes, expected %u bytes.", bytes_written, size_bytes);
        return ESP_FAIL;
    }
    check_esp_err(i2s_channel_disable(tx_handle), "i2s_channel_disable_tx");
    return ESP_OK;
}

esp_err_t i2s_audio_dual_pcm16_data(int16_t *buffer, int samples)
{
    for (int i = 0; i < samples; i+=2)
    {
        buffer[i+1] = buffer[i];
    }
    return ESP_OK;
}

esp_err_t i2s_audio_test_pcm16_data(int16_t *buffer, int samples)
{
    ESP_LOGI(TAG, "Recording........");
    i2s_audio_read_pcm16_data(buffer, samples);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Processing........");
    i2s_audio_dual_pcm16_data(buffer, samples);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Palying........");
    i2s_audio_play_pcm16_data(buffer, samples);
    return ESP_OK;
}


esp_err_t i2s_audio_read_pcm24_data(int32_t *buffer, int samples)
{
    size_t bytes_read = 0;
    size_t bytes_to_read = (size_t)samples * sizeof(int32_t);

    check_esp_err(i2s_channel_enable(rx_handle), "i2s_channel_enable_rx");
    check_esp_err(i2s_channel_read(rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(1000)), "i2s_channel_read");
    if (bytes_read != bytes_to_read)
    {
        ESP_LOGW(TAG, "Read data: expected %u bytes, got %u bytes", bytes_to_read, bytes_read);
        return ESP_FAIL;
    }
    check_esp_err(i2s_channel_disable(rx_handle), "i2s_channel_disable_rx");
    return ESP_OK;
}

esp_err_t i2s_audio_play_pcm24_data(int32_t *buffer, int samples)
{
    size_t bytes_written = 0;
    size_t size_bytes = (size_t)samples * sizeof(int32_t);

    check_esp_err(i2s_channel_enable(tx_handle), "i2s_channel_enable_tx");
    check_esp_err(i2s_channel_write(tx_handle, (const void *)buffer, size_bytes, &bytes_written, pdMS_TO_TICKS(1000)), "i2s_channel_write");
    if (bytes_written != size_bytes)
    {
        ESP_LOGW(TAG, "Write data: Wrote %u bytes, expected %u bytes.", bytes_written, size_bytes);
        return ESP_FAIL;
    }
    check_esp_err(i2s_channel_disable(tx_handle), "i2s_channel_disable_tx");
    return ESP_OK;
}

esp_err_t i2s_audio_dual_pcm24_data(int32_t *buffer, int samples)
{
    for (int i = 0; i < samples; i+=2)
    {
        buffer[i+1] = buffer[i];
    }
    return ESP_OK;
}

esp_err_t i2s_audio_test_pcm24_data(int32_t *buffer, int samples)
{
    ESP_LOGI(TAG, "Recording........");
    i2s_audio_read_pcm24_data(buffer, samples);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Processing........");
    i2s_audio_dual_pcm24_data(buffer, samples);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Palying........");
    i2s_audio_play_pcm24_data(buffer, samples);
    return ESP_OK;
}
