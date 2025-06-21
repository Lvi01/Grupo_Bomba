#ifndef POTENCIOMETRO_H
#define POTENCIOMETRO_H

#include <stdint.h>

#define ADC_PIN 28  // GPIO compartilhada com o microfone na BitDogLab.

// Torna adc_value_x visível para outros arquivos
extern uint16_t adc_value_x;

// Inicializa o ADC para leitura do potenciômetro
void init_potenciometro(void);

// Lê e retorna o valor atual do potenciômetro (ex: 0~4095)
uint16_t read_potenciometro(void);

#endif // POTENCIOMETRO_H