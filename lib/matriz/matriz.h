#ifndef MATRIZ_H
#define MATRIZ_H

#include <stdint.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Externas para uso em outros arquivos, se necessário
extern PIO pio;
extern uint sm;

// Atualiza a matriz de LEDs conforme o nível do tanque
void matriz_atualizar_tanque(uint16_t nivel_percentual, uint16_t max_percentual);

// Inicializa a matriz de LEDs
void init_matriz(void);

#endif // MATRIZ_H