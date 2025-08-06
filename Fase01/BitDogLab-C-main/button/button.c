/**
 * Embarcatech 
 * Exemplo Botão com a BitDogLab
 */

#include "pico/stdlib.h"

#define LED_BLUE 12   // GPIO conectado ao terminal azul do LED RGB
#define BUTTON_A 5    // GPIO conectado ao Botão A

int main() {
    // Configuração do GPIO do LED como saída
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_put(LED_BLUE, false);  // Inicialmente, o LED está apagado

    // Configuração do GPIO do Botão A como entrada com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    while (true) {
        // Lê o estado do Botão A
        bool button_a_state = gpio_get(BUTTON_A);  // HIGH = solto, LOW = pressionado

        // Atualiza o estado do LED com base no estado do Botão A
        gpio_put(LED_BLUE, button_a_state);  // Se solto (HIGH), LED acende; se pressionado (LOW), apaga

        // Pequeno delay para evitar leituras inconsistentes (debounce simples)
        sleep_ms(50);
    }
}

