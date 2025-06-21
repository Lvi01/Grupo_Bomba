#include "matriz.h"
#include "build/matriz.pio.h"

#define NUM_LEDS 25 // Total de LEDs na matriz 5x5
#define LED_MATRIX 7 // GPIO 7 para a matriz de LEDs

PIO pio = pio0; 
uint sm;
uint32_t valor_led;

// Inicializa o PIO e o programa para a matriz de LEDs
void init_matriz(void) {
    uint offset = pio_add_program(pio, &final_program);
    sm = pio_claim_unused_sm(pio, true);
    final_program_init(pio, sm, offset, LED_MATRIX);
}

// Mapa da matriz de LEDs (formato da planta)
uint8_t matriz[NUM_LEDS] = {
    0, 4, 4, 4, 0,
    0, 3, 3, 3, 0,
    0, 2, 2, 2, 0,
    0, 1, 1, 1, 0,
    0, 0, 0, 0, 0
};

// Função para converter valores RGB em formato GRB para a matriz de LEDs
uint32_t matrix_grb(double b, double r, double g) {
    unsigned char R = r * 255, G = g * 255, B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// Função para atualizar a matriz conforme o nível do tanque
void matriz_atualizar_tanque(uint16_t nivel_percentual, uint16_t max_percentual) {
    float percentual = (float)nivel_percentual / (float)max_percentual;
    if (percentual > 1.0f) percentual = 1.0f;

    uint8_t nivel = 0;
    if (percentual >= 1.0f) nivel = 4;
    else if (percentual >= 0.75f) nivel = 3;
    else if (percentual >= 0.5f) nivel = 2;
    else if (percentual >= 0.25f) nivel = 1;
    else nivel = 0;

    // Itera de baixo para cima para inverter a representação
    for (int linha = 4; linha >= 0; linha--) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int i = linha * 5 + coluna;
            if (matriz[i] > 0 && matriz[i] <= nivel) {
                valor_led = matrix_grb(0.3, 0.0, 0.0); // Azul
            } else if (matriz[i] == 0) {
                valor_led = matrix_grb(0.1, 0.1, 0.1); // Branco
            } else {
                valor_led = matrix_grb(0.0, 0.0, 0.0); // Desligado
                
            }
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    }
}
