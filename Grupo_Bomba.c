#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include <stdio.h>

#define WIFI_SSID "SSID_WIFI"
#define WIFI_PASS "SENHA_WIFI"
#define NIVEL_AGUA 26 // Pino ADC para o nível de água (ALTERAR CONFORME NECESSÁRIO)

int limite_maximo = 100;
int limite_minimo = 10;

bool volatile enchendo = false; // Estado da bomba: true para enchendo, false para esvaziando
bool volatile bomba_ligada = false; // Estado da bomba: true para ligada, false para desligada
int nivel_agua = 0; // Nível de água atual (0-100)

// HTML da interface web
const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><meta charset=UTF-8><title>Controle Nível Água</title>"
    "<style>body{font-family:sans-serif;text-align:center;padding:10px;background:#e6e1e1}"
    ".bar{width:50%;background:#ddd;border-radius:10px;overflow:hidden;margin:0 auto 15px;height:40px}"
    ".fill{height:100%;transition:width .3s}#agua{background:#2196F3}"
    ".lbl{font-weight:bold;font-size:16px;margin:15px 0 5px;display:block}"
    ".sec{display:flex;justify-content:center;gap:20px;margin-top:30px;flex-wrap:wrap}"
    ".box{background:#fff;padding:10px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.1);min-width:150px}"
    ".box-info{background:#fff;padding:10px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.1);min-width:200px}"
    ".inp{padding:6px 8px;font-size:14px;width:100px;border:1px solid #ddd;border-radius:4px;margin-bottom:8px}"
    ".btn{background:#2196F3;color:#fff;padding:8px 16px;font-size:14px;border:0;border-radius:6px;cursor:pointer;width:100%}"
    ".max{background:#f44336}.min{background:#4CAF50}"
    "</style><script>"
    "var cMin=10,cMax=100;"
    "function setMax(){var e=document.getElementById('lmax'),v=+e.value;if(v<=cMin||v>100){alert('Máximo deve ser entre '+(cMin+1)+' e 100');return}fetch('/config/set_limitMax/'+v);e.value=''}"
    "function setMin(){var e=document.getElementById('lmin'),v=+e.value;if(v>=cMax||v<0){alert('Mínimo deve ser entre 0 e '+(cMax-1));return}fetch('/config/set_limitMin/'+v);e.value=''}"
    "function update(){fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('estado').innerText=d.bomba_ligada?(d.estado?'Enchendo':'Esvaziando'):'Parada';document.getElementById('nivel').innerText=d.nivel_agua+' %';document.getElementById('vol').innerText=(d.nivel_agua/100*5).toFixed(2)+' L';document.getElementById('agua').style.width=d.nivel_agua+'%';document.getElementById('vmax').innerText=d.limite_max+' %';document.getElementById('vmin').innerText=d.limite_min+' %';cMax=d.limite_max;cMin=d.limite_min})}"
    "setInterval(update,500)"
    "</script></head><body>"
    "<h1>Controle de Nível de água com Interface Web</h1>"
    "<p>Estado da bomba: <span id=estado>--</span></p>"
    "<p class=lbl>Nível de água: <span id=nivel>--</span> / Volume: <span id=vol>--</span></p>"
    "<div class=bar><div id=agua class=fill></div></div><hr>"
    "<div class=sec>"
    "<div class=box><p class=lbl>Definir Limite Máximo:</p>"
    "<input type=number id=lmax min=1 max=100 class=inp placeholder='> Mínimo'>"
    "<button class='btn max' onclick=setMax()>Definir Máximo</button></div>"
    "<div class=box-info><p class=lbl>Limite Máximo: <span id=vmax>--</span></p>"
    "<p class=lbl>Limite Mínimo: <span id=vmin>--</span></p></div>"
    "<div class=box><p class=lbl>Definir Limite Mínimo:</p>"
    "<input type=number id=lmin min=0 max=99 class=inp placeholder='< Máximo'>"
    "<button class='btn min' onclick=setMin()>Definir Mínimo</button></div>"
    "</div></body></html>";


// Estrutura para manter o estado da conexão HTTP
struct http_state {
    char response[4096];
    size_t len;
    size_t sent;
};


// Protótipos das funções
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
static void start_http_server(void);

// FUNÇÃO PRINCIPAL
int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(NIVEL_AGUA);

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

    start_http_server();

    while (true) {
        cyw43_arch_poll();

        // ALTERAR COM A PORTA CORRESPONDENTE AO ADC
        adc_select_input(0);

        // VALOR LIDO PELO POTENCIOMETRO
        nivel_agua = adc_read();

        nivel_agua = (nivel_agua * 100) / 4095; 

        
        printf("[DEBUG] Nível de água: %d\n", nivel_agua);
        sleep_ms(500);
    }

    cyw43_arch_deinit();
    return 0;
}

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs) {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    if (strstr(req, "GET /status")) {
        // ESTE BLOCO GERA O JSON PARA A INTERFACE WEB
        // AS VARIÁVEIS USADAS AQUI SÃO:
        // - enchendo: estado atual (enchendo/esvaziando)
        // - nivel_agua: nível atual do reservatório
        // - limite_maximo: limite superior configurado
        // - limite_minimo: limite inferior configurado  
        // - bomba_ligada: se a bomba está ligada ou não
    
        // ALTERAR COM A PORTA CORRESPONDENTE AO ADC
        adc_select_input(0);

        // VALOR LIDO PELO POTENCIOMETRO
        nivel_agua = adc_read();

        nivel_agua = (nivel_agua * 100) / 4095; // Converte para porcentagem (0-100)

        // CRIAÇÃO DO JSON
        char json_payload[96];
        int json_len = snprintf(json_payload, sizeof(json_payload),
        "{\"estado\":%d,\"nivel_agua\":%d,\"limite_max\":%d, \"limite_min\":%d, \"bomba_ligada\":%d}\r\n",
        enchendo, nivel_agua, limite_maximo, limite_minimo, bomba_ligada);

        printf("[DEBUG] JSON: %s\n", json_payload);
        
        hs->len = snprintf(hs->response, sizeof(hs->response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        json_len, json_payload);

    }
    else if (strstr(req, "GET /config/set_limitMax/")) {
        // ESTE BLOCO PROCESSA MUDANÇAS NO LIMITE MÁXIMO VINDAS DA INTERFACE


        char* pos = strstr(req, "/config/set_limitMax/") + strlen("/config/set_limitMax/");

        // Captura o número ignorando o restante da string
        char valor_str[16] = {0};
        int i = 0;
        while (pos[i] != ' ' && pos[i] != '\r' && pos[i] != '\n' && pos[i] != '\0' && i < 15) {
            valor_str[i] = pos[i];
            i++;
        }
        valor_str[i] = '\0';

        int novo_limite = atoi(valor_str);
        limite_maximo = novo_limite;

        printf("[DEBUG] Novo limite máximo: %d\n", limite_maximo);

        const char *txt = "Limite máximo atualizado";
        hs->len = snprintf(hs->response, sizeof(hs->response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        (int)strlen(txt), txt);
    }
    else if (strstr(req, "GET /config/set_limitMin/")) {
        // ESTE BLOCO PROCESSA MUDANÇAS NO LIMITE MÍNIMO VINDAS DA INTERFACE

        char* pos = strstr(req, "/config/set_limitMin/") + strlen("/config/set_limitMin/");

        // Captura o número ignorando o restante da string
        char valor_str[16] = {0};
        int i = 0;
        while (pos[i] != ' ' && pos[i] != '\r' && pos[i] != '\n' && pos[i] != '\0' && i < 15) {
            valor_str[i] = pos[i];
            i++;
        }
        valor_str[i] = '\0';

        int novo_limite = atoi(valor_str);
        limite_minimo = novo_limite;

        printf("[DEBUG] Novo limite mínimo: %d\n", limite_minimo);

        const char *txt = "Limite mínimo atualizado";
        hs->len = snprintf(hs->response, sizeof(hs->response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        (int)strlen(txt), txt);
    }
    else {
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(HTML_BODY), HTML_BODY);
    }

    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);

    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}