#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "buzzer.h"

#define BUZZER_PIN 10

static uint slice_num;

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
