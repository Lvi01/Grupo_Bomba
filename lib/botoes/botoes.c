#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "botoes.h"
#include "pico/time.h"

#define BOTAO_5 5
#define BOTAO_6 6
#define RELE_16 16
#define RELE_17 17

// Funções de callback para alarmes
static int64_t desligar_rele_16(alarm_id_t id, void *user_data) {
    gpio_put(RELE_16, 1);
    return 0;
}

static int64_t desligar_rele_17(alarm_id_t id, void *user_data) {
    gpio_put(RELE_17, 1);
    return 0;
}

// Função para gerar a onda A (ligar o relé 16)
void gerar_onda_A(void) {
    gpio_put(RELE_16, 0);
    add_alarm_in_ms(200, desligar_rele_16, NULL, false);
}

// Função para gerar a onda B (ligar o relé 17)
void gerar_onda_B(void) {
    gpio_put(RELE_17, 0);
    add_alarm_in_ms(200, desligar_rele_17, NULL, false);
}

// Inicializa os botões e relés
void init_botoes(void) {
    gpio_init(BOTAO_5);
    gpio_set_dir(BOTAO_5, GPIO_IN);
    gpio_pull_up(BOTAO_5);

    gpio_init(BOTAO_6);
    gpio_set_dir(BOTAO_6, GPIO_IN);
    gpio_pull_up(BOTAO_6);

    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);

    gpio_init(RELE_16);
    gpio_set_dir(RELE_16, GPIO_OUT);
    gpio_put(RELE_16, 1);

    gpio_init(RELE_17);
    gpio_set_dir(RELE_17, GPIO_OUT);
    gpio_put(RELE_17, 1);
}
