#include "pico/stdlib.h"

#include "FreeRTOS.h"

#include "task.h"

const uint led_pin_blue = 11;
const uint led_pin_red = 12;

void setup(void)
{
    gpio_init(led_pin_red);
    gpio_set_dir(led_pin_red, GPIO_OUT);
    gpio_init(led_pin_blue);
    gpio_set_dir(led_pin_blue, GPIO_OUT);
}

void vBlinkTask(void *pvParamters)
{

    uint *led = (uint *)pvParamters;
    for (;;)
    {

        gpio_put(*led, 1);

        vTaskDelay(250 / portTICK_PERIOD_MS);

        gpio_put(*led, 0);

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void main(void)
{

    setup();

    xTaskCreate(vBlinkTask, "Blink Task", 128, (void *)&led_pin_red, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink Task 2", 128, (void *)&led_pin_blue, 1, NULL);

    vTaskStartScheduler();
    for (;;)
        ;
}