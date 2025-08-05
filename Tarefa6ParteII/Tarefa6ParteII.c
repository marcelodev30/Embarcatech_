#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

//pinos e módulos controlador i2c selecionado
#define I2C_PORT    i2c1
#define PINO_SCL    14
#define PINO_SDA    15

//botão do Joystick
const int SW = 22;

// Pinos dos LEDs RGB e do buzzer
const uint BLUE_LED_PIN  = 12;   // LED azul no GPIO 12
const uint RED_LED_PIN   = 13;   // LED vermelho no GPIO 13
const uint GREEN_LED_PIN = 11;   // LED verde no GPIO 11
const uint BUZZER_PIN    = 10;   // Pino do buzzer

uint pos_y = 18;  
ssd1306_t disp;

// Configura o PWM para um LED específico
void setup_pwm_led(uint led, uint *slice, uint16_t level) {
    gpio_set_function(led, GPIO_FUNC_PWM);          
    *slice = pwm_gpio_to_slice_num(led);              
    pwm_set_clkdiv(*slice, 16.0f);                      
    pwm_set_wrap(*slice, 4096);                        
    pwm_set_gpio_level(led, level);                   
    pwm_set_enabled(*slice, true);                   
}

//função para inicialização de todos os recursos do sistema
void inicializa(void) {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26); 
    adc_gpio_init(27);  
    i2c_init(I2C_PORT, 400 * 1000);  // 400 kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);

    // Inicializa os pinos dos LEDs RGB
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);

    // Inicializa o buzzer com PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // Botão do Joystick
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
}

// Função de botão pressionado
bool check_button_press(void) {
    static absolute_time_t last_press_time = 0;
    if (!gpio_get(SW)) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_press_time, now) > 200000) {
            last_press_time = now;
            return true;
        }
    }
    return false;
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(BUZZER_PIN, top / 2);  
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0);     
    sleep_ms(50);                             
}

// Exibe um texto no display OLED
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale) {
    ssd1306_draw_string(&disp, pos_x, pos_y, scale, msg);
    ssd1306_show(&disp);
}

// Desenha um retângulo vazio no display.
void print_retangulo(int x1, int y1, int x2, int y2) {
    ssd1306_draw_empty_square(&disp, x1, y1, x2, y2);
    ssd1306_show(&disp);
}

// Joystick LED
void rodar_programa_joystick_led(void) {
    uint slice_blue, slice_red;
    setup_pwm_led(BLUE_LED_PIN, &slice_blue, 100);
    setup_pwm_led(RED_LED_PIN, &slice_red, 100);
    bool exit_program = false;

    while (!exit_program) {
        // Leitura ADC canal 0
        adc_select_input(0);
        sleep_us(2);
        uint16_t vrx_value = adc_read();

        // Leitura do ADC canal 1
        adc_select_input(1);
        sleep_us(2);
        uint16_t vry_value = adc_read();

        pwm_set_gpio_level(BLUE_LED_PIN, vrx_value);
        pwm_set_gpio_level(RED_LED_PIN, vry_value);

        if (check_button_press()) {
            exit_program = true;
            pwm_set_gpio_level(BLUE_LED_PIN, 0);
            pwm_set_gpio_level(RED_LED_PIN, 0);
        }
        sleep_ms(100);
    }
}

// Tocar Buzzer toca uma sequência de notas 
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};

void tocar_buzzer(void) {
    size_t num_notes = sizeof(star_wars_notes) / sizeof(star_wars_notes[0]);
    for (size_t i = 0; i < num_notes; i++) {
        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
        } else {
            play_tone(star_wars_notes[i], note_duration[i]);
        }
        // Permite sair se o botão for pressionado
        if (!gpio_get(SW)) {
            break;
        }
    }
}

// LED RGB 
void ligar_led_rgb(void) {
    uint slice_r, slice_g, slice_b;
    setup_pwm_led(RED_LED_PIN, &slice_r, 0);
    setup_pwm_led(GREEN_LED_PIN, &slice_g, 0);
    setup_pwm_led(BLUE_LED_PIN, &slice_b, 0);

    uint16_t counter = 0;
    while (true) {
        uint16_t r = (uint16_t)((sin(counter * 0.02) + 1.0) * 2047.5);
        uint16_t g = (uint16_t)((sin(counter * 0.02 + 2.094) + 1.0) * 2047.5);
        uint16_t b = (uint16_t)((sin(counter * 0.02 + 4.188) + 1.0) * 2047.5);

        pwm_set_gpio_level(RED_LED_PIN, r);
        pwm_set_gpio_level(GREEN_LED_PIN, g);
        pwm_set_gpio_level(BLUE_LED_PIN, b);

        if (check_button_press()) {
            break;
        }
        counter++;
        sleep_ms(20);
    }
    pwm_set_gpio_level(RED_LED_PIN, 0);
    pwm_set_gpio_level(GREEN_LED_PIN, 0);
    pwm_set_gpio_level(BLUE_LED_PIN, 0);
}


// Função Principal
int main() {
    inicializa();
    pos_y = 18;  

    while (true) {
         // Texto do Menu
        ssd1306_clear(&disp);
        print_texto("Menu", 52, 2, 1);
        print_retangulo(2, pos_y - 2, 120, 12);
        print_texto("Joystick LED", 6, 18, 1.5);
        print_texto("Tocar Buzzer", 6, 30, 1.5);
        print_texto("LED RGB", 6, 42, 1.5);
        
        adc_select_input(0);
        sleep_us(2);
        uint16_t adc_val = adc_read();
        const uint16_t lower_threshold = 1000;
        const uint16_t upper_threshold = 3000;
        
        // Se o joystick for movido para baixo e ainda não estiver no último item
        if (adc_val > upper_threshold && pos_y > 18) {
            pos_y -= 12;
            sleep_ms(300); 
        }
        // Se movido para cima e não estiver no primeiro item
        else if (adc_val < lower_threshold && pos_y < 42) {
            pos_y += 12;
            sleep_ms(300);
        }

        if (check_button_press()) {
            ssd1306_clear(&disp);
            if (pos_y == 18) {
                rodar_programa_joystick_led();
            } else if (pos_y == 30) {
                tocar_buzzer();
            } else if (pos_y == 42) {
                ligar_led_rgb();
            }
            sleep_ms(500);
        }    
        sleep_ms(100);
    }
    
    return 0;
}
