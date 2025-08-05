#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define LEDR 12
#define LEDG 11
#define LEDB 13

#define PWM_STEP (1 << 8)

int main() {
    stdio_init_all();

    uint slice_r, slice_g, slice_b;
    uint16_t pwm_r = 0xFF00, pwm_g = 0, pwm_b = 0;
    uint8_t state = 0;

    gpio_set_function(LEDR, GPIO_FUNC_PWM);
    gpio_set_function(LEDG, GPIO_FUNC_PWM);
    gpio_set_function(LEDB, GPIO_FUNC_PWM);

    slice_r = pwm_gpio_to_slice_num(LEDR);
    slice_g = pwm_gpio_to_slice_num(LEDG);
    slice_b = pwm_gpio_to_slice_num(LEDB);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_r, &config, true);
    pwm_init(slice_g, &config, true);
    pwm_init(slice_b, &config, true);

    pwm_set_gpio_level(LEDR, pwm_r);
    pwm_set_gpio_level(LEDG, pwm_g);
    pwm_set_gpio_level(LEDB, pwm_b);

    while (true) {
        switch (state) {
            case 0:
                pwm_r -= PWM_STEP;
                pwm_g += PWM_STEP;
                pwm_set_gpio_level(LEDR, pwm_r);
                pwm_set_gpio_level(LEDG, pwm_g);
                if (!pwm_r)
                    state = 1;
                break;
            case 1:
                pwm_g -= PWM_STEP;
                pwm_b += PWM_STEP;
                pwm_set_gpio_level(LEDG, pwm_g);
                pwm_set_gpio_level(LEDB, pwm_b);
                if (!pwm_g)
                    state = 2;
                break;
            case 2:
                pwm_b -= PWM_STEP;
                pwm_r += PWM_STEP;
                pwm_set_gpio_level(LEDB, pwm_b);
                pwm_set_gpio_level(LEDR, pwm_r);
                if (!pwm_b)
                    state = 0;
                break;
        }
        sleep_ms(5);
    }
}
