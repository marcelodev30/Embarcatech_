#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define SAMPLE_RATE 2000.0 // Taxa de amostragem em Hz
#define N 400              // Número de amostras
#define TARGET_FREQ 261.0  // Frequência alvo: C4

float goertzel(float *samples, int num_samples, float target_freq, float sample_rate)
{
    float omega = 2.0 * M_PI * target_freq / sample_rate;
    float coeff = 2.0 * cos(omega);

    float s_prev = 0.0;
    float s_prev2 = 0.0;
    float s;

    for (int i = 0; i < num_samples; i++)
    {
        s = samples[i] + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
    }

    float power = s_prev2 * s_prev2 + s_prev * s_prev - coeff * s_prev * s_prev2;
    return power;
}

int main()
{
    stdio_init_all();
    adc_init();
    adc_gpio_init(28);   // Configura GPIO 28 como entrada do ADC
    adc_select_input(2); // Usa o canal 2 do ADC

    float samples[N];
    float power;
    while (true)
    {
        for (int i = 0; i < N; i++)
        {
            uint16_t raw = adc_read();
            samples[i] = (float)raw / 4095.0; // Normaliza a leitura (0 a 1)
                                              // samples[i] = adc_read();
            sleep_us(1000000 / SAMPLE_RATE);  // Ajusta a taxa de amostragem
        }

        power = goertzel(samples, N, TARGET_FREQ, SAMPLE_RATE);
        if (power > 50.0)
            printf("Power at %0.1f Hz: %f\n", TARGET_FREQ, power);
    }
}
