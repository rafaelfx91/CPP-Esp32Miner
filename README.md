⚡ ESP32 Crypto Miner - Mineração SHA-256 com Interface Web
<img src="https://img.shields.io/badge/ESP32-NodeMCU-green?style=for-the-badge&logo=espressif" /> <img src="https://img.shields.io/badge/Platform-Arduino_IDE-blue?style=for-the-badge&logo=arduino" /> <img src="https://img.shields.io/badge/SHA--256-Mining-orange?style=for-the-badge&logo=bitcoin" /> <img src="https://img.shields.io/badge/Web-Interface-success?style=for-the-badge&logo=html5" /> <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" />

Sistema completo de mineração cryptocurrency com ESP32, interface web responsiva e monitoramento em tempo real via LEDs.
📋 Índice
    🔧 Funcionalidades
    🛠️ Hardware Requerido
    📦 Dependências
    ⚙️ Configuração
    🔌 Conexões dos LEDs
    🌐 Interface Web
    💻 Código Principal
    🚀 Instalação
    📊 Monitoramento
    🛠️ Solução de Problemas

🔧 Funcionalidades
<img src="https://img.shields.io/badge/Multi_Mode-STA_+_AP-blue?style=flat" /> <img src="https://img.shields.io/badge/Real_Time_Monitoring-green?style=flat" /> <img src="https://img.shields.io/badge/Web_Interface-responsive?style=flat" /> <img src="https://img.shields.io/badge/SHA--256-Accelerated-orange?style=flat" />
    Mineração Real SHA-256 com acelerador hardware
    Interface Web Responsiva com dark mode
    Sistema Dual Mode (STA + Access Point)
    Monitoramento por LEDs em tempo real
    Configuração via Web sem recompilação
    Logs de Mineração persistentes
    Reconexão Automática à pool de mineração

🛠️ Hardware Requerido
Componente	Quantidade	Especificações
<img src="https://img.shields.io/badge/ESP32-NodeMCU-important" />	1x	ESP32 DevKit ou NodeMCU-32S
<img src="https://img.shields.io/badge/LED_Verde-5mm-success" />	2x	Para WiFi e Shares
<img src="https://img.shields.io/badge/LED_Amarelo-5mm-warning" />	1x	Para Modo AP
<img src="https://img.shields.io/badge/Resistores_220Ω-1/4W-inactive" />	4x	Para os LEDs
<img src="https://img.shields.io/badge/Cabo_USB-Micro_USB-blue" />	1x	Alimentação e programação
📦 Dependências
<img src="https://img.shields.io/badge/ArduinoJSON-6.x-blue" /> <img src="https://img.shields.io/badge/WebServer-ESP32-green" /> <img src="https://img.shields.io/badge/SPIFFS-File_System-orange" /> <img src="https://img.shields.io/badge/WiFi-Multi_Mode-yellow" />
cpp

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"

⚙️ Configuração
Configurações Padrão
cpp

String wifi_ssid = "a";
String wifi_password = "a";
String trx_wallet = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";
String pool_host = "sha256.unmineable.com";
String pool_port = "3333";
String worker_name = "esp32-miner#cub7-5a3h";
String coin_type = "TRX";

🔌 Conexões dos LEDs
Pino ESP32	LED	Cor	Função
D18	<img src="https://img.shields.io/badge/LED_AP-Amarelo-yellow" />	Amarelo	Modo Access Point
D19	<img src="https://img.shields.io/badge/LED_WiFi-Verde-green" />	Verde	WiFi Conectado
D22	<img src="https://img.shields.io/badge/LED_Mining-Verde-green" />	Verde	Minerando Ativo
D23	<img src="https://img.shields.io/badge/LED_Shares-Verde-green" />	Verde	Shares Aceitos
🌐 Interface Web
<img src="https://img.shields.io/badge/Dark_Mode-Enabled-dark" /> <img src="https://img.shields.io/badge/Responsive-Design-blue" /> <img src="https://img.shields.io/badge/Real_Time-Updates-green" />
Funcionalidades da Interface:
    Status em Tempo Real dos LEDs e conexões
    Configuração WiFi com scanner de redes
    Estatísticas de Mineração (hashrate, shares, etc.)
    Controles de Sistema (reiniciar, apagar logs)
    Modo AP para configuração inicial

Acesso:
    Modo STA: http://[IP-DO-ESP32]
    Modo AP: http://192.168.4.1

💻 Código Principal
Estrutura do Projeto
text

ESP32-Miner/
├── src/
│   ├── miner_core.ino      # Código principal
│   └── sha256_acelerado.h  # Acelerador SHA-256
├── data/
│   └── config.json         # Configurações salvas
└── logs/
    └── miner_log.txt       # Logs de mineração

Funções Principais
cpp

void setupLEDs()           // Inicializa sistema de LEDs
void connectToWiFi()       // Conexão WiFi dual mode  
void connectToMiningPool() // Conexão com pool
void processMiningJob()    // Processamento SHA-256
void handlePoolResponse()  // Respostas da pool
void setupWebServer()      // Servidor web

🚀 Instalação
1. Preparação do Ambiente
<img src="https://img.shields.io/badge/Arduino_IDE-Required-blue" /> <img src="https://img.shields.io/badge/ESP32_Boards-Installed-green" />

    Instale Arduino IDE
    Adicione suporte ao ESP32
    Instale as bibliotecas necessárias
   
3. Upload do Código
cpp
// 1. Conecte o ESP32 via USB
// 2. Selecione a porta COM
// 3. Faça upload do código
// 4. Abra Serial Monitor (115200 baud)

3. Configuração Inicial
    Conecte na rede "ESP32-Miner-Config"
    Acesse http://192.168.4.1
    Configure WiFi e dados da pool
    Salve e reinicie

📊 Monitoramento
Estatísticas em Tempo Real
<img src="https://img.shields.io/badge/Hashrate-Live_Update-orange" /> <img src="https://img.shields.io/badge/Shares-Aceitos/Rejeitados-green" /> <img src="https://img.shields.io/badge/Uptime-Continuo-blue" />
Métrica	Descrição	Atualização
Hashrate	Velocidade de mineração	Tempo real
Shares	Trabalhos aceitos	Imediato
Hashes	Total calculado	Contínuo
Uptime	Tempo online	Segundos
Sistema de LEDs

<img src="https://img.shields.io/badge/D18-AP_Mode-yellow" /> Amarelo Fixo: Modo AP Ativo
<img src="https://img.shields.io/badge/D19-WiFi_Connected-green" /> Verde Fixo: WiFi Conectado
<img src="https://img.shields.io/badge/D22-Mining_Active-green" /> Verde Piscante: Minerando
<img src="https://img.shields.io/badge/D23-Shares_Accepted-green" /> Verde Fixo: Shares > 0
🛠️ Solução de Problemas
Problemas Comuns
<img src="https://img.shields.io/badge/WiFi-Connection_Issues-red" /> - Verifique credenciais WiFi - Force modo AP para reconfigurar<img src="https://img.shields.io/badge/Pool-Connection_Failed-orange" /> - Verifique host e porta da pool - Confirme carteira e worker name<img src="https://img.shields.io/badge/LEDs-Not_Working-yellow" /> - Verifique conexões dos LEDs - Confirme resistores (220Ω)<img src="https://img.shields.io/badge/Web_Interface-Unaccessible-blue" /> - Verifique IP correto - Confirme modo de operação
Comandos de Depuração
cpp

// Monitor Serial (115200 baud)
Serial.println("✅ WiFi Conectado!");
Serial.println("❌ Falha na pool");
Serial.println("⛏️ Minerando ativo");

📈 Performance Esperada
<img src="https://img.shields.io/badge/Hashrate-50--100_H/s-orange" /> <img src="https://img.shields.io/badge/Consumo-3.3V@500mA-blue" /> <img src="https://img.shields.io/badge/Temperatura-40--60°C-red" />
Parâmetro	Valor Típico
Hashrate	50-100 H/s
Consumo	500mA @ 3.3V
Temperatura	40-60°C
Shares/Dia	2-5 (depende da dificuldade)
⚠️ Avisos Importantes
<img src="https://img.shields.io/badge/Educational-Purposes_only-yellow" /> <img src="https://img.shields.io/badge/Not_Profitable-Demonstration-red" /> <img src="https://img.shields.io/badge/Power_Management-Required-orange" />
    Fins Educacionais: Demonstração de tecnologia
    Não Lucrativo: Hashrate muito baixo para lucro
    Gerenciamento Térmico: Monitorar temperatura do ESP32
    Consumo Elétrico: Considerar custos de energia

🔄 Atualizações Futuras
<img src="https://img.shields.io/badge/Features-Planned-green" />
    Suporte a múltiplas pools
    Configuração OTA (Over-The-Air)
    Dashboard móvel
    Logs detalhados via SD Card
    Controle de temperatura automático

📞 Suporte
<img src="https://img.shields.io/badge/Serial_Monitor-Debugging-blue" /> <img src="https://img.shields.io/badge/Web_Interface-Status-green" /> <img src="https://img.shields.io/badge/LEDs-Visual_Feedback-orange" />

Para suporte técnico:
    Verifique LEDs de status
    Consulte Serial Monitor
    Acesse interface web
    Revise configurações salvas

<img src="https://img.shields.io/badge/ESP32-Miner_Project-purple?style=for-the-badge" /> <img src="https://img.shields.io/badge/SHA--256-Optimized-orange?style=for-the-badge" /> <img src="https://img.shields.io/badge/Open_Source-MIT-green?style=for-the-badge" />


