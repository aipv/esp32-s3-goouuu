#include <stdio.h>
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2s_audio.h"

static const char *TAG = "I2S_AUDIO";

static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

static int32_t rx_buffer[I2S_AUDIO_SAMPLE_COUNT];
static int16_t pcm16_buf[I2S_AUDIO_SAMPLE_COUNT];

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
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_MIC_GPIO_SCK,
            .ws   = I2S_AUDIO_MIC_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_AUDIO_MIC_GPIO_DIN,
        },
    };
    check_esp_err(i2s_channel_init_std_mode(rx_handle, &std_cfg), "i2s_channel_init_std_mode_rx");
    //check_esp_err(i2s_channel_enable(rx_handle), "i2s_channel_enable_rx");
    ESP_LOGI(TAG, "i2s_audio_mic_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_spk_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    check_esp_err(i2s_new_channel(&chan_cfg, &tx_handle, NULL), "i2s_new_channel_tx");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_AUDIO_SPK_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SPK_GPIO_BCLK, // GPIO15
            .ws = I2S_AUDIO_SPK_GPIO_LRCK,   // GPIO16
            .dout = I2S_AUDIO_SPK_GPIO_DOUT, // GPIO7
            .din = I2S_GPIO_UNUSED,
        },
    };

    check_esp_err(i2s_channel_init_std_mode(tx_handle, &std_cfg), "i2s_channel_init_std_mode_tx");
    //check_esp_err(i2s_channel_enable(tx_handle), "i2s_channel_enable_tx");
    ESP_LOGI(TAG, "i2s_audio_spk_init() Success!");
    return ESP_OK;
}

esp_err_t i2s_audio_read_test_data()
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;

    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Enable failed: %d", ret);
    }

    ret = i2s_channel_read(rx_handle, rx_buffer, sizeof(rx_buffer), &bytes_read, pdMS_TO_TICKS(1000));
    int total_samples = bytes_read / 4;
    ESP_LOGI(TAG, "i2s_channel_read() : %d, %d, %d, %d!", sizeof(rx_buffer), bytes_read, total_samples, ret);

    for (int i = 0; i < total_samples; i++)
    {
        pcm16_buf[i] = rx_buffer[i*2] >> 8; // 24bit -> 16bit
    }

    ret = i2s_channel_disable(rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Disable failed: %d", ret);
    }

    ESP_LOGI(TAG, "%ld %ld %ld %ld %ld %ld %ld %ld", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3],
        rx_buffer[4], rx_buffer[5], rx_buffer[6], rx_buffer[7]);
    ESP_LOGI(TAG, "%d %d %d %d %d %d %d %d", pcm16_buf[0], pcm16_buf[1], pcm16_buf[2], pcm16_buf[3],
        pcm16_buf[4], pcm16_buf[5], pcm16_buf[6], pcm16_buf[7]);
    return ESP_OK;
}

esp_err_t i2s_audio_read_pcm24_data(int32_t *buffer, int samples)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_to_read = samples * 4;

    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Enable failed: %d", ret);
    }

    ret = i2s_channel_read(rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed at block, error %d", ret);
        return ret;
    }
    if (bytes_read != bytes_to_read) {
        ESP_LOGW(TAG, "Read partial data: expected %u bytes, got %u bytes", bytes_to_read, bytes_read);
        return ESP_FAIL; 
    }

    ret = i2s_channel_disable(rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Disable failed: %d", ret);
    }
    return ESP_OK;
}

esp_err_t i2s_audio_play_pcm24_data(int32_t *buffer, int samples)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_written = 0;
    size_t size_bytes = (size_t)samples * sizeof(int32_t);

    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Enable failed: %d", ret);
    }

    ESP_LOGI(TAG, "Input %d, %d", samples, size_bytes);
    ret = i2s_channel_write(tx_handle, (const void *)buffer, size_bytes, &bytes_written, pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Output %d, %d", bytes_written, ret);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S write failed with error: %d", ret);
        return ret;
    }
    if (bytes_written != size_bytes)
    {
        ESP_LOGW(TAG, "Partial write warning: Wrote %u bytes, expected %u bytes.", 
                 (unsigned int)bytes_written, (unsigned int)size_bytes);
        return ESP_FAIL; 
    }

    ret = i2s_channel_disable(tx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2S Disable failed: %d", ret);
    }
    return ESP_OK;
}
