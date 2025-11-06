⚡ ESP32 Crypto Miner - Mineração SHA-256 com Interface Web
<img src="https://img.shields.io/badge/ESP32-NodeMCU-green?style=for-the-badge&logo=espressif" /> <br>
<img src="https://img.shields.io/badge/Platform-Arduino_IDE-blue?style=for-the-badge&logo=arduino" /><br>
<img src="https://img.shields.io/badge/SHA--256-Mining-orange?style=for-the-badge&logo=bitcoin" /><br>
<img src="https://img.shields.io/badge/Web-Interface-success?style=for-the-badge&logo=html5" /> <br>
<img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" /><br>

Sistema completo de mineração cryptocurrency com ESP32, interface web responsiva e monitoramento em tempo real via LEDs.<br>
📋 Índice<br>
    🔧 Funcionalidades<br>
    🛠️ Hardware Requerido<br>
    📦 Dependências<br>
    ⚙️ Configuração<br>
    🔌 Conexões dos LEDs<br>
    🌐 Interface Web<br>
    💻 Código Principal<br>
    🚀 Instalação<br>
    📊 Monitoramento<br>
    🛠️ Solução de Problemas<br>

<img src="https://github.com/rafaelfx91/CPP-Esp32Miner/blob/main/nerminer-trx/ESP32%20Diagrama.png" width="400" height="400" alt="Diagrama ESP32"/><br>
<img src="https://github.com/rafaelfx91/CPP-Esp32Miner/blob/main/nerminer-trx/wifi1.png"  width="400" height="400" alt="WIFI1"/><br>
<img src="https://github.com/rafaelfx91/CPP-Esp32Miner/blob/main/nerminer-trx/wifi2.png"  width="400" height="400" alt="WIFI2"/><br>
    
<br>
🔧 Funcionalidades<br><br>
<img src="https://img.shields.io/badge/Multi_Mode-STA_+_AP-blue?style=flat" /> <br>
<img src="https://img.shields.io/badge/Real_Time_Monitoring-green?style=flat" /> <br>
<img src="https://img.shields.io/badge/Web_Interface-responsive?style=flat" /> <br>
<img src="https://img.shields.io/badge/SHA--256-Accelerated-orange?style=flat" /><br>
    Mineração Real SHA-256 com acelerador hardware<br>
    Interface Web Responsiva com dark mode<br>
    Sistema Dual Mode (STA + Access Point)<br>
    Monitoramento por LEDs em tempo real<br>
    Configuração via Web sem recompilação<br>
    Logs de Mineração persistentes<br>
    Reconexão Automática à pool de mineração<br>
<br>
🛠️ Hardware Requerido<br>
Componente	Quantidade	Especificações
<img src="https://img.shields.io/badge/ESP32-NodeMCU-important" />	1x	ESP32 DevKit ou NodeMCU-32S<br>
<img src="https://img.shields.io/badge/LED_Verde-5mm-success" />	2x	Para WiFi e Shares<br>
<img src="https://img.shields.io/badge/LED_Amarelo-5mm-warning" />	1x	Para Modo AP<br>
<img src="https://img.shields.io/badge/Resistores_220Ω-1/4W-inactive" />	4x	Para os LEDs<br>
<img src="https://img.shields.io/badge/Cabo_USB-Micro_USB-blue" />	1x	Alimentação e programação<br>
📦 Dependências<br>
<img src="https://img.shields.io/badge/ArduinoJSON-6.x-blue" /> <br>
<img src="https://img.shields.io/badge/WebServer-ESP32-green" /> <br>
<img src="https://img.shields.io/badge/SPIFFS-File_System-orange" /> <br>
<img src="https://img.shields.io/badge/WiFi-Multi_Mode-yellow" /><br>

cpp<br>
#include <WiFi.h><br>
#include <WebServer.h><br>
#include <ArduinoJson.h><br>
#include "SPIFFS.h"<br>

⚙️ Configuração<br>
Configurações Padrão<br>

cpp<br>
String wifi_ssid = "a";<br>
String wifi_password = "a";<br>
String trx_wallet = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";<br>
String pool_host = "sha256.unmineable.com";<br>
String pool_port = "3333";<br>
String worker_name = "esp32-miner#cub7-5a3h";<br>
String coin_type = "TRX";<br>

🔌 Conexões dos LEDs<br>
Pino ESP32	LED	Cor	Função<br>
D18	<img src="https://img.shields.io/badge/LED_AP-Amarelo-yellow" />	Amarelo	Modo Access Point<br>
D19	<img src="https://img.shields.io/badge/LED_WiFi-Verde-green" />	Verde	WiFi Conectado<br>
D22	<img src="https://img.shields.io/badge/LED_Mining-Verde-green" />	Verde	Minerando Ativo<br>
D23	<img src="https://img.shields.io/badge/LED_Shares-Verde-green" />	Verde	Shares Aceitos<br>
🌐 Interface Web<br>
<img src="https://img.shields.io/badge/Dark_Mode-Enabled-dark" /> <br>
<img src="https://img.shields.io/badge/Responsive-Design-blue" /> <br>
<img src="https://img.shields.io/badge/Real_Time-Updates-green" /><br>
Funcionalidades da Interface:<br>
    Status em Tempo Real dos LEDs e conexões<br>
    Configuração WiFi com scanner de redes<br>
    Estatísticas de Mineração (hashrate, shares, etc.)<br>
    Controles de Sistema (reiniciar, apagar logs)<br>
    Modo AP para configuração inicial<br>

Acesso:<br>
    Modo STA: http://[IP-DO-ESP32]<br>
    Modo AP: http://192.168.4.1<br>
<br>
💻 Código Principal<br>
Estrutura do Projeto<br>
text<br>
<br>
ESP32-Miner/<br>
├── src/<br>
│   ├── miner_core.ino      # Código principal<br>
│   └── sha256_acelerado.h  # Acelerador SHA-256<br>
├── data/<br>
│   └── config.json         # Configurações salvas<br>
└── logs/<br>
    └── miner_log.txt       # Logs de mineração<br>
<br>
Funções Principais<br>
cpp<br>
<br>
void setupLEDs()           // Inicializa sistema de LEDs<br>
void connectToWiFi()       // Conexão WiFi dual mode  <br>
void connectToMiningPool() // Conexão com pool<br>
void processMiningJob()    // Processamento SHA-256<br>
void handlePoolResponse()  // Respostas da pool<br>
void setupWebServer()      // Servidor web<br>
<br>
🚀 Instalação<br>
1. Preparação do Ambiente<br>
<img src="https://img.shields.io/badge/Arduino_IDE-Required-blue" /> <br>
<img src="https://img.shields.io/badge/ESP32_Boards-Installed-green" /><br>
<br>
    Instale Arduino IDE<br>
    Adicione suporte ao ESP32<br>
    Instale as bibliotecas necessárias<br>
   <br>
3. Upload do Código<br>
cpp<br>
// 1. Conecte o ESP32 via USB<br>
// 2. Selecione a porta COM<br>
// 3. Faça upload do código<br>
// 4. Abra Serial Monitor (115200 baud)<br>
<br>
3. Configuração Inicial<br>
    Conecte na rede "ESP32-Miner-Config"<br>
    Acesse http://192.168.4.1<br>
    Configure WiFi e dados da pool<br>
    Salve e reinicie<br>

📊 Monitoramento<br>
Estatísticas em Tempo Real<br>
<img src="https://img.shields.io/badge/Hashrate-Live_Update-orange" /> <br>
<img src="https://img.shields.io/badge/Shares-Aceitos/Rejeitados-green" /> <br>
<img src="https://img.shields.io/badge/Uptime-Continuo-blue" /><br>
Métrica	Descrição	Atualização<br>
Hashrate	Velocidade de mineração	Tempo real<br>
Shares	Trabalhos aceitos	Imediato<br>
Hashes	Total calculado	Contínuo<br>
Uptime	Tempo online	Segundos<br>
Sistema de LEDs<br>
<br>
<img src="https://img.shields.io/badge/D18-AP_Mode-yellow" /> Amarelo Fixo: Modo AP Ativo<br>
<img src="https://img.shields.io/badge/D19-WiFi_Connected-green" /> Verde Fixo: WiFi Conectado<br>
<img src="https://img.shields.io/badge/D22-Mining_Active-green" /> Verde Piscante: Minerando<br>
<img src="https://img.shields.io/badge/D23-Shares_Accepted-green" /> Verde Fixo: Shares > 0<br>
🛠️ Solução de Problemas<br>
Problemas Comuns<br>
<img src="https://img.shields.io/badge/WiFi-Connection_Issues-red" /> - Verifique credenciais WiFi - Force modo AP para reconfigurar<img src="https://img.shields.io/badge/Pool-Connection_Failed-orange" /> - Verifique host e porta da pool - Confirme carteira e worker name<img src="https://img.shields.io/badge/LEDs-Not_Working-yellow" /> - Verifique conexões dos LEDs - Confirme resistores (220Ω)<img src="https://img.shields.io/badge/Web_Interface-Unaccessible-blue" /> - Verifique IP correto - Confirme modo de operação<br>
Comandos de Depuração<br>
cpp<br>
<br>
// Monitor Serial (115200 baud)<br>
Serial.println("✅ WiFi Conectado!");<br>
Serial.println("❌ Falha na pool");<br>
Serial.println("⛏️ Minerando ativo");<br>
<br>
📈 Performance Esperada<br>
<img src="https://img.shields.io/badge/Hashrate-50--100_H/s-orange" /> <br>
<img src="https://img.shields.io/badge/Consumo-3.3V@500mA-blue" /> <br>
<img src="https://img.shields.io/badge/Temperatura-40--60°C-red" /><br>
Parâmetro	Valor Típico<br>
Hashrate	50-100 H/s<br>
Consumo	500mA @ 3.3V<br>
Temperatura	40-60°C<br>
Shares/Dia	2-5 (depende da dificuldade)<br>

⚠️ Avisos Importantes<br>
<img src="https://img.shields.io/badge/Educational-Purposes_only-yellow" /> <br>
<img src="https://img.shields.io/badge/Not_Profitable-Demonstration-red" /> <br>
<img src="https://img.shields.io/badge/Power_Management-Required-orange" /><br>
    Fins Educacionais: Demonstração de tecnologia<br>
    Não Lucrativo: Hashrate muito baixo para lucro<br>
    Gerenciamento Térmico: Monitorar temperatura do ESP32<br>
    Consumo Elétrico: Considerar custos de energia<br>
<br>
🔄 Atualizações Futuras<br>
<img src="https://img.shields.io/badge/Features-Planned-green" /><br>
    Suporte a múltiplas pools<br>
    Configuração OTA (Over-The-Air)<br>
    Dashboard móvel<br>
    Logs detalhados via SD Card<br>
    Controle de temperatura automático<br>
<br>
📞 Suporte<br>
<img src="https://img.shields.io/badge/Serial_Monitor-Debugging-blue" /> <br>
<img src="https://img.shields.io/badge/Web_Interface-Status-green" /> <br>
<img src="https://img.shields.io/badge/LEDs-Visual_Feedback-orange" /><br>
<br>
Para suporte técnico:<br>
    Verifique LEDs de status<br>
    Consulte Serial Monitor<br>
    Acesse interface web<br>
    Revise configurações salvas<br>
<br>
<img src="https://img.shields.io/badge/ESP32-Miner_Project-purple?style=for-the-badge" /> <br>
<img src="https://img.shields.io/badge/SHA--256-Optimized-orange?style=for-the-badge" /> <br>
<img src="https://img.shields.io/badge/Open_Source-MIT-green?style=for-the-badge" /><br>
<br>

