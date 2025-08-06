#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#define BOTAO_PIN 5         // Botão para ativar a campainha




#define NUM_PIXELS 25       // Matriz de LEDs 5x5
#define WS2812_PIN 7        // GPIO dos LEDs
#define IS_RGBW false       // RGB padrão


// Define a cor de um pixel com brilho reduzido
void set_pixel_color(uint32_t *pixels, int index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= 0 && index < NUM_PIXELS) {
        pixels[index] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
}

// Envia os pixels para os LEDs WS2812
void send_pixels(PIO pio, int sm, uint32_t *pixels) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        pio_sm_put_blocking(pio, sm, pixels[i] << 8);
    }
}

// Exibe um ícone de campainha com brilho reduzido
void DisplayCampainha() {
    PIO pio = pio0;
    int sm = 0;
    uint32_t pixels[NUM_PIXELS] = {0};

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Matriz da campainha // baixo
    int pattern[5][5] = {
        {1, 1, 1, 1, 1},  
        {1, 0, 0, 0, 1},  
        {1, 1, 0, 0, 1},  
        {1, 0, 0, 0, 1},  
        {1, 1, 1, 1, 1}   
    };
    // Configura os LEDs com brilho reduzido
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;
            if (pattern[y][x] == 1) {
                set_pixel_color(pixels, index, 5, 0, 0);  // Amarelo 50% brilho
            }
        }
    }

    // Envia os dados para os LEDs
    send_pixels(pio, sm, pixels);
}

// Apaga todos os LEDs
void ClearLeds() {
    PIO pio = pio0;
    int sm = 0;
    uint32_t pixels[NUM_PIXELS] = {0};
    send_pixels(pio, sm, pixels);
}

int main() {
    stdio_init_all();

    gpio_init(BOTAO_PIN);
    gpio_set_dir(BOTAO_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_PIN);

    while (true) {
        if (!gpio_get(BOTAO_PIN)) {  // Botão pressionado
            DisplayCampainha();
            sleep_ms(2000);
            ClearLeds();
            sleep_ms(500);
        }
    }

    return 0;
}
