#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// 定义 BOOT 按钮连接的 GPIO 端口
#define BOOT_BUTTON_GPIO    (0)
// 定义日志标签
static const char *TAG = "BOOT_DETECTOR";

/**
 * @brief 初始化 BOOT 按钮 GPIO
 */
void button_init(void)
{
    ESP_LOGI(TAG, "初始化 BOOT 按钮 GPIO%d", BOOT_BUTTON_GPIO);

    // 配置 GPIO 为输入模式
    gpio_set_direction(BOOT_BUTTON_GPIO, GPIO_MODE_INPUT);
    
    // 启用内部上拉电阻
    // BOOT 按钮是低电平有效：未按高电平，按下低电平
    gpio_set_pull_mode(BOOT_BUTTON_GPIO, GPIO_PULLUP_ONLY);
}

/**
 * @brief 按钮检测任务
 */
void button_task(void *pvParameter)
{
    // 用于去抖的变量：记录上一次有效的按钮状态
    int last_button_state = 1; // 1 (HIGH) 表示未按下
    
    // 记录按钮按下的次数
    int press_count = 0;

    while (1) {
        // 读取当前 GPIO 状态
        int current_state = gpio_get_level(BOOT_BUTTON_GPIO);

        // 检测下降沿 (从 HIGH -> LOW)，意味着按钮被按下
        // 1. last_button_state 必须是 HIGH (未按下)
        // 2. current_state 必须是 LOW (按下)
        if (last_button_state == 1 && current_state == 0) {
            // 按钮刚被按下
            vTaskDelay(pdMS_TO_TICKS(50)); // 软件去抖 (50ms)
            
            // 再次读取状态以确认
            if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
                // 确认按钮被按下，执行事件
                press_count++;
                ESP_LOGW(TAG, "🎉 BOOT 按钮被按下! 当前按下次数: %d", press_count);
            }
        }
        
        if (last_button_state == 0 && current_state == 1) {
            ESP_LOGW(TAG, "🎉 BOOT 按钮被释放! 当前按下次数: %d", press_count);
        }

        // 更新上一次的按钮状态
        last_button_state = current_state;

        // 任务休眠，节省 CPU 资源
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

/**
 * @brief 主应用入口
 */
void app_main(void)
{
    // 1. 初始化按钮 GPIO
    button_init();

    // 2. 创建一个任务来持续检测按钮状态
    // 任务优先级可以根据实际需求调整
    xTaskCreate(button_task, "button_detect_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "应用启动成功，开始监测 BOOT 按钮...");
}