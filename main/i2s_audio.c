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
    i2s_chan_config_t chan_cfg = {
        .id = (i2s_port_t)0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 8,
        .dma_frame_num = 256,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    check_esp_err(i2s_new_channel(&chan_cfg, NULL, &rx_handle), "i2s_new_channel_rx");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)I2S_AUDIO_MIC_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            #ifdef   I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
            #endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_MIC_GPIO_SCK,
            .ws   = I2S_AUDIO_MIC_GPIO_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_AUDIO_MIC_GPIO_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
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
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)I2S_AUDIO_SPK_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            #ifdef   I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
            #endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_AUDIO_SPK_GPIO_BCLK,
            .ws = I2S_AUDIO_SPK_GPIO_LRCK,
            .dout = I2S_AUDIO_SPK_GPIO_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
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
    for (int i = 0; i < 32; i++)
        ESP_LOGW(TAG, "%d %d %d %d %d %d %d %d", buffer[i*8], buffer[i*8 + 1], buffer[i*8 + 2], 
            buffer[i*8 + 3], buffer[i*8 + 4], buffer[i*8 + 5], buffer[i*8 + 6], buffer[i*8 + 7]);

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

#define I2S_READ_CHUNK_SAMPLES 1024 // 每次读取 1024 个样本 (4KB)

/**
 * @brief 安全地循环读取指定总数的 I2S 数据样本。
 * * @param buffer 指向已分配好完整内存（例如 256KB）的目标缓冲区。
 * @param total_samples 需要读取的样本总数（例如 65536）。
 * @return esp_err_t 
 */
esp_err_t i2s_audio_read_data_safe(int32_t *buffer, int total_samples)
{
    esp_err_t ret = ESP_OK;
    size_t total_bytes_to_read = (size_t)total_samples * sizeof(int32_t);
    size_t total_bytes_read = 0;
    int32_t *current_ptr = buffer; // 当前写入位置

    // 1. 在读取开始时启用 I2S 通道
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable_rx failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. 循环读取，直到达到总目标字节数
    while (total_bytes_read < total_bytes_to_read) {
        size_t bytes_left = total_bytes_to_read - total_bytes_read;
        
        // 确定本次需要读取的字节数（取剩余字节数和 CHUNK_SIZE 中的较小值）
        size_t bytes_to_read_this_chunk = (bytes_left > I2S_READ_CHUNK_SAMPLES * sizeof(int32_t)) 
                                         ? I2S_READ_CHUNK_SAMPLES * sizeof(int32_t) 
                                         : bytes_left;
        size_t bytes_read_this_chunk = 0;

        // 设置一个较短的超时时间（例如 500ms），防止长时间阻塞
        ret = i2s_channel_read(
            rx_handle, 
            current_ptr, // 写入当前位置
            bytes_to_read_this_chunk, 
            &bytes_read_this_chunk, 
            pdMS_TO_TICKS(500)
        );
        
        // 检查读取错误
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "i2s_channel_read failed at byte %u: %s", total_bytes_read, esp_err_to_name(ret));
            goto cleanup;
        }

        // 检查是否读到预期字节数（超时可能导致读到的字节数少于请求）
        if (bytes_read_this_chunk < bytes_to_read_this_chunk) {
            ESP_LOGW(TAG, "Incomplete read: expected %u, got %u. Stopping read.", bytes_to_read_this_chunk, bytes_read_this_chunk);
            total_bytes_read += bytes_read_this_chunk;
            break; // 停止读取
        }

        // 更新总进度和写入指针
        total_bytes_read += bytes_read_this_chunk;
        current_ptr += (bytes_read_this_chunk / sizeof(int32_t)); 
    }

cleanup:
    // 3. 在读取结束后禁用 I2S 通道
    i2s_channel_disable(rx_handle);

    // 4. 最终检查是否读取完整
    if (total_bytes_read != total_bytes_to_read) {
        ESP_LOGW(TAG, "Final read incomplete: expected %u bytes, got %u bytes", total_bytes_to_read, total_bytes_read);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}