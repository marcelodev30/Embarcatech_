#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
#define BTN_A_PIN 5

int A_state = 0;    // Botão A está pressionado?

void SinalAberto();
void SinalAtencao();
void SinalFechado();



int WaitWithRead(int timeMS){
    for(int i = 0; i < timeMS; i = i+100){
        A_state = !gpio_get(BTN_A_PIN);
        if(A_state == 1){
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
}

int main(){
    
    // INICIANDO LEDS
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    // INICIANDO BOTÃO
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    // Inicializa a comunicação I2C e o display OLED
    stdio_init_all();
    i2c_init(i2c1, 400000);  // Define a velocidade do I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(); // Inicializa o display OLED
 

    
        while (true) {
        SinalFechado(); // Semáforo VERMELHO para pedestres
        A_state = WaitWithRead(8000); // Espera até 8 segundos ou botão ser pressionado

        if (A_state) { // Botão pressionado - acelera transição para o VERDE
            SinalAtencao();
            sleep_ms(2000); // Semáforo AMARELO por 2 segundos
            SinalAberto();
            sleep_ms(10000); // Semáforo VERDE por 10 segundos
        } else { // Sem botão pressionado - fluxo normal
            SinalAtencao();
            sleep_ms(2000); // Semáforo AMARELO por 2 segundos
            SinalAberto();
            sleep_ms(8000); // Semáforo VERDE por 8 segundos
        }
    }

    return 0;
}

void SinalAberto(){
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);
   
     struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    char *text[] = {
        "              ",
        "              ",
        "  SINAL ABERTO",
        "              ",
        "  ATRAVESSAR  ",
        "  COM CUIDADO  "
    };
    
    int y = 0;
    for (uint i = 0; i < count_of(text); i++){
        ssd1306_draw_string(ssd, 10, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);

 }

void SinalAtencao(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);

struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);


    char *text[] = {
        "         ",
        "         ",
        "  SINAL De",
        "  ATENCAO",
        "  PREPARE-SE ",
     
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++){
        ssd1306_draw_string(ssd, 10, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void SinalFechado(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
  struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    char *text[] = {
        "              ",
        "              ",
        "SINAL FECHADO",
        "              ",
        "   AGUARDE  ",
        "           "
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++){
        ssd1306_draw_string(ssd, 10, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}
