#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#define MQTT_TOPIC_LEN 100
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0

#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1

#define TEMP_WORKER_TIME_S 10

typedef struct
{
    mqtt_client_t *mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
} MQTT_CLIENT_DATA_T;



static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("Connected to MQTT broker\n");
    }
    else
    {

        panic("Failed to connect to mqtt server");
    }
}

static void start_client(MQTT_CLIENT_DATA_T *state)
{
    state->mqtt_client_inst = mqtt_client_new();
    if (!state->mqtt_client_inst)
    {
        panic("MQTT client instance creation error");
    }

    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, MQTT_PORT, mqtt_connection_cb, state, &state->mqtt_client_info) != ERR_OK)
    {
        panic("MQTT broker connection error");
    }

    cyw43_arch_lwip_end();
}

int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip

    if (cyw43_arch_init())
    {
        panic("Failed to inizialize CYW43");
    }

    while (true)
    {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
