#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// Configurações Wi-Fi
#define WIFI_SSID "Marcelo Alves"
#define WIFI_PASSWORD "sua senha"
#define WEB_PORT 80

// Definições de pinos e módulos externo
#define DHT_PIN     18 // sersor de temperatura 
#define RELAY_PORTA_SOCIAL_PIN   16 // modulo rele 

// Definições de pinos e módulos controlador i2c selecionado
#define I2C_PORT    i2c1
#define PINO_SCL    14
#define PINO_SDA    15
#define NUM_PIXELS 25  // Matriz de LEDs 5x5     
#define WS2812_PIN 7        // GPIO dos LEDs
#define IS_RGBW false       // RGB padrão
const int SW = 22; //botão do Joystick
const int botao_campainha = 5; //botão da campainha 
const uint BLUE_LED_PIN  = 12;   // LED azul no GPIO 12
const uint RED_LED_PIN   = 13;   // LED vermelho no GPIO 13
const uint GREEN_LED_PIN = 11;   // LED verde no GPIO 11
const int Buzzer_PIN    = 10;   // Pino do buzzer

uint pos_y = 18;  
ssd1306_t disp;

static char campainha_ultimo_alerta[64] = "--"; // Variável armazenar a data/hora do último alerta da campainha (formato HH:MM:SS)

//============ Funções dos perifericos de Placa ==================================

int dht11_ler(float *temperature, float *humidity); // Função para ler dht11 sem biblioteca.
void MatrizLed(int matriz[5][5], uint8_t red, uint8_t green, uint8_t blue); // Função para liga Matriz Led de forma indelével 
void ApagaMatrizLed(); // Apaga todos os LEDs da Matriz Led.
void beep(); // Função para gerar um beep intermitente.
bool verificar_botao_Joystick(void); // Função para verificar se botão pressionado.
void verificar_Campainha(void); // Função para verificar se botão da Campainha e acionador.
void send_pixels(PIO pio, int sm, uint32_t *pixels); // Envia os pixels para os LEDs WS2812.
void set_pixel_color(uint32_t *pixels, int index, uint8_t r, uint8_t g, uint8_t b); // Define a cor de um pixel com brilho reduzido

//============ Funções dos Display =======================================================

void print_texto(char *msg, uint pos_x, uint pos_y, uint scale); // Exibe um texto no display OLED
void print_retangulo(int x1, int y1, int x2, int y2); // Desenha um retângulo vazio no display.

//============ Funções das opções do Menu =======================================================
void menu_temperatura_umidade(void); // Função do exibir a Temperatura no Display
void menu_rele_porta_social(void); // Função do SubMenu Controle do Rele no Display
void menu_campainha_ultimo_alerta(void); // Função do exibir o ultimo alerta da campainha

//============ Funções dos Servidor Web ===================================================
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static void start_http_server(void); // Inicia o servidor HTTP

// Função para inicialização de todos os recursos do placa
void inicializa(void);

//========================= Função Principal ==================================================
int main() {
    inicializa();// Função para inicialização de todos os recursos da placa
    static char ip_str[16];
    pos_y = 18;  
    sleep_us(2);
    ssd1306_clear(&disp);
    print_texto("carregando",6, 18, 2);
 // Inicializa o Wi‑Fi (Pico W)
 if (cyw43_arch_init()) {
        ssd1306_clear(&disp);
        print_texto("Erro ao inicializar o Wi‑Fi.",6, 18, 1);
}else{
    cyw43_arch_enable_sta_mode();
    ssd1306_clear(&disp);
    print_texto("Conectando à rede Wi‑Fi....",6, 18, 1);
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        ssd1306_clear(&disp);
        print_texto("Falha ao conectar na rede Wi‑Fi.",6, 18, 1);
        snprintf(ip_str, sizeof(ip_str), "WIFi Erro");
    }else{
        uint8_t *ip = (uint8_t *)&cyw43_state.netif[0].ip_addr.addr;
        snprintf(ip_str, sizeof(ip_str), "IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }
}
    
// Inicia o servidor HTTP
start_http_server();
    while (true) {
        cyw43_arch_poll();
        verificar_Campainha();
         // Texto do Menu
        ssd1306_clear(&disp);
        print_texto("Menu", 52, 2, 1);
        print_retangulo(2, pos_y - 2, 120, 12);
        print_texto("Controle Porta Social", 6, 18, 1.5);
        print_texto("Temp/Umidade", 6, 30, 1.5);
        print_texto("Campainha Status", 6, 42, 1.5);
        print_texto(ip_str,  6, 54, 1.5); 
        
        //print_texto("Temp/Umidade", 6, 54, 1.5);
      
        
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

        if (verificar_botao_Joystick()) {
            ssd1306_clear(&disp);
            if (pos_y == 18) {
                menu_rele_porta_social();
            } else if (pos_y == 30) {
                menu_temperatura_umidade();
            }  else if (pos_y == 42) {
                menu_campainha_ultimo_alerta();
            } 
            
            sleep_ms(500);
        }    
        sleep_ms(100);
    }
    cyw43_arch_deinit();
    return 0;
}

// Função para inicialização de todos os recursos do sistema
void inicializa(void) {
    stdio_init_all();

    // Inicialização do adc Joystick
    adc_init();
    adc_gpio_init(26); 
    adc_gpio_init(27);
    
    // Inicialização do Display i2c 
    i2c_init(I2C_PORT, 400 * 1000);  // 400 kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);

    // Inicialização dos pinos dos LEDs RGB
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);

    // Inicialização do buzzer com PWM
    gpio_set_function(Buzzer_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(Buzzer_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(Buzzer_PIN, 0);

    // Inicialização do Botão do Joystick
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    // Inicialização do botão com pull-up
    gpio_init(botao_campainha);
    gpio_set_dir(botao_campainha, GPIO_IN);
    gpio_pull_up(botao_campainha);
 
    // Inicialização do DHT11 com pull-up
    gpio_init(DHT_PIN);
    gpio_pull_up(DHT_PIN);

    //Inicialização do Modulo Relé
    gpio_init(RELAY_PORTA_SOCIAL_PIN);
    gpio_set_dir(RELAY_PORTA_SOCIAL_PIN, GPIO_OUT);
    gpio_put(RELAY_PORTA_SOCIAL_PIN, 1);
    
}

// Função para verificar se botão pressionado
bool verificar_botao_Joystick(void) {
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

// Função para verificar se botão da Campainha e acionador
void verificar_Campainha(void) {
    int matriz[5][5] = {
        {1, 0, 0, 0, 1},  
        {0, 1, 0, 1, 0},  
        {0, 0, 1, 0, 0},  
        {0, 1, 0, 1, 0},  
        {1, 0, 0, 0, 1}   
    };
    static absolute_time_t last_press_time = 0;
    if (!gpio_get(botao_campainha)) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_press_time, now) > 500000) {
            last_press_time = now;
      // ================= Simula a hora a partir do boot =============================
            // Atualiza a string do alerta com a hora simulada a partir do boot
            uint32_t ms = to_ms_since_boot(now);
            uint32_t hours = ms / 3600000;
            uint32_t minutes = (ms % 3600000) / 60000;
            uint32_t seconds = (ms % 60000) / 1000;
      // ================================================================================== 
            snprintf(campainha_ultimo_alerta, sizeof(campainha_ultimo_alerta), "13-02-2025 %02u:%02u:%02u", hours, minutes, seconds);
            // Aciona alerta visual e sonoro
            for (size_t i = 0; i < 5; i++){
                MatrizLed(matriz,0,255,0);
                    beep();
                    sleep_ms(500);
                    ApagaMatrizLed(); 
                    sleep_ms(500);     
             }
        }
    }
   
}


// Função para gerar um beep intermitente
void beep() {
    uint slice_num = pwm_gpio_to_slice_num(Buzzer_PIN);
    pwm_set_gpio_level(Buzzer_PIN, 2048); // Ativar PWM (50% duty cycle)
    sleep_ms(100); // Duração do beep
    pwm_set_gpio_level(Buzzer_PIN, 0);   // Desativar PWM
    sleep_ms(10); // Pausa entre os beeps
}

// Função para ler dht11 sem biblioteca
int dht11_ler(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    
    // Envia sinal de início
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    busy_wait_ms(18);
    gpio_put(DHT_PIN, 1);
    busy_wait_us(40);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    // Aguarda resposta do sensor
    uint32_t timeout = 0;
    while (gpio_get(DHT_PIN)) {  // Aguarda pino baixo
        if (++timeout > 100000) return -1; // Timeout após 100ms
        busy_wait_us(1);
    }
    timeout = 0;
    while (!gpio_get(DHT_PIN)) { // Aguarda pino alto
        if (++timeout > 100000) return -1;
        busy_wait_us(1);
    }
    timeout = 0;
    while (gpio_get(DHT_PIN)) { // Aguarda pino baixo novamente
        if (++timeout > 100000) return -1;
        busy_wait_us(1);
    }

    // Lê os 40 bits de dados
    for (int i = 0; i < 40; i++) {
        while (!gpio_get(DHT_PIN)); // Aguarda transição para alto
        uint32_t t = time_us_32();
        while (gpio_get(DHT_PIN)); // Mede a duração do sinal alto
        if (time_us_32() - t > 40) data[i/8] |= (1 << (7 - (i%8)));
    }

    // Verifica checksum
    if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
        return -1;
    }else
    {
        *humidity = 50.0;
        *temperature = 50.0;
    }

    *humidity = data[0];
    *temperature = data[2];

    return 0;
}

// Função para Controle do Rele  
void controleRele(bool status){
    if (status)gpio_put(RED_LED_PIN, status);
    else if (!status)gpio_put(RED_LED_PIN, 0);
    int matriz[5][5] = {
        {1, 1, 1, 1, 1},  
        {1, 0, 0, 0, 1},  
        {1, 1, 0, 0, 1},  
        {1, 0, 0, 0, 1},  
        {1, 1, 1, 1, 1}   
    };
    
    gpio_put(RELAY_PORTA_SOCIAL_PIN, !status);
    for (size_t i = 0; i < 3; i++){
        if (status)MatrizLed(matriz,255,0,0);
        else MatrizLed(matriz,0,255,0);
            beep();
            sleep_ms(1000);
            ApagaMatrizLed(); 
            sleep_ms(500);     
     }
}

// ================= FUNÇÕES DO display OLED =================================

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

// Função do exibir a Temperatura
void menu_temperatura_umidade(void) {
    bool exit_program = false;
    char msg[32];
    float temp, humi;
    while (!exit_program) {
        ssd1306_clear(&disp);
        if (dht11_ler(&temp, &humi) == 0) {
            print_texto("Temperatura", 20, 2, 1);
            snprintf(msg, sizeof(msg), "Temp: %.1f C", temp);
            print_texto(msg, 6, 20, 1);
            snprintf(msg, sizeof(msg), "Umidade: %.1f %%", humi);
            print_texto(msg, 6, 40, 1);
        } else {
            print_texto("Erro no sensor!", 6, 30, 1);
            if (verificar_botao_Joystick()) exit_program = true;
        }
        ssd1306_show(&disp);
        //  tempo de Leitura em 10ms para checar o botão.
        for (int i = 0; i < (2000 / 10); i++) {
         if (verificar_botao_Joystick()) exit_program = true;
             sleep_ms(10); // Espera 10ms por iteração
     }
    }
}

// Função do exibir o ultimo alerta da campainha
void menu_campainha_ultimo_alerta(void) {
    bool exit_program = false;
    ssd1306_clear(&disp);
    print_texto("Ultimo Alerta", 20, 2, 1);
    ssd1306_show(&disp);
    if (strcmp(campainha_ultimo_alerta, "--") == 0){
        print_texto("Sem registros", 20, 20, 1);
        print_texto("Clique para sair", 10, 40, 1);
        ssd1306_show(&disp);
    }else {
        print_texto(campainha_ultimo_alerta, 6, 20, 1);
        print_texto("Clique para sair", 10, 40, 1);
        ssd1306_show(&disp);
    }
    while (!exit_program) {
         if (verificar_botao_Joystick()) exit_program = true;
    }
}

// Função do SubMenu Controle do Rele  
void menu_rele_porta_social(void) {
    uint sub_pos_y = 20; 
    bool exit_submenu = false;
    const uint16_t lower_threshold = 1000;
    const uint16_t upper_threshold = 3000;

    while (!exit_submenu) {
        // Leitura do joystick
        adc_select_input(0);
        sleep_us(2);
        uint16_t adc_val = adc_read();

        // Navegação vertical
        if (adc_val > upper_threshold && sub_pos_y > 20) {
            sub_pos_y -= 14;
            sleep_ms(200);
        } else if (adc_val < lower_threshold && sub_pos_y < 48) {
            sub_pos_y += 14;
            sleep_ms(200);
        }
        // Exibição do submenu
        ssd1306_clear(&disp);
        print_texto("CONTROLE PORTA SOCIAL", 6, 2, 1);
        print_retangulo(2, sub_pos_y - 2, 120, 12);
        print_texto("> Abrir", 10, 20, 1);
        print_texto("> Fechar", 10, 34, 1);
        print_texto("> Sair", 10, 48, 1);
        ssd1306_show(&disp);

        // Ação do botão
        if (verificar_botao_Joystick()) {
            if (sub_pos_y == 20) {
                controleRele(true);
                sleep_ms(1000);
            } else if (sub_pos_y == 34) {
                controleRele(false);
                sleep_ms(1000);
            } else if (sub_pos_y == 48) {
                exit_submenu = true;
            }
        }
        sleep_ms(100);
    }
}

// ================= FUNÇÕES DO SERVIDOR WEB ==========================================

// Callback que processa requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *request = (char *)p->payload;
    printf("Requisição recebida: %s\n", request);
    if (strstr(request, "GET /relay/on")) {
        //gpio_put(RELAY_PORTA_SOCIAL_PIN, 0);
        controleRele(true);
        printf("Porta Social aberta via web.\n");
    } else if (strstr(request, "GET /relay/off")) {
        //gpio_put(RELAY_PORTA_SOCIAL_PIN, 1);
        controleRele(false);
        printf("Porta Social fechada via web.\n");
    }
    float temp = 0.0f, hum = 0.0f;
    int sensor_status = dht11_ler(&temp, &hum);
    int doorState = gpio_get(RELAY_PORTA_SOCIAL_PIN);
    const char *doorStatus = (doorState == 1) ? "Fechada" : "Aberta";
    char html_response[2048];
   
        snprintf(html_response, sizeof(html_response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<!DOCTYPE html>"
            "<html lang=\"pt\">"
            "<head>"
            "  <meta charset=\"UTF-8\">"
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "  <title>Automação Residencial</title>"
            "  <link href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css\" rel=\"stylesheet\">"
            "  <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css\" rel=\"stylesheet\">"
            "</head>"
            "<body class=\"bg-dark text-white text-center p-4\">"
            "  <div class=\"container\">"
            "    <h2>Automação Residencial</h2>"
            "    <div class=\"row g-3 justify-content-center\">"
            "      <div class=\"col-md-4\">"
            "        <div class=\"card bg-secondary text-white text-center p-3\">"
            "          <i class=\"fas fa-thermometer-half text-warning fs-1\"></i>"
            "          <p class=\"mt-2\">Temperatura: <span>%.1f°C</span></p>"
            "        </div>"
            "      </div>"
            "      <div class=\"col-md-4\">"
            "        <div class=\"card bg-secondary text-white text-center p-3\">"
            "          <i class=\"fas fa-bell text-warning fs-1\"></i>"
            "          <p class=\"mt-2\">Campainha: <span>%s</span></p>"
            "        </div>"
            "      </div>"
            "      <div class=\"col-md-4\">"
            "        <div class=\"card bg-secondary text-white text-center p-3\">"
            "          <i class=\"fas fa-door-closed text-info fs-1\"></i>"
            "          <p class=\"mt-2\">Porta Social: <span>%s</span></p>"
            "          <p class=\"mt-2\">"
            "            <a class=\"btn btn-success\" href=\"/relay/on\">Abrir</a> "
            "            <a class=\"btn btn-danger\" href=\"/relay/off\">Fechar</a>"
            "          </p>"
            "        </div>"
            "      </div>"
            "    </div>"
            "    <p class=\"mt-3\"><a class=\"btn btn-secondary\" href=\"/\">Atualizar</a></p>"
            "  </div>"
            "</body>"
            "</html>",
            temp,campainha_ultimo_alerta, doorStatus);

    tcp_write(tpcb, html_response, strlen(html_response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

// Callback de conexão: associa o callback de recepção
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

// Inicia o servidor HTTP na porta 80
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB.\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80.\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80.\n");
}

// ================= FUNÇÕES da Matriz Led =========================================

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

// Função para liga Matriz Led de forma indelével 
void MatrizLed(int matriz[5][5], uint8_t red, uint8_t green, uint8_t blue) {
    PIO pio = pio0;
    int sm = 0;
    uint32_t pixels[NUM_PIXELS] = {0};

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Configura os LEDs com brilho reduzido
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int index = y * 5 + x;
            if (matriz[y][x] == 1) {
                set_pixel_color(pixels, index, red, green, blue);  
            }
        }
    }
    // Envia os dados para os LEDs
    send_pixels(pio, sm, pixels);
}

// Apaga todos os LEDs da Matriz Led
void ApagaMatrizLed() {
    PIO pio = pio0;
    int sm = 0;
    uint32_t pixels[NUM_PIXELS] = {0};
    send_pixels(pio, sm, pixels);
}