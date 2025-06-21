#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "rgb.h"

#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER_PIN 10 // Usado para PWM do bip

static uint slice_num;

void configurar_leds() {
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    set_rgb(false, false, false);

    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice_num, 12500);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 6250);
    pwm_set_enabled(slice_num, false);
}

void set_rgb(bool r, bool g, bool b) {
    gpio_put(LED_R, r);
    gpio_put(LED_G, g);
    gpio_put(LED_B, b);
}

void piscar_amarelo_com_bipe() {
    set_rgb(true, true, false);
    pwm_set_enabled(slice_num, true);
    sleep_ms(150);
    set_rgb(false, false, false);
    pwm_set_enabled(slice_num, false);
    sleep_ms(150);
}
