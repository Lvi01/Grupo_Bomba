#include "web.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int limite_minimo;
extern int limite_maximo;

int inicializar_wifi(char *ip_str ,char *WIFI_SSID, char *WIFI_PASS) {
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
    snprintf(ip_str, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    printf("WiFi => Conectado com sucesso!\n IP: %s\n", ip_str);

    return 0; // Retorna 0 para indicar sucesso
}


// HTML da interface web - Página completa com estilos CSS e JavaScript
// Contém formulários para configurar limites e mostra status em tempo real
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

/**
 * Função privada que inicializa e configura o servidor HTTP
 * - Cria um novo PCB (Protocol Control Block) TCP
 * - Faz o bind para a porta 80 (HTTP padrão)
 * - Coloca o servidor em modo de escuta
 * - Define callback para novas conexões
 */
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

/**
 * Função pública para inicializar o módulo web
 * Esta é a função chamada pelo main() para iniciar o servidor HTTP
 */
void init_web(void) {
    // Inicializa o servidor HTTP
    start_http_server();
}

/**
 * - Callback chamado quando dados são enviados com sucesso via TCP
 * - Atualiza o contador de bytes enviados
 * - Fecha a conexão quando todos os dados foram enviados
 * - Libera a memória alocada para o estado HTTP
 */
static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len) {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

/**
 * Callback principal que processa requisições HTTP recebidas
 * - Analisa o tipo de requisição (GET /status, /config, ou página principal)
 * - Gera resposta apropriada (JSON para status, confirmação para config, HTML para página)
 * - Envia resposta de volta ao cliente
 * - Gerencia memória e conexão TCP
 */
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // Se não há dados, fecha a conexão
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

    // Rota para status em JSON - retorna dados atuais do sistema
    if (strstr(req, "GET /status")) {
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
    // Rota para configurar limite máximo - extrai valor da URL e atualiza variável global
    else if (strstr(req, "GET /config/set_limitMax/")) {
        char* pos = strstr(req, "/config/set_limitMax/") + strlen("/config/set_limitMax/");
        char valor_str[16] = {0};
        int i = 0;
        // Extrai o valor numérico da URL
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
    // Rota para configurar limite mínimo - extrai valor da URL e atualiza variável global
    else if (strstr(req, "GET /config/set_limitMin/")) {
        char* pos = strstr(req, "/config/set_limitMin/") + strlen("/config/set_limitMin/");
        char valor_str[16] = {0};
        int i = 0;
        // Extrai o valor numérico da URL
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
    // Rota padrão - retorna a página HTML principal
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

    // Configura callbacks e envia resposta
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);

    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    return ERR_OK;
}

/**
 * Callback chamado quando uma nova conexão TCP é estabelecida
 * - Configura o callback para receber dados HTTP
 * - Retorna ERR_OK para aceitar a conexão
 */
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}