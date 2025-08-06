#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

//pinos e módulos controlador i2c selecionado
#define I2C_PORT i2c1
#define PINO_SCL 14
#define PINO_SDA 15

//botão do Joystick
const int SW = 22;  

//definição dos LEDs RGB
const uint BLUE_LED_PIN = 12;   // LED azul no GPIO 12
const uint RED_LED_PIN  = 13;   // LED vermelho no GPIO 13
const uint GREEN_LED_PIN = 11;  // LED verde no GPIO 11
const uint BUZZER_PIN = 10;     // Pino do buzzer

//variável para armazenar a posição do seletor do display
uint pos_y = 12;

ssd1306_t disp;
uint16_t led_b_level, led_r_level = 100; // Inicialização dos níveis de PWM para os LEDs
uint slice_led_b, slice_led_r;           // Variáveis para armazenar os slices de PWM correspondentes aos LEDs
// Função para configurar o PWM de um LED 
void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
  gpio_set_function(led, GPIO_FUNC_PWM); // Configura o pino do LED como saída PWM
  *slice = pwm_gpio_to_slice_num(led);   // Obtém o slice do PWM associado ao pino do LED
  pwm_set_clkdiv(*slice, 16.0);   // Define o divisor de clock do PWM
  pwm_set_wrap(*slice, 4096);          // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(led, level);        // Define o nível inicial do PWM para o LED
  pwm_set_enabled(*slice, true);         // Habilita o PWM no slice correspondente ao LED
}
//função para inicialização de todos os recursos do sistema
void inicializa(){
    stdio_init_all();
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    i2c_init(I2C_PORT, 400 * 1000);  // I2C Inicialização. Usando 400Khz.
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);

    // Inicialização dos LEDs RGB
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);

    // Inicialmente, desligar o LED RGB
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);

    // Inicializar o buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o PWM inicialmente

    // Botão do Joystick
    gpio_init(SW);             // Inicializa o pino do botão
    gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(SW);
    
   

  
}

bool check_button_press() {
    static absolute_time_t last_press_time = 0;
    if (!gpio_get(SW)) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_press_time, now) > 200000) {  // Debounce de 200ms
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
    pwm_set_gpio_level(BUZZER_PIN, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

// Notas musicais para a música tema de Star Wars
const uint star_wars_notes[] = {
    440, 440, 440, 349, 523, 440, 349, 523, 440, 0,  // Primeira parte
    659, 659, 659, 698, 523, 415, 349, 523, 440, 0 
};

// Duração das notas em milissegundos
const uint note_duration[] = {
     500, 500, 500, 350, 150, 500, 350, 150, 650, 500,  // Primeira parte
     500, 500, 500, 350, 150, 500, 350, 150, 650, 500  
};


//função escrita no display.
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale){
    ssd1306_draw_string(&disp, pos_x, pos_y, scale, msg);  // desenha texto
    ssd1306_show(&disp);  // apresenta no Oled
}

//o desenho do retangulo fará o papel de seletor
void print_retangulo(int x1, int y1, int x2, int y2){
    ssd1306_draw_empty_square(&disp, x1, y1, x2, y2);
    ssd1306_show(&disp);
}




void rodar_programa_joystick_led(){
 // Configura o PWM
    setup_pwm_led(BLUE_LED_PIN, &slice_led_b, led_b_level); // Configura o PWM para o LED azul
    setup_pwm_led(RED_LED_PIN, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho
      bool exit = false;
    absolute_time_t last_check = get_absolute_time();
    uint16_t vrx_value,vry_value;
    while (!exit) {
        adc_select_input(0);    // Seleciona o canal ADC para o eixo X
        sleep_us(2);            // Pequeno delay para estabilidade
        vrx_value = adc_read(); // Lê o valor do eixo X (0-4095)

        // Leitura do valor do eixo Y do joystick
        adc_select_input(1); // Seleciona o canal ADC para o eixo Y
        sleep_us(2);        // Pequeno delay para estabilidade
        vry_value = adc_read(); 
        pwm_set_gpio_level(GREEN_LED_PIN, vrx_value); // Ajusta o brilho do LED azul com o valor do eixo X
        pwm_set_gpio_level(RED_LED_PIN, vry_value); 

        if (absolute_time_diff_us(last_check, get_absolute_time()) > 10000) {
            last_check = get_absolute_time();
            if (!gpio_get(SW)) {
                exit = true;
                pwm_set_gpio_level(RED_LED_PIN, 0); //
            }
        }
        sleep_ms(400);
    }
   

}  

void tocar_buzzer(){  
         // Tocar buzzer por 500ms
            for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
                if (star_wars_notes[i] == 0) {
                     sleep_ms(note_duration[i]);
                        if (gpio_get(SW) == 0) {return; }// Sai da função se o botão for pressionado
            } else {
                play_tone(star_wars_notes[i], note_duration[i]);
                  if (gpio_get(SW) == 0) { 
            return;  // Sai da função se o botão for pressionado
        }
            }
    } 
   
}

void ligar_led_rgb(){
    uint slice_r, slice_g, slice_b;
    setup_pwm_led(RED_LED_PIN, &slice_r, 0);
    setup_pwm_led(GREEN_LED_PIN, &slice_g, 0);
    setup_pwm_led(BLUE_LED_PIN, &slice_b, 0);

    uint16_t counter = 0;
    const uint16_t pwm_max = 4095;
    
    while (true) {
        // Cores em ondas senoidais com fase diferente
        uint16_t r = (sin(counter * 0.02) + 1) * 2047.5;
        uint16_t g = (sin(counter * 0.02 + 2.094) + 1) * 2047.5;
        uint16_t b = (sin(counter * 0.02 + 4.188) + 1) * 2047.5;

        pwm_set_gpio_level(RED_LED_PIN, r);
        pwm_set_gpio_level(GREEN_LED_PIN, g);
        pwm_set_gpio_level(BLUE_LED_PIN, b);
        if (gpio_get(SW) == 0) {
            pwm_set_gpio_level(RED_LED_PIN, 0);
            pwm_set_gpio_level(GREEN_LED_PIN, 0);
            pwm_set_gpio_level(BLUE_LED_PIN, 0);
            break;
        }else{
        counter++;    
        sleep_ms(20);
        }
    }
    pwm_set_gpio_level(RED_LED_PIN, 0);
    pwm_set_gpio_level(GREEN_LED_PIN, 0);
    pwm_set_gpio_level(BLUE_LED_PIN, 0);
}

int main()
{
    inicializa();
    char *text = ""; // texto do menu
    uint countdown = 0; // verificar seleções para baixo do joystick
    uint countup = 2;   // verificar seleções para cima do joystick
        
    while (true) {
        // Trecho de código aproveitado de https://github.com/BitDogLab/BitDogLab-C/blob/main/joystick/joystick.c
        adc_select_input(0);
        uint adc_y_raw = adc_read();
        const uint bar_width = 40;
        const uint adc_max = (1 << 12) - 1;
        uint bar_y_pos = adc_y_raw * bar_width / adc_max; // bar_y_pos determinará se o Joystick foi pressionado para cima ou para baixo

        // Texto do Menu
        ssd1306_clear(&disp);  // Limpa a tela
        print_texto(text = "Menu", 52, 2, 1);
        print_retangulo(2, pos_y, 120, 18);
        print_texto(text = "Joystick LED", 6, 18, 1.5);
        print_texto(text = "Tocar Buzzer", 6, 30, 1.5);
        print_texto(text = "LED RGB", 6, 42, 1.5);

        if (bar_y_pos < 20 && countdown < 2) {
            pos_y += 12;
            countdown += 1;
            countup -= 1;
        } else if (bar_y_pos > 20 && countup < 2) {
            pos_y -= 12;
            countup += 1;
            countdown -= 1;
        }

        sleep_ms(250);

        // Verifica se o botão foi pressionado
        if (gpio_get(SW) == 0) {
            switch (pos_y) {
                case 12:
                    rodar_programa_joystick_led();
                    break;
                case 24:
                    tocar_buzzer();
                    break;
                case 36:
                    ligar_led_rgb();
                    break;

            }
        }
    }

    return 0;
}
