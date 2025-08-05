#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define IR_PIN 18  // Pino GPIO para o receptor IR
#define DEBOUNCE_DELAY 500  // 500 microssegundos para debounce

// Variáveis para capturar o tempo dos pulsos
volatile uint32_t last_time = 0;
volatile uint32_t pulse_width = 0;
volatile uint32_t ir_data = 0;  // Variável para armazenar o código IR capturado
volatile uint8_t bit_count = 0; // Contador de bits

// Função para converter o valor de um pulso para hexadecimal e exibir no monitor serial
void print_hex(uint32_t value) {
    printf("Código IR: 0x%08X\n", value);
}

// Função de callback para capturar os pulsos do IR
void ir_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Implementa debounce, ignorando pulsos se o intervalo for menor que DEBOUNCE_DELAY
    if (current_time - last_time < DEBOUNCE_DELAY) {
        return;  // Ignora o sinal, pois está muito próximo do último pulso
    }

    pulse_width = current_time - last_time;  // Calcula a largura do pulso
    last_time = current_time;

    // Filtro de modulação (38kHz): Considera apenas pulsos maiores que 1000 microssegundos como válidos
    if (pulse_width > 1000) {  // Pulso longo é considerado '1'
        ir_data |= (1 << (31 - bit_count));  // Ajuste o bit mais significativo
    } else {  // Pulso curto é considerado '0'
        ir_data &= ~(1 << (31 - bit_count));
    }

    // Incrementa o contador de bits
    bit_count++;

    // Quando 32 bits forem capturados (um código completo IR)
    if (bit_count >= 32) {
        print_hex(ir_data);  // Exibe o código IR completo em hexadecimal
        ir_data = 0;         // Reseta os dados
        bit_count = 0;       // Reseta o contador de bits
    }
}

int main() {
    // Inicializa a UART para o monitor serial
    stdio_init_all();
    printf("Iniciando captura de sinais IR...\n");

    // Inicializa o pino do receptor IR
    gpio_init(IR_PIN);
    gpio_set_dir(IR_PIN, GPIO_IN);
    gpio_pull_up(IR_PIN);  // Habilita o pull-up interno

    // Configura a interrupção para detectar transições no pino IR
    gpio_set_irq_enabled_with_callback(IR_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &ir_callback);

    // Loop principal
    while (1) {
        tight_loop_contents();  // Continua executando
    }

    return 0;
}
