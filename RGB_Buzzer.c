//OBS: Adicionei um ADC para testar a lógica aqui em casa, tava tudo tranquilo porém no projeto vai precisar alterar os nomes das variáveis

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

//definindo as portas do led e buzzer
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define ADC_PIN 28
#define BUZZER_PIN 10

#define RES_POTENCIOMETRO 10000
#define LIM_MIN 1000
#define LIM_MAX 9000

//funções para alterar o estados dos leds (3 funções)
void configurar_leds();
void set_rgb(bool r, bool g, bool b);
void piscar_amarelo_com_bipe();

//funções para alterar o estado do buzzer (3 funções)
void configurar_buzzer();
void tocar_buzzer_alerta();
void parar_buzzer();


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

        // Print do valor de nível
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

void configurar_leds() {
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);

    set_rgb(false, false, false);
}

void set_rgb(bool r, bool g, bool b) {
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}

uint slice_num;

void configurar_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 12500);                   
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 6250); 
    pwm_set_enabled(slice_num, false);
}

void tocar_buzzer_alerta() {
    pwm_set_enabled(slice_num, true);
    sleep_ms(1000); 
    pwm_set_enabled(slice_num, false);
}

void parar_buzzer() {
    pwm_set_enabled(slice_num, false);
}

void piscar_amarelo_com_bipe() {
    set_rgb(true, true, false);       
    pwm_set_enabled(slice_num, true);
    sleep_ms(150);

    set_rgb(false, false, false);     
    pwm_set_enabled(slice_num, false); 
    sleep_ms(150);
}
