#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#include "botoes.h" // Biblioteca para controle dos botões
#include "ssd1306.h" // Biblioteca para controle do display OLED -- FUNCIONALIDADE NÃO IMPLEMENTADA
#include "matriz.pio.h" // Programa PIO para a matriz de LEDs
#include "matriz.h" // Biblioteca para controle da matriz de LEDs
#include "potenciometro.h" // Biblioteca para controle do potenciômetro

// --- DEFINES E VARIÁVEIS GLOBAIS ---
int limite_maximo = 100;
int limite_minimo = 10;

volatile bool seguranca_ativa = false; 
volatile bool enchendo = false; // Estado da bomba: true para enchendo
volatile bool esvaziando = false; // Estado da bomba: true para esvaziando
volatile bool bomba_ligada = false; // Estado da bomba: true para ligada, false para desligada

absolute_time_t ultima_troca = {0};

// --- ASSINATURA DAS FUNÇÕES ---
void gpio_irq_handler(uint gpio, uint32_t events);
void seguranca_enchimento_automatico(void);

// --- FUNÇÃO PRINCIPAL ---
int main()
{
    stdio_init_all();
    init_botoes();
    init_matriz();
    init_potenciometro();

    gpio_set_irq_enabled_with_callback(BOTAO_5, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_6, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        read_potenciometro(); // Atualiza adc_value_x
        seguranca_enchimento_automatico();
        matriz_atualizar_tanque(adc_value_x, limite_maximo);

        printf("ADC Value: %d, Enchendo: %d, Esvaziando: %d, Bomba Ligada: %d\n",
               adc_value_x, enchendo, esvaziando, bomba_ligada);
        // Outras lógicas do loop principal...
        sleep_ms(100);
    }
}

// --- IMPLEMENTAÇÃO DAS FUNÇÕES ---

// Handler de interrupção dos botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (seguranca_ativa) {
        // Ignora interrupções enquanto a segurança está ativa
        return;
    }
    absolute_time_t agora = get_absolute_time();
    if (absolute_time_diff_us(ultima_troca, agora) > 200000) {
        // Controle para ENCHER: só permite se não estiver esvaziando e não atingiu o nível máximo
        if (gpio == BOTAO_5 && !esvaziando && adc_value_x < limite_maximo) {
            gerar_onda_A();
            enchendo = !enchendo;
            bomba_ligada = !bomba_ligada;
            ultima_troca = agora;
        }
        // Controle para ESVAZIAR: só permite se não estiver enchendo e não atingiu o nível mínimo
        else if (gpio == BOTAO_6 && !enchendo && adc_value_x > limite_minimo) {
            gerar_onda_B();
            esvaziando = !esvaziando;
            bomba_ligada = !bomba_ligada;
            ultima_troca = agora;
        }
        // Se não pode acionar, apenas ignora o comando
    }
}

// Função de segurança: enche automaticamente se o nível cair abaixo do mínimo
void seguranca_enchimento_automatico(void) {
    if (adc_value_x < limite_minimo && !enchendo) {
        enchendo = true;
        bomba_ligada = true;
        seguranca_ativa = true;
        gerar_onda_A(); // Liga a bomba
    }
    // Para a bomba quando atingir o mínimo desejado
    else if (adc_value_x >= limite_minimo && enchendo) {
        enchendo = false;
        bomba_ligada = false;
        seguranca_ativa = false;
        gerar_onda_A(); // Desliga a bomba
    }
}
