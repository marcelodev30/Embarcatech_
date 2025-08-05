#include "pico/stdlib.h" 
#include "hardware/i2c.h" 
#include "inc/ssd1306.h" // Biblioteca para controle do OLED 
 
#define I2C_SDA_PIN 4 
#define I2C_SCL_PIN 5 
 
// Configuração do display OLED 
#define OLED_WIDTH 128 
#define OLED_HEIGHT 32 
ssd1306_t oled; 

const int LED_R_PIN = 13;
const int LED_G_PIN = 11;
const int LED_B_PIN = 12;

const int BTN_A_PIN = 5;
 
void init_oled() { 

    i2c_init(i2c_default, 100 * 1000); 
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C); 
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN); 
    ssd1306_init(&oled, i2c_default, 0x3C, OLED_WIDTH, OLED_HEIGHT); 
    
} 
 

void SinalAberto() { 
    
} 
 
void SinalAtencao() { 


} 
 
void SinalFechado() { 

} 
 
int main() { 
    // Inicialização de LEDs e Botão 
    gpio_init(LED_R_PIN); 
    gpio_set_dir(LED_R_PIN, GPIO_OUT); 
    gpio_init(LED_G_PIN); 
    gpio_set_dir(LED_G_PIN, GPIO_OUT); 
    gpio_init(LED_B_PIN); 
    gpio_set_dir(LED_B_PIN, GPIO_OUT); 
 
    gpio_init(BTN_A_PIN); 
    gpio_set_dir(BTN_A_PIN, GPIO_IN); 
    gpio_pull_up(BTN_A_PIN); 
 
    // Inicialização do OLED 
    init_oled(); 
 
    while (true) { 
    
 
    return 0; 
}
}