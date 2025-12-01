#ifndef GPIO_BUTTON_H
#define GPIO_BUTTON_H

#include "esp_err.h"
#include "driver/gpio.h"

// Define the 5 GPIOs where your buttons are connected
#define GPIO_BUTTON_0 GPIO_NUM_0
#define GPIO_BUTTON_1 GPIO_NUM_38
#define GPIO_BUTTON_2 GPIO_NUM_39

// The buttons are active-low (connected to GND, pulled high by internal resistor)
#define BUTTON_PRESSED 0

typedef void (*button_callback_t)(uint8_t gpio_num);

// Initialize all button GPIOs and start the gpio button task
esp_err_t gpio_button_init(void);
esp_err_t gpio_button_start(void);
esp_err_t gpio_button_set_callback_func(void);

#endif // GPIO_BUTTON_H