⚡ ESP32 SHA-256 Crypto Miner

Um minerador de criptomoedas completo baseado no ESP32, com interface web para configuração e monitoramento em tempo real.
🚀 Características Principais
    Mineração Real SHA-256: Implementação otimizada com acelerador de hardware
    Interface Web Responsiva: Controle total via navegador
    Sistema Dual WiFi: Modo STA (mineração) + AP (configuração)
    LEDs Indicadores: Status visual em tempo real
    Configuração Persistente: Salva configurações na memória SPIFFS
    Multi-pool: Compatível com pools Stratum (Unmineable, etc.)
    Multi-moedas: Suporte a TRX, BTC, DOGE e outras via Unmineable

📋 Pré-requisitos
Hardware
    ESP32 (com acelerador SHA-256)
    4x LEDs (para indicadores de status)
    Fonte de alimentação estável
    Conexão WiFi

Software
    Arduino IDE ou PlatformIO
    Bibliotecas:
        WiFi
        WebServer
        ArduinoJson
        SPIFFS

🔧 Instalação
    Clone o repositório:
    git clone https://github.com/seu-usuario/esp32-miner.git

2. Instale as bibliotecas necessárias:
    WiFi
    WebServer
    ArduinoJson (v6.x ou superior)
    SPIFFS

 3. Configure o hardware:
    LED_AP_MODE      → GPIO 18  (Amarelo)
    LED_WIFI_CONNECT → GPIO 19  (Verde)  
    LED_MINING       → GPIO 22  (Verde - Piscante)
    LED_SHARES       → GPIO 23  (Verde - Fixo)

4. Carregue o código para o ESP32

5.Primeira configuração:
        Conecte-se ao WiFi "ESP32-Miner-Config" (senha: 12345678)
        Acesse http://192.168.4.1
        Configure sua rede WiFi e dados da pool


⚙️ Configuração
Via Interface Web

Acesse a interface web para configurar:
    WiFi: SSID e senha da sua rede
    Pool Mining:
        Host: sha256.unmineable.com
        Porta: 3333
    Carteira: Sua carteira TRX (ou outra criptomoeda)
    Worker: Nome do seu minerador
    Coin: Tipo de moeda (TRX, BTC, DOGE, etc.)
    
Configuração Padrão
String wifi_ssid = "";
String wifi_password = "";
String trx_wallet = "";
String pool_host = "";
String pool_port = "";
String worker_name = "";
String coin_type = "";

💡 Sistema de LEDs
LED	GPIO	Cor	Estado	Significado
D18	GPIO18	🟡 Amarelo	Fixo	Modo AP Ativo
D19	GPIO19	🟢 Verde	Fixo	WiFi Conectado
D22	GPIO22	🟢 Verde	Piscante	Minerando Ativamente
D23	GPIO23	🟢 Verde	Fixo	Shares Aceitos


🌐 Interface Web
Status em Tempo Real
    Conexão: Status WiFi e pool
    Mineração: Hashrate, shares, hashes calculados
    Hardware: IP, MAC, tempo online
    LEDs: Status visual dos indicadores

Controles
    🔍 Scan de redes WiFi
    ⚙️ Configuração completa
    🔄 Reinício do minerador
    📶 Ativação modo AP
    🗑️ Limpeza de logs

🔌 API Endpoints
    GET / - Interface web principal
    GET /api/stats - Estatísticas em JSON
    GET /api/config - Configuração atual
    POST /api/save-config - Salvar nova configuração
    GET /api/scan-wifi - Listar redes disponíveis
    POST /api/restart-miner - Reiniciar minerador
    POST /api/enable-ap - Ativar modo AP
    POST /api/delete-logs - Apagar logs

⛏️ Funcionamento da Mineração
Protocolo Stratum
    Subscribe: Registro na pool
    Authorize: Autenticação do worker
    Notify: Recebimento de trabalhos
    Submit: Envio de shares válidos

Algoritmo SHA-256
    Implementação acelerada por hardware
    Cálculo de double SHA-256
    Verificação de target de dificuldade
    Submissão automática de shares

📊 Monitoramento
Logs Salvos
    Hashes calculados
    Shares aceitos/rejeitados
    Hashrate médio
    Tempo de operação

Estatísticas
{
  "hashes": 15000,
  "hashrate": "45.2 H/s", 
  "shares": 3,
  "shares_rejected": 0,
  "uptime": "2h 15m 30s"
}

🛠️ Solução de Problemas
WiFi Não Conecta
    Verifique SSID e senha
    Certifique-se do sinal adequado
    Use modo AP para reconfigurar

Pool Não Conecta
    Verifique host e porta
    Confirme carteira e worker
    Teste conectividade de rede

Baixo Hashrate
    ESP32 tem poder limitado (~50 H/s)
    Hashrate varia com dificuldade
    Verifique estabilidade da alimentação

⚠️ Limitações
    Performance: ~50 H/s (típico para ESP32)
    Rentabilidade: Mais educacional que lucrativo
    Energia: Consome ~500mA durante mineração
    Rede: Requer conexão WiFi estável

📝 Licença

Este projeto é para fins educacionais. Verifique a legalidade da mineração em sua região.
🤝 Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para:
    Reportar bugs
    Sugerir melhorias
    Enviar pull requests
