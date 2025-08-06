#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    // Configuração dos pinos dos botões como entrada com pull-up
    const uint BUTTON_A_PIN = 5;  // Botão A no GPIO 5
    const uint BUTTON_B_PIN = 6;  // Botão B no GPIO 6
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);

    // Configuração dos LEDs RGB como saída
    const uint BLUE_LED_PIN= 12;   // LED azul no GPIO 12
    const uint RED_LED_PIN  = 13; // LED vermelho no GPIO 13
    const uint GREEN_LED_PIN = 11;  // LED verde no GPIO 11
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

    while (true) {
        // Se o botão A for pressionado
        if (gpio_get(BUTTON_A_PIN) == 0) {
            gpio_put(BLUE_LED_PIN, 0);   // Desliga o LED azul
            gpio_put(RED_LED_PIN, 1);   // Liga o LED vermelho
            gpio_put(GREEN_LED_PIN, 0);  // Desliga o LED verde
        }
        // Se o botão B for pressionado
        else if (gpio_get(BUTTON_B_PIN) == 0) {
            gpio_put(BLUE_LED_PIN, 0);   // Desliga o LED azul
            gpio_put(RED_LED_PIN, 0);   // Desliga o LED vermelho
            gpio_put(GREEN_LED_PIN, 1);  // Liga o LED verde
        }
        // Se nenhum botão estiver pressionado
        else {
            gpio_put(RED_LED_PIN, 0);   // Desliga o LED vermelho
            gpio_put(GREEN_LED_PIN, 0); // Desliga o LED verde
            gpio_put(BLUE_LED_PIN, 0);  // Desliga o LED azul
        }
        sleep_ms(10); // Pequeno atraso para evitar debounce
    }
    return 0; // O programa nunca alcança essa linha
}
