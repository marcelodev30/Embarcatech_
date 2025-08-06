#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "mqtt.h"

// Pino do botão
#define BUTTON_PIN 15  // Botão conectado ao GPIO15

// Variável global para controle de envio
bool send_data = true;  // Estado inicial: envio ativado

// Callback para mensagens MQTT
void mqtt_callback(mqtt_message_t *msg) {
    if (strcmp(msg->topic, "device/control") == 0) {
        if (strcmp((char *)msg->payload, "ON") == 0) {
            send_data = true;
            printf("Envio de dados ativado via MQTT\n");
        } else if (strcmp((char *)msg->payload, "OFF") == 0) {
            send_data = false;
            printf("Envio de dados desativado via MQTT\n");
        }
    }
}

// Conecta ao Wi-Fi
void connect_to_wifi() {
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar o Wi-Fi\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms("SSID", "SENHA", CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
    } else {
        printf("Wi-Fi conectado com sucesso!\n");
    }
}

int main() {
    stdio_init_all();
    connect_to_wifi();

    // Inicializa botão
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Configura o cliente MQTT com login e senha
    mqtt_client_t client;
    mqtt_connect_params_t connect_params = {
        .host = "broker.hivemq.com",      // Substitua pelo endereço do broker
        .port = 1883,                    // Porta MQTT padrão
        .client_id = "pico_client",      // ID do cliente MQTT
        .username = "usuario_mqtt",      // Substitua pelo nome de usuário
        .password = "senha_mqtt",        // Substitua pela senha
        .keep_alive = 60,                // Intervalo keep-alive em segundos
    };
    mqtt_init_with_params(&client, &connect_params, mqtt_callback);

    if (!mqtt_connect(&client)) {
        printf("Erro ao conectar ao broker MQTT\n");
        return -1;
    }

    mqtt_subscribe(&client, "device/control", QOS0);

    while (1) {
        // Verifica o estado do botão
        if (gpio_get(BUTTON_PIN) == 0) {  // Botão pressionado
            send_data = !send_data;       // Alterna o estado
            sleep_ms(500);               // Debounce

            // Publica o estado atual no tópico MQTT
            char state[4];
            snprintf(state, sizeof(state), send_data ? "ON" : "OFF");
            mqtt_publish(&client, "device/control", state, strlen(state), QOS0);
            printf("Estado alterado pelo botão: %s\n", state);
        }

        // Publica dados fictícios se envio estiver ativado
        if (send_data) {
            const char *payload = "{\"status\": \"ativo\", \"valor\": 123}";
            mqtt_publish(&client, "device/data", payload, strlen(payload), QOS0);
            printf("Dados enviados: %s\n", payload);
        }

        mqtt_poll(&client);  // Processa mensagens MQTT
        sleep_ms(2000);      // Aguarda 2 segundos
    }
}
