#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "potenciometro.h"

#define ADC_PIN 28  // GPIO compartilhada com o microfone na BitDogLab.

uint16_t adc_value_x; // Variável global para armazenar o valor lido do ADC

// Inicializa o ADC para leitura do potenciômetro
void init_potenciometro(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(2);  // Seleciona o canal ADC 0
}

// Lê e retorna o valor atual do potenciômetro (ex: 0~4095)
uint16_t read_potenciometro(void) {
    adc_value_x = adc_read();
    return adc_value_x;  // Retorna o valor lido do ADC
}