#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"

#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h" 
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    while (true)
    {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
