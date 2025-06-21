#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "lib/rgb.h"
#include "lib/buzzer.h"

#define ADC_PIN 28
#define RES_POTENCIOMETRO 10000
#define LIM_MIN 1000
#define LIM_MAX 9000

int main() {
    stdio_init_all(); 

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(2);

    configurar_leds();
    configurar_buzzer();

    float nivel;
    uint16_t valor_adc;
    bool enchendo = false;
    bool esvaziando = false;

    while (true) {
        valor_adc = adc_read();
        nivel = (valor_adc * RES_POTENCIOMETRO) / 4095.0;

        printf("Nível: %.2f ohms\n", nivel);
        printf("Valor adc: %.2f \n", valor_adc);

        if (nivel < LIM_MIN) {
            set_rgb(true, false, false);
            tocar_buzzer_alerta();
            enchendo = true;
            esvaziando = false;
        }
        else if (nivel > LIM_MAX) {
            set_rgb(true, false, false);
            tocar_buzzer_alerta();
            enchendo = false;
            esvaziando = true;
        }
        else if (enchendo || esvaziando) {
            piscar_amarelo_com_bipe(); 
        }
        else {
            set_rgb(false, true, false);
            parar_buzzer();
        }

        sleep_ms(300);
    }

    return 0;
}
