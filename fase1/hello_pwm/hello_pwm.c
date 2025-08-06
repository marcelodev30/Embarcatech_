#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"


const int led_verde_pedestre = 11;
const int buzzer = 21;
const int botao = 5;    
//--------------------------------

// Função para gerar um beep intermitente
void beep(); 
// Função de travessia de pedestres
void travessia_pedestre();
// Ativar o LED verde
void sinal_verde();
// Ativar o LED amarelo
void sinal_amarelo();
// Ativar o LED vermelho
void sinal_vermelho();
//Todos os LEDs apagados.
void low_Lends();
// Função para inicializar LEDs, botão e buzzer
void setup_init();
// Função genérica para verificar o botão durante a execução do Semáforo
void verificar_executa(void (*sinal)(), int tempo);
void set_leds(bool red, bool green, bool blue);



int main() {  
// Função para inicializar LEDs, botão e buzzer
setup_init();
//----------------------------------------------
  while (true) {
    // Semáforo o sinal verde
    verificar_executa(sinal_verde,8000);
    
    // Semáforo o sinal amarelo
    verificar_executa(sinal_amarelo,2000);
    
    // Semáforo o sinal vermelho
    verificar_executa(sinal_vermelho,10000);
   
  }
}


// Função genérica para verificar o botão durante a execução do Semáforo
void verificar_executa(void (*sinal)(), int tempo) {
   
    // Dividir o tempo em passos de 10ms para checar o botão
    for (int i = 0; i < (tempo / 10); i++) {
       // Acender o sinal correspondente
       sinal();
        if (gpio_get(botao) == 0) {
            // Botão pressionado, executa a travessia
            travessia_pedestre();
            break;
        }
        sleep_ms(10); // Espera 10ms por iteração
    }
}


// Função de travessia de pedestres
void travessia_pedestre() {
    // Ativar o LED amarelo por 5 segundos
    sinal_amarelo();
    sleep_ms(5000);

    // Ativar o LED vermelho e o LED verde para pedestres
    sinal_vermelho();
    set_leds(0,1,0);

    // Acionar o buzzer intermitente por 15 segundos
    for (int i = 0; i < 15; i++) {
        beep();
        sleep_ms(1000); // Pausa de 1 segundo entre os beeps
    }

    // Desativar LEDs e buzzer
    low_Lends();
    set_leds(0,1,0);
    pwm_set_gpio_level(buzzer, 0);
    return;
}

// Função para gerar um beep intermitente
void beep() {
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    pwm_set_gpio_level(buzzer, 2048); // Ativar PWM (50% duty cycle)
    sleep_ms(100); // Duração do beep
    pwm_set_gpio_level(buzzer, 0);   // Desativar PWM
    sleep_ms(10); // Pausa entre os beeps
}

// Ativar o LED verde
void sinal_verde(){
set_leds(0,1,0);

} 
// Ativar o LED amarelo
void sinal_amarelo(){
 set_leds(1,1,0);
} 
// Ativar o LED vermelho
void sinal_vermelho(){
 set_leds(1,0,0);
  
} 
//Todos os LEDs apagados.
void low_Lends(){
set_leds(0,0,0);
}

// Função para inicializar LEDs, botão e buzzer
void setup_init(){
    // Inicialização dos LEDs do semáforo
  
 low_Lends();
  
    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);
    gpio_init(12);
    gpio_set_dir(12, GPIO_OUT);


  // Inicialização do LED de pedestres
  gpio_init(led_verde_pedestre);
  gpio_set_dir(led_verde_pedestre, GPIO_OUT);
  gpio_put(led_verde_pedestre, 0);
 
  // Inicialização do botão
  gpio_init(botao);
  gpio_set_dir(botao, GPIO_IN);
  gpio_pull_up(botao);
 
 
 // Inicialização do buzzer
  gpio_init(buzzer);
  gpio_set_dir(buzzer, GPIO_OUT);
  gpio_set_function(buzzer, 100);
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (100 * 4096));
    pwm_init(slice_num, &config, true);
  pwm_set_gpio_level(buzzer, 0);
//-----------------------------------------------------------------------
}


void set_leds(bool red, bool green, bool blue){
    gpio_put(13, red);
    gpio_put(11, green);
    gpio_put(12, blue);
}