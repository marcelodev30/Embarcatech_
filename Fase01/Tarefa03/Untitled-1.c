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

// Configurações Wi‑Fi
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WEB_PORT 80

// pinos e módulos controlador I2C
#define I2C_PORT    i2c1
#define PINO_SCL    14
#define PINO_SDA    15
#define DHT_PIN     18
#define RELAY_PORTA_SOCIAL_PIN   16   // Pino para controlar a "Porta Social"

// Botões físicos
const int SW = 22;                      // Botão do Joystick
#define DOORBELL_BUTTON_PIN 21          // Botão de Campainha

// Alertas físicos
#define DOORBELL_LED_PIN 20             // LED de alerta para a campainha

// Pinos dos LEDs RGB e do buzzer
const uint BLUE_LED_PIN  = 12;          // LED azul
const uint RED_LED_PIN   = 13;          // LED vermelho
const uint GREEN_LED_PIN = 11;          // LED verde
const uint BUZZER_PIN    = 10;          // Pino do buzzer

uint pos_y = 18;
ssd1306_t disp;

// Variável global para armazenar a data/hora do último alerta da campainha (formato HH:MM:SS)
static char doorbell_alert_str[64] = "--";
// Variável para debounce do botão da campainha
static absolute_time_t last_doorbell_time = {0};

// Protótipos das funções
int dht11_read(float *temperature, float *humidity);
void mostrar_temperatura_umidade(void);
void controle_rele_porta_social(void);

// Protótipos do servidor web
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static void start_http_server(void);

// Configura o PWM para um LED específico
void setup_pwm_led(uint led, uint *slice, uint16_t level) {
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, 16.0f);
    pwm_set_wrap(*slice, 4096);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

// Inicializa todos os recursos do sistema
void inicializa(void) {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    i2c_init(I2C_PORT, 400 * 1000);  // 400 kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);

    // Inicializa os LEDs RGB
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);

    // Inicializa o buzzer com PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // Inicializa o botão do Joystick
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    // Inicializa o sensor DHT11
    gpio_init(DHT_PIN);
    gpio_pull_up(DHT_PIN);

    // Inicializa o pino da Porta Social
    gpio_init(RELAY_PORTA_SOCIAL_PIN);
    gpio_set_dir(RELAY_PORTA_SOCIAL_PIN, GPIO_OUT);
    gpio_put(RELAY_PORTA_SOCIAL_PIN, 1);  // Estado 1 = Fechada

    // Inicializa o botão de Campainha
    gpio_init(DOORBELL_BUTTON_PIN);
    gpio_set_dir(DOORBELL_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(DOORBELL_BUTTON_PIN);

    // Inicializa o LED de alerta da Campainha
    gpio_init(DOORBELL_LED_PIN);
    gpio_set_dir(DOORBELL_LED_PIN, GPIO_OUT);
    gpio_put(DOORBELL_LED_PIN, 0);
}

// Função para detectar botão pressionado (debounce) do Joystick
bool check_button_press(void) {
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

// Função para verificar o botão da Campainha e acionar o alerta
void check_doorbell(void) {
    // O botão da campainha é ativo em nível baixo
    if (!gpio_get(DOORBELL_BUTTON_PIN)) {
        absolute_time_t now = get_absolute_time();
        // Debounce: somente dispara se 500 ms se passaram
        if (absolute_time_diff_us(last_doorbell_time, now) > 500000) {
            last_doorbell_time = now;
            // Atualiza a string do alerta com a hora atual (simulada a partir do boot)
            uint32_t ms = to_ms_since_boot(now);
            uint32_t hours = ms / 3600000;
            uint32_t minutes = (ms % 3600000) / 60000;
            uint32_t seconds = (ms % 60000) / 1000;
            snprintf(doorbell_alert_str, sizeof(doorbell_alert_str), "%02u:%02u:%02u", hours, minutes, seconds);
            // Aciona alerta visual e sonoro
            gpio_put(DOORBELL_LED_PIN, 1);         // LED aceso
            play_tone(1000, 200);                    // Buzzer toca 1000 Hz por 200 ms
            gpio_put(DOORBELL_LED_PIN, 0);
        }
    }
}

// botao_campainha

// campainha_ultimo_alerta


// Toca uma nota no buzzer com a frequência e duração especificadas
void play_tone(uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(BUZZER_PIN, top / 2);
    sleep_ms(duration_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    sleep_ms(50);
}

// Exibe texto no display OLED
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale) {
    ssd1306_draw_string(&disp, pos_x, pos_y, scale, msg);
    ssd1306_show(&disp);
}

// Desenha um retângulo vazio no display OLED
void print_retangulo(int x1, int y1, int x2, int y2) {
    ssd1306_draw_empty_square(&disp, x1, y1, x2, y2);
    ssd1306_show(&disp);
}

// Função "Joystick LED"
void rodar_programa_joystick_led(void) {
    uint slice_blue, slice_red;
    setup_pwm_led(BLUE_LED_PIN, &slice_blue, 100);
    setup_pwm_led(RED_LED_PIN, &slice_red, 100);
    bool exit_program = false;
    while (!exit_program) {
        adc_select_input(0);
        sleep_us(2);
        uint16_t vrx_value = adc_read();
        adc_select_input(1);
        sleep_us(2);
        uint16_t vry_value = adc_read();
        pwm_set_gpio_level(BLUE_LED_PIN, vrx_value);
        pwm_set_gpio_level(RED_LED_PIN, vry_value);
        if (check_button_press()) {
            exit_program = true;
            pwm_set_gpio_level(BLUE_LED_PIN, 0);
            pwm_set_gpio_level(RED_LED_PIN, 0);
        }
        sleep_ms(100);
    }
}

// Tocar sequência de notas no buzzer (ex: tema Star Wars)
const uint star_wars_notes[] = { 523, 659, 784, 1047, 784, 659, 523, 392 };
const uint note_duration[] = { 200, 250, 300, 500, 300, 250, 400, 600 };
void tocar_buzzer(void) {
    size_t num_notes = sizeof(star_wars_notes) / sizeof(star_wars_notes[0]);
    for (size_t i = 0; i < num_notes; i++) {
        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
        } else {
            play_tone(star_wars_notes[i], note_duration[i]);
        }
        if (!gpio_get(SW)) {  // Permite sair se o botão for pressionado
            break;
        }
    }
}



// static char campainha_ultimo_alerta[64] = "--";


// Leitura do sensor DHT11
int dht11_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    busy_wait_ms(18);
    gpio_put(DHT_PIN, 1);
    busy_wait_us(40);
    gpio_set_dir(DHT_PIN, GPIO_IN);
    uint32_t timeout = 0;
    while (gpio_get(DHT_PIN)) {
        if (++timeout > 100000) return -1;
        busy_wait_us(1);
    }
    timeout = 0;
    while (!gpio_get(DHT_PIN)) {
        if (++timeout > 100000) return -1;
        busy_wait_us(1);
    }
    timeout = 0;
    while (gpio_get(DHT_PIN)) {
        if (++timeout > 100000) return -1;
        busy_wait_us(1);
    }
    for (int i = 0; i < 40; i++) {
        while (!gpio_get(DHT_PIN));
        uint32_t t = time_us_32();
        while (gpio_get(DHT_PIN));
        if (time_us_32() - t > 40)
            data[i/8] |= (1 << (7 - (i % 8)));
    }
    if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
        return -1;
    }
    *humidity = data[0];
    *temperature = data[2];
    return 0;
}

// Exibe informações de temperatura e umidade no display OLED
void mostrar_temperatura_umidade(void) {
    bool exit_program = false;
    char msg[32];
    float temp, humi;
    while (!exit_program) {
        ssd1306_clear(&disp);
        if (dht11_read(&temp, &humi) == 0) {
            print_texto("Temperatura", 20, 2, 1);
            snprintf(msg, sizeof(msg), "Temp: %.1f C", temp);
            print_texto(msg, 6, 20, 1);
            snprintf(msg, sizeof(msg), "Umidade: %.1f %%", humi);
            print_texto(msg, 6, 40, 1);
            if (check_button_press()) exit_program = true;
        } else {
            print_texto("Erro no sensor!", 6, 30, 1);
            if (check_button_press()) exit_program = true;
        }
        ssd1306_show(&disp);
        if (check_button_press()) exit_program = true;
        sleep_ms(2000);
    }
}

// Controle da Porta Social via display e joystick
void controle_rele_porta_social(void) {
    uint sub_pos_y = 20;
    bool exit_submenu = false;
    const uint16_t lower_threshold = 1000;
    const uint16_t upper_threshold = 3000;
    while (!exit_submenu) {
        adc_select_input(0);
        sleep_us(2);
        uint16_t adc_val = adc_read();
        if (adc_val > upper_threshold && sub_pos_y > 20) {
            sub_pos_y -= 14;
            sleep_ms(200);
        } else if (adc_val < lower_threshold && sub_pos_y < 48) {
            sub_pos_y += 14;
            sleep_ms(200);
        }
        ssd1306_clear(&disp);
        print_texto("CONTROLE PORTA SOCIAL", 6, 2, 1);
        print_retangulo(2, sub_pos_y - 2, 120, 12);
        print_texto("> Abrir", 10, 20, 1);
        print_texto("> Fechar", 10, 34, 1);
        print_texto("> Sair", 10, 48, 1);
        ssd1306_show(&disp);
        if (check_button_press()) {
            if (sub_pos_y == 20) {
                gpio_put(RELAY_PORTA_SOCIAL_PIN, 0);
                sleep_ms(1000);
            } else if (sub_pos_y == 34) {
                gpio_put(RELAY_PORTA_SOCIAL_PIN, 1);
            } else if (sub_pos_y == 48) {
                exit_submenu = true;
            }
        }
        sleep_ms(100);
    }
}

// ================= FUNÇÕES DO SERVIDOR WEB =================

// Callback que processa as requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *request = (char *)p->payload;
    printf("Requisição recebida: %s\n", request);
    if (strstr(request, "GET /relay/on")) {
        gpio_put(RELAY_PORTA_SOCIAL_PIN, 0);
        printf("Porta Social aberta via web.\n");
    } else if (strstr(request, "GET /relay/off")) {
        gpio_put(RELAY_PORTA_SOCIAL_PIN, 1);
        printf("Porta Social fechada via web.\n");
    }
    float temp = 0.0f, hum = 0.0f;
    int sensor_status = dht11_read(&temp, &hum);
    int doorState = gpio_get(RELAY_PORTA_SOCIAL_PIN);
    const char *doorStatus = (doorState == 1) ? "Fechada" : "Aberta";
    char html_response[2048];
    if (sensor_status == 0) {
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
            temp, doorbell_alert_str, doorStatus);
    } else {
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
            "    <h2>Erro ao ler dados do sensor.</h2>"
            "    <p><a class=\"btn btn-secondary\" href=\"/\">Atualizar</a></p>"
            "  </div>"
            "</body>"
            "</html>");
    }
    tcp_write(tpcb, html_response, strlen(html_response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

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

int main() {
    inicializa();
    pos_y = 18;
    sleep_us(3);
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi‑Fi.\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Conectando à rede Wi‑Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar na rede Wi‑Fi.\n");
        return 1;
    }
    uint8_t *ip = (uint8_t *)&cyw43_state.netif[0].ip_addr.addr;
    printf("Wi‑Fi conectado! Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    start_http_server();
    sleep_us(3);
    while (true) {
        cyw43_arch_poll();
        // Verifica continuamente o botão da campainha e atualiza o alerta
        check_doorbell();
        ssd1306_clear(&disp);
        print_texto("Menu", 52, 2, 1);
        print_retangulo(2, pos_y - 2, 120, 12);
        print_texto("Joystick LED", 6, 18, 1.5);
        print_texto("Tocar Buzzer", 6, 30, 1.5);
        print_texto("Controle Porta Social",  6, 42, 1.5);
        print_texto("Temp/Umidade", 6, 54, 1.5);
        adc_select_input(0);
        sleep_us(2);
        uint16_t adc_val = adc_read();
        const uint16_t lower_threshold = 1000;
        const uint16_t upper_threshold = 3000;
        if (adc_val > upper_threshold && pos_y > 18) {
            pos_y -= 12;
            sleep_ms(300);
        } else if (adc_val < lower_threshold && pos_y < 54) {
            pos_y += 12;
            sleep_ms(300);
        }
        if (check_button_press()) {
            ssd1306_clear(&disp);
            if (pos_y == 18) {
                rodar_programa_joystick_led();
            } else if (pos_y == 30) {
                tocar_buzzer();
            } else if (pos_y == 42) {
                controle_rele_porta_social();
            } else if (pos_y == 54) {
                mostrar_temperatura_umidade();
            }
            sleep_ms(500);
        }
        sleep_ms(100);
    }
    cyw43_arch_deinit();
    return 0;
}
