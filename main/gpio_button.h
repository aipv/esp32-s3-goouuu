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

// Structure for the event queue
typedef struct {
    uint8_t gpio_num;
    uint32_t press_time_ms;
} button_event_t;

// Initialize all button GPIOs and start the gpio button task
esp_err_t gpio_button_init(void);

#endif // GPIO_BUTTON_H