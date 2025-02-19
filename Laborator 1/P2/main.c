#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#define GPIO_OUTPUT_IO 4
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_IO)
void app_main() {
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    //io_conf.mode = GPIO_MODE_INPUT;
    //io_conf.pin_bit_mask = 1ULL<<2;


    gpio_config(&io_conf);
    int cnt = 0;
    while(1) {
        printf("cnt: %d\n", cnt++);
        if (cnt % 4 == 0)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_OUTPUT_IO, cnt % 2);
        }
        else if (cnt % 4 == 1)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_OUTPUT_IO, cnt % 2);
        }
        else if (cnt % 4 == 2)
        {
            vTaskDelay(250 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_OUTPUT_IO, cnt % 2);
        }
        else if (cnt % 4 == 3)
        {
            vTaskDelay(750 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_OUTPUT_IO, cnt % 2);
        }
    }
}