#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>

#include "botoes.h"
#include "ssd1306.h"
#include "matriz.h"
#include "potenciometro.h"
#include "web.h"
#include "rgb.h"
#include "buzzer.h"

// --- DEFINES E VARIÁVEIS GLOBAIS ---
#define WIFI_SSID           "WIFI"
#define WIFI_PASS           "SENHA"
#define NIVEL_MAXIMO        80
#define NIVEL_MINIMO        0
#define CAPACIDADE_TANQUE_ML 5000
#define DEBOUNCE_US         200000

absolute_time_t ultima_troca = {0};
ssd1306_t ssd;
volatile bool seguranca_ativa = false; 
volatile bool enchendo = false;      // Estado da bomba: true para enchendo
volatile bool esvaziando = false;    // Estado da bomba: true para esvaziando
volatile bool bomba_ligada = false;  // Estado da bomba: true para ligada, false para desligada
int nivel_agua;
int limite_minimo = NIVEL_MINIMO;
int limite_maximo = NIVEL_MAXIMO;                      // Variável global para armazenar o nível de água
volatile bool modo_display = 1;      // Alterna o display entre informações do tanque/bomba e conexão remota

// --- ASSINATURA DAS FUNÇÕES ---
void gpio_irq_handler(uint gpio, uint32_t events);
void seguranca_enchimento_automatico(void);
void info_display(bool modo_display, char ip_str[]);
void inicializar_perifericos(void);
void atualizar_nivel_agua(void);
void atualizar_feedback_bomba(void);

// --- FUNÇÃO PRINCIPAL ---
/**
 * Função principal do sistema.
 * Inicializa periféricos, conecta ao Wi-Fi, inicializa webserver e executa o loop principal.
 */
int main() {
    inicializar_perifericos();

    gpio_set_irq_enabled_with_callback(BOTAO_5, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_6, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    char ip_str[16];
    if (inicializar_wifi(ip_str, WIFI_SSID, WIFI_PASS) != 0) {
        sleep_ms(5000); // Espera 5 segundos antes de encerrar para dar tempo de conectar ao serial monitor
        printf("Falha ao inicializar Wi-Fi. Encerrando...\n");
        return -1;
    }

    init_web();

    while (true) {
        atualizar_nivel_agua();
        atualizar_feedback_bomba();
        seguranca_enchimento_automatico();
        matriz_atualizar_tanque(nivel_agua, limite_maximo);
        cyw43_arch_poll();
        info_display(modo_display, ip_str);
        sleep_ms(100);
    }

    cyw43_arch_deinit();
    return 0;
}

// --- FUNÇÕES AUXILIARES ---
/**
 * Inicializa todos os periféricos do sistema:
 * botões, matriz de LEDs, potenciômetro, display OLED, LEDs RGB e buzzer.
 */
void inicializar_perifericos(void) {
    stdio_init_all();
    init_botoes();
    init_matriz();
    init_potenciometro();
    configurar_leds();
    configurar_buzzer();
    init_display(&ssd);
}

/**
 * Lê o valor do potenciômetro e atualiza a variável global nivel_agua.
 * O valor é convertido para uma escala de 0 a 100 (%).
 */
void atualizar_nivel_agua(void) {
    read_potenciometro();
    // Converte o valor ADC para porcentagem
    int adc_percent = (adc_value_x * 100) / 4095;

    // Define os limites físicos do potenciômetro
    const int ADC_MIN = 60; // Cheio (100%)
    const int ADC_MAX = 81; // Vazio (0%)

    if (adc_percent >= ADC_MAX) {
        nivel_agua = 0; // Vazio
    } else if (adc_percent <= ADC_MIN) {
        nivel_agua = 100; // Cheio
    } else {
        // Mapeia linearmente a faixa [ADC_MAX, ADC_MIN] para [0, 100]
        nivel_agua = ((ADC_MAX - adc_percent) * 100) / (ADC_MAX - ADC_MIN);
    }
}

/**
 * Atualiza o estado da bomba com base no nível de água e nos botões pressionados.
 * Controla o LED RGB, buzzer e matriz de LEDs conforme o estado do sistema.
 */
void atualizar_feedback_bomba(void) {
    // Controle de limites de nível de água
    if (nivel_agua < limite_minimo || nivel_agua > limite_maximo) {
            parar_buzzer();
            set_rgb(true, false, false);
            tocar_buzzer_alerta();
    }
    // Se estiver enchendo ou esvaziando, pisca LED amarelo com bip
    else if (esvaziando || enchendo) {
        parar_buzzer();
        piscar_amarelo_com_bipe();
    }
    // Se não tudo está normal
    else {
        parar_buzzer();
        set_rgb(false, true, false);
    }
}

/**
 * Handler de interrupção dos botões.
 * Realiza debounce e alterna modos ou aciona/desliga bomba conforme o botão pressionado.
 */
void gpio_irq_handler(uint gpio, uint32_t events) {
    absolute_time_t agora = get_absolute_time();
    if (absolute_time_diff_us(ultima_troca, agora) > DEBOUNCE_US) {
        if (gpio == BOTAO_JOYSTICK) { // Alterna modo do display
            modo_display = !modo_display;
        }
        if (seguranca_ativa) {
            // Ignora interrupções que controlam as bombas enquanto a segurança está ativa
            return;
        }
        if (gpio == BOTAO_5 && !esvaziando && nivel_agua < limite_maximo) {
            enchendo = !enchendo;
            bomba_ligada = !bomba_ligada;
            gerar_onda_A();
        }
        else if (gpio == BOTAO_6 && !enchendo && nivel_agua > limite_minimo) {
            esvaziando = !esvaziando;
            bomba_ligada = !bomba_ligada;
            gerar_onda_B();
        }
        ultima_troca = agora;
    }
}

/**
 * Função de segurança: enche automaticamente se o nível cair abaixo do mínimo.
 * Desliga a bomba ao atingir o nível mínimo ou máximo.
 */
void seguranca_enchimento_automatico(void) {
    if (nivel_agua < limite_minimo && !enchendo) {
        if (esvaziando) {
            esvaziando = false; // Desliga a bomba de esvaziamento se estava esvaziando
            gerar_onda_B(); // Desliga a bomba de esvaziamento
        }
        else{
            enchendo = true;
            bomba_ligada = true;
            seguranca_ativa = true;
            gerar_onda_A(); // Liga a bomba de encher
        }
    }
    else if (nivel_agua > limite_maximo && !esvaziando) {
        if(enchendo){
            enchendo = false; // Desliga a bomba de encher se estava enchendo
            gerar_onda_A(); // Desliga a bomba de encher
        }
        else{
            esvaziando = true;
            bomba_ligada = true;
            seguranca_ativa = true;
            gerar_onda_B(); // Liga a bomba de esvaziamento
        }
    }

    if(seguranca_ativa){
        if (nivel_agua >= limite_minimo && enchendo) {
            enchendo = false;
            bomba_ligada = false;
            seguranca_ativa = false;
            gerar_onda_A(); // Desliga a bomba de encher quando atinge o nível mínimo novamente
        }
        else if (nivel_agua <= limite_maximo && esvaziando) {
            esvaziando = false;
            bomba_ligada = false;
            seguranca_ativa = false;
            gerar_onda_B(); // Desliga a bomba de esvaziamento quando atinge o nível máximo novamente
        }
    }
}

/**
 * Atualiza o display OLED com informações do sistema.
 * Se modo_display == 0, mostra status da bomba e nível de água.
 * Se modo_display == 1, mostra status do Wi-Fi e IP.
 */
void info_display(bool modo_display, char ip_str[]) {
    if (modo_display == 0) { // Infos da bomba
        char buffer[32];
        bool cor = true;
        ssd1306_fill(&ssd, false);

        // Moldura principal
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);

        // Linhas horizontais para separar as seções
        ssd1306_line(&ssd, 3, 25, 123, 25, cor); // Linha superior
        ssd1306_line(&ssd, 3, 37, 123, 37, cor); // Linha intermediária

        // Linha vertical para separar as duas colunas inferiores
        ssd1306_line(&ssd, 63, 38, 63, 60, cor);

        // Título centralizado
        ssd1306_draw_string(&ssd, "CEPEDI  TIC37", 8, 6);
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);

        // Estado da bomba centralizado na linha do meio
        if (bomba_ligada) {
            snprintf(buffer, sizeof(buffer), "BOMBA - ON ");
        } else {
            snprintf(buffer, sizeof(buffer), "BOMBA - OFF");
        }
        ssd1306_draw_string(&ssd, buffer, 20, 28);

        // Nível em % (coluna esquerda)
        snprintf(buffer, sizeof(buffer), "%3d%%", nivel_agua);
        ssd1306_draw_string(&ssd, "NIVEL", 17, 40);
        ssd1306_draw_string(&ssd, buffer, 15, 53);

        // Volume em ml (coluna direita)
        snprintf(buffer, sizeof(buffer), "%dml", nivel_agua * (CAPACIDADE_TANQUE_ML / 100));
        ssd1306_draw_string(&ssd, "VOLUME", 69, 40);
        ssd1306_draw_string(&ssd, buffer, 70, 53);

        ssd1306_send_data(&ssd);
    }
    else { // Infos do wifi

        char buffer[32]; // Buffer para armazenar as strings que serão desenhadas no display
        bool cor = true; // Cor do texto (true para branco, false para preto)
        ssd1306_fill(&ssd, false); // Limpa o display com fundo preto

        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha

        ssd1306_draw_string(&ssd, "CEPEDI  TIC37", 8, 6);
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
        ssd1306_draw_string(&ssd, "WIFI - ON - IP", 7, 28);
        snprintf(buffer, sizeof(buffer), "%s", ip_str);
        ssd1306_draw_string(&ssd, buffer, 11, 45);

        ssd1306_send_data(&ssd);    // Envia os dados para o display OLED
    }
}
