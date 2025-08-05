#include "pico/stdlib.h"

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12


#define BTN_A_PIN 5

// Função para gerar um beep intermitente

// Função de travessia de pedestres
void travessia_pedestre();
// Ativar o LED verde
void sinal_verde();
// Ativar o LED amarelo
void sinal_amarelo();
// Ativar o LED vermelho
void sinal_vermelho();
//Todos os LEDs apagados.
// Função genérica para verificar o botão durante a execução do Semáforo
void verificar_executa(void (*sinal)(), int tempo);

void set_leds(bool red, bool green, bool blue){
    gpio_put(LED_R_PIN, red);
    gpio_put(LED_G_PIN, green);
    gpio_put(LED_B_PIN, blue);
}

int main(){
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    
    
    while(true){
    
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
        if (gpio_get(BTN_A_PIN) == 0) {
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
   sinal_verde();

    // Acionar o buzzer intermitente por 15 segundos
    for (int i = 0; i < 15; i++) {
        
        sleep_ms(1000); // Pausa de 1 segundo entre os beeps
    }

    // Desativar LEDs e buzzer
   
   sinal_verde();
 
    return;
}

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