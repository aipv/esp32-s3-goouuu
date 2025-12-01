idf_component_register(
    SRCS "main.c" "gpio_button.c" "i2s_audio.c" "wav_audio.c" "wifi_station.c" "network_socket.c" "application.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        freertos             # 操作系统内核
        esp_event            # ESP 事件处理（Wi-Fi、TCP/IP 等都需要）
        nvs_flash            # 非易失性存储（Wi-Fi 配置需要）
        esp_wifi             # Wi-Fi 驱动本身
        esp_netif            # 网络接口（用于配置 STA/AP）
        lwip                 # TCP/IP 协议栈（负责IP地址、Socket等）
        esp_driver_gpio      # GPIO 驱动 (gpio_button.c 需要)
        esp_driver_i2s       # I2S 驱动 (i2s_audio.c 需要)
        esp_timer
)