#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>

#include "botoes.h" // Biblioteca para controle dos botões
#include "ssd1306.h" // Biblioteca para controle do display OLED -- FUNCIONALIDADE NÃO IMPLEMENTADA
#include "matriz.h" // Biblioteca para controle da matriz de LEDs
#include "potenciometro.h" // Biblioteca para controle do potenciômetro
#include "web.h" // Biblioteca para controle da interface web
#include "rgb.h" // Biblioteca para controle do Led RGB
#include "buzzer.h" // Biblioteca para controle do buzzer


// --- DEFINES E VARIÁVEIS GLOBAIS ---
#define WIFI_SSID "SSID"
#define WIFI_PASS "SENHA"

int limite_maximo = 100;
int limite_minimo = 10;
absolute_time_t ultima_troca = {0};
ssd1306_t ssd;
volatile bool seguranca_ativa = false; 
volatile bool enchendo = false; // Estado da bomba: true para enchendo
volatile bool esvaziando = false; // Estado da bomba: true para esvaziando
volatile bool bomba_ligada = false; // Estado da bomba: true para ligada, false para desligada
int nivel_agua; // Variável global para armazenar o nível de água


// --- ASSINATURA DAS FUNÇÕES ---
void gpio_irq_handler(uint gpio, uint32_t events);
void seguranca_enchimento_automatico(void);

// FUNÇÃO PRINCIPAL
int main() {
    stdio_init_all();

    init_botoes();
    init_matriz();
    init_potenciometro();
    configurar_leds();
    configurar_buzzer();

    gpio_set_irq_enabled_with_callback(BOTAO_5, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_6, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicialização e configuração do Wi-Fi
    if (cyw43_arch_init()) {
        printf("WiFi => FALHA\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    // Conexão à rede Wi-Fi
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("WiFi => ERRO\n");
        sleep_ms(100);
        return -1;
    }

    // Exibe o endereço IP atribuído
    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    printf("WiFi => Conectado com sucesso!\n IP: %s\n", ip_str);

    init_web(); // Inicializa o servidor web
    init_display(&ssd); // Inicializa o display

    ssd1306_fill(&ssd, 0);

    ssd1306_draw_string(&ssd, "BOMBA      OFF", 5, 8);

    ssd1306_send_data(&ssd);

    while (true) {
        read_potenciometro(); // Atualiza adc_value_x
        
        nivel_agua = adc_value_x; // Lê o valor do ADC
        nivel_agua = (nivel_agua * 100) / 4095; // Converte para porcentagem (0-100)
        
        seguranca_enchimento_automatico();
        matriz_atualizar_tanque(nivel_agua, limite_maximo);
        
        cyw43_arch_poll();
            
        printf("[DEBUG] Nível de água: %d\n", nivel_agua);
        sleep_ms(500);
            if (nivel_agua < limite_minimo) {
            set_rgb(true, false, false);
            tocar_buzzer_alerta();
            enchendo = true;
            esvaziando = false;
        }
        else if (nivel_agua > limite_maximo) {
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

        char bomba_string[15];
        if (bomba_ligada){
            sprintf(bomba_string, "BOMBA       ON");
        }
        else{
            sprintf(bomba_string, "BOMBA      OFF");
        }
        ssd1306_draw_string(&ssd, bomba_string, 5, 8);
        ssd1306_send_data(&ssd);

        sleep_ms(300);
    }

    cyw43_arch_deinit();
    return 0;
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
        if (gpio == BOTAO_5 && !esvaziando && nivel_agua < limite_maximo) {
            gerar_onda_A();
            enchendo = !enchendo;
            bomba_ligada = !bomba_ligada;
            ultima_troca = agora;
        }
        // Controle para ESVAZIAR: só permite se não estiver enchendo e não atingiu o nível mínimo
        else if (gpio == BOTAO_6 && !enchendo && nivel_agua > limite_minimo) {
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
    if (nivel_agua < limite_minimo && !enchendo) {
        enchendo = true;
        bomba_ligada = true;
        seguranca_ativa = true;
        gerar_onda_A(); // Liga a bomba
    }
    // Para a bomba quando atingir o mínimo desejado
    else if (nivel_agua >= limite_minimo && enchendo) {
        enchendo = false;
        bomba_ligada = false;
        seguranca_ativa = false;
        gerar_onda_A(); // Desliga a bomba
    }
}
