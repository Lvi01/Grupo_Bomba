# Grupo_Bomba – Controle Inteligente de Nível de Água com Raspberry Pi Pico W

Este projeto implementa um sistema embarcado completo para **monitoramento e controle do nível de água em um tanque**, utilizando o Raspberry Pi Pico W (BitDogLab RP2040) e diversos periféricos. O sistema oferece **controle manual e medidas automáticas de segurança**, além de uma interface web responsiva para monitoramento e configuração remota.

---

## **Funcionalidades Principais**

- **Leitura precisa do nível de água** via potenciômetro acoplado a uma boia, com calibração para a faixa física real do sensor.
- **Controle de duas bombas** (enchimento e esvaziamento) por relés, com acionamento manual (botões físicos ou web) e lógica de segurança automática para evitar estados inválidos.
- **Feedback visual e sonoro**: matriz de LEDs, display OLED, LED RGB e buzzer sinalizam o estado do sistema e alertas.
- **Interface web**: monitoramento em tempo real, ajuste dos limites de operação e visualização do estado das bombas.
- **Display OLED**: alternância entre informações do tanque e status da conexão Wi-Fi/IP.
- **Lógica robusta de segurança**: impede enchimento/esvaziamento fora dos limites, nunca permite ambos os modos ativos simultaneamente, e corrige automaticamente estados inválidos.

---

## **Arquitetura e Organização dos Arquivos**

```
Grupo_Bomba/
│
├── Grupo_Bomba.c           # Arquivo principal: lógica central, inicialização, loop principal, display
├── CMakeLists.txt          # Configuração do projeto para build com CMake
├── readme.md               # Este arquivo
│
└── lib/
    ├── botoes/             # Controle dos botões físicos e relés
    │   ├── botoes.c
    │   └── botoes.h
    ├── buzzer/             # Controle do buzzer (alerta sonoro)
    │   ├── buzzer.c
    │   └── buzzer.h
    ├── matriz/             # Controle da matriz de LEDs (nível gráfico do tanque)
    │   ├── matriz.c
    │   ├── matriz.h
    │   └── matriz.pio
    ├── potenciometro/      # Leitura do potenciômetro (nível de água)
    │   ├── potenciometro.c
    │   └── potenciometro.h
    ├── rgb/                # Controle do LED RGB (feedback visual)
    │   ├── rgb.c
    │   └── rgb.h
    ├── ssd1306/            # Controle do display OLED via I2C
    │   ├── ssd1306.c
    │   ├── ssd1306.h
    │   └── font.h
    └── web/                # Interface web e conexão Wi-Fi
        ├── web.c
        ├── web.h
        └── index.html
```

---

## **Descrição dos Periféricos e Funcionalidade**

### **ADC (Potenciômetro)**
- O potenciômetro de 10K, acoplado a uma boia, fornece uma leitura analógica proporcional ao nível de água.
- O ADC do RP2040 converte esse valor para digital, calibrado para mapear a faixa física real do sensor (ex: 81% = vazio, 60% = cheio).
- O valor é convertido em porcentagem (0–100%) e volume (ml) para uso em toda a lógica do sistema.

### **Relés (GPIO 16 e 17)**
- Controlam as bombas de enchimento e esvaziamento.
- Acionamento manual via botões físicos (BOTAO_5 e BOTAO_6) ou interface web.
- Lógica de segurança impede acionamento simultâneo ou fora dos limites definidos.

### **Matriz de LEDs**
- Representação gráfica do nível do tanque em tempo real.
- Controlada por PIO para eficiência e atualização rápida.

### **Buzzer e LED RGB**
- **Buzzer:** Alerta sonoro quando o nível ultrapassa os limites.
- **LED RGB:**  
  - Verde: operação normal  
  - Vermelho: alerta de limite  
  - Amarelo piscando: bomba em operação

### **Display OLED (I2C)**
- Exibe informações do tanque (nível, volume, estado da bomba) e status da conexão Wi-Fi/IP.
- Layout gráfico com retângulos, linhas e informações centralizadas para melhor visualização.
- Alternância de modo feita pelo botão do joystick.

### **Wi-Fi e Interface Web**
- O módulo CYW43 do Pico W conecta o sistema à rede Wi-Fi.
- Servidor HTTP integrado exibe painel web responsivo:
  - Visualização do nível de água (barra gráfica, valor em % e litros)
  - Estado da bomba (enchendo, esvaziando, parada)
  - Ajuste remoto dos limites mínimo e máximo
  - Atualização em tempo real (AJAX)

---

## **Lógica de Segurança e Robustez**

- **Controle majoritariamente manual**, mas com medidas automáticas de segurança:
  - Nunca permite encher/esvaziar fora dos limites.
  - Nunca permite ambos os modos ativos simultaneamente.
  - Corrige automaticamente estados inválidos, mesmo em caso de erro humano ou comando remoto inadequado.
- **Feedback visual/sonoro** imediato para qualquer situação de alerta.
- **Sincronização entre web, botões e lógica embarcada** para garantir consistência do estado do sistema.

---

## **Como usar**

1. **Monte o hardware** conforme o esquema (Pico W, potenciômetro, relés, matriz de LEDs, buzzer, LED RGB, display OLED).
2. **Configure o Wi-Fi** editando as constantes `WIFI_SSID` e `WIFI_PASS` em `Grupo_Bomba.c`.
3. **Compile e grave o firmware** no Pico W.
4. **Acesse a interface web** pelo IP mostrado no display OLED após conexão.
5. **Monitore e controle** o tanque tanto localmente (botões, display, LEDs) quanto remotamente (web).

---

**Desenvolvido por:**  
Levi, Jabson, Henrique e Rodrigo – EmbarcaTech - 06/2025
