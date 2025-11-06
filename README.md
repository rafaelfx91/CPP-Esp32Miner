<!DOCTYPE html>
<html>
<head>
</head>
<body>
  
<h1 align="center">⚡ ESP32 SHA-256 Crypto Miner</h1>

<p align="center">
  <strong>Minerador de criptomoedas completo com interface web e LEDs indicadores</strong>
  <br>
  <em>Mineração real SHA-256 • Interface web responsiva • Sistema dual WiFi</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-SHA256 Miner-green" alt="ESP32">
  <img src="https://img.shields.io/badge/Platform-Arduino%20%7C%20PlatformIO-blue" alt="Platform">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License">
  <img src="https://img.shields.io/badge/Version-1.0.0-orange" alt="Version">
</p>

<div align="center">
  
  🎯 **Mineração Real** • 🌐 **Interface Web** • 📱 **Responsivo** • 💡 **LEDs Indicadores** • 🔧 **Fácil Configuração**

</div>

<br>

## 🚀 **Características Principais**

| Feature | Descrição |
|---------|-----------|
| ⚡ **Mineração Real SHA-256** | Implementação otimizada com acelerador de hardware do ESP32 |
| 🌐 **Interface Web Responsiva** | Controle completo via navegador em qualquer dispositivo |
| 📶 **Sistema Dual WiFi** | Modo STA (mineração) + AP (configuração) automático |
| 💡 **LEDs Indicadores Visuais** | 4 LEDs para status em tempo real |
| 💾 **Configuração Persistente** | Salva settings na memória SPIFFS |
| 🎯 **Multi-pool Support** | Compatível com pools Stratum (Unmineable, etc.) |
| 💰 **Multi-moedas** | Suporte a TRX, BTC, DOGE e outras via Unmineable |
| 📊 **Monitoramento em Tempo Real** | Estatísticas detalhadas via API REST |

<br>

## 🎯 **Demonstração Visual**

### 💡 Sistema de LEDs
| GPIO | Cor | Estado | Significado |
|------|-----|--------|-------------|
| **D18** | 🟡 Amarelo | Fixo | **Modo AP Ativo** |
| **D19** | 🟢 Verde | Fixo | **WiFi Conectado** |
| **D22** | 🟢 Verde | Piscante | **Minerando Ativamente** |
| **D23** | 🟢 Verde | Fixo | **Shares Aceitos** |

### 📱 Interface Web
![Interface Web](https://via.placeholder.com/800x400/667eea/ffffff?text=ESP32+Miner+Interface+Web)
> *Interface web responsiva com controle total do minerador*

<br>

## 📋 **Pré-requisitos**

### 🛠️ Hardware Requerido
- ✅ **ESP32** (com acelerador SHA-256)
- ✅ **4x LEDs** (para indicadores de status)
- ✅ **Fonte de alimentação estável** (≥500mA)
- ✅ **Conexão WiFi** 2.4GHz

### 📚 Software & Bibliotecas
```cpp<br>
// Bibliotecas necessárias<br>
#include <WiFi.h><br>
#include <WebServer.h><br>
#include <ArduinoJson.h><br>
#include <SPIFFS.h><br>
#include "sha256_acelerado.h"<br>

🔧 Instalação Rápida<br>
1. 📥 Clone o Repositório<br>
	git clone https://github.com/seu-usuario/esp32-miner.git<br>
	cd esp32-miner<br>

2. 🔌 Conexões dos LEDs<br>
	#define LED_AP_MODE      18  // D18 - Amarelo - Modo AP<br>
	#define LED_WIFI_CONNECT 19  // D19 - Verde - WiFi Conectado  <br>
	#define LED_MINING       22  // D22 - Verde - Minerando (Piscante)<br>
	#define LED_SHARES       23  // D23 - Verde - Shares Aceitos<br>

3. ⚙️ Configuração Inicial<br>
	// Configuração padrão - personalize no painel web<br>
	String wifi_ssid = "sua-rede-wifi";
	String wifi_password = "sua-senha";<br>
	String trx_wallet = "sua-carteira";<br>
	String pool_host = "pool";<br>
	String pool_port = "3333";<br>
	String worker_name = "seuminer#seu-id";<br>
	String coin_type = "COIN";<br>

4. 🚀 Primeira Execução<br>
    Carregue o código no ESP32<br>
    Conecte-se ao WiFi ESP32-Miner-Config (senha: 12345678)<br>
    Acesse http://192.168.4.1<br>
    Configure sua rede WiFi e dados da pool<br>

🌐 Interface Web - Features<br>
📊 Painel de Status<br>
	{<br>
	  "wifi_ssid": "SuaRedeWiFi",<br>
	  "pool_connected": true,<br>
	  "hashrate": "45.2 H/s",<br>
	  "shares": 15,<br>
	  "uptime": "2h 15m 30s"<br>
	}<br>

⚙️ Configurações
    🔍 Scan de redes WiFi automático<br>
    💰 Configuração de carteira e moeda<br>
    🎯 Seleção de pool personalizável<br>
    👷 Nome do worker customizável<br>

🔧 Controles<br>
    🔄 Reinício do minerador<br>
    📶 Ativação modo AP<br>
    🗑️ Limpeza de logs<br>
    💾 Salvar configuração<br>


⛏️ Funcionamento da Mineração<br>
🔄 Protocolo Stratum<br>
	graph LR<br>
		A[ESP32 Miner] --> B[Subscribe]<br>
		B --> C[Authorize]<br>
		C --> D[Notify - Jobs]<br>
		D --> E[Submit - Shares]<br>
		E --> F[💰 Rewards]<br>

⚡ Algoritmo SHA-256<br>
    🚀 Aceleração por hardware do ESP32<br>
    🔄 Double SHA-256 calculation<br>
    ✅ Verificação automática de target<br>
    📤 Submissão inteligente de shares<br>


📊 Performance & Estatísticas<br>
🎯 Métricas em Tempo Real<br>
	Métrica	Valor Típico	Descrição<br>
	Hashrate	40-60 H/s	Poder de processamento<br>
	Shares/Hora	0.1-0.5	Shares aceitos por hora<br>
	Consumo	~500mA	Consumo elétrico<br>
	Temperatura	45-65°C	Temperatura de operação<br>
	
📈 Exemplo de Logs<br>
	[1200000] Hashes: 15000 | Shares: 3 | Rejeitados: 0 | Hashrate: 45.2 H/s | Tempo: 5 min<br>
	[2400000] Hashes: 32000 | Shares: 7 | Rejeitados: 1 | Hashrate: 48.1 H/s | Tempo: 11 min<br>

🛠️ API Endpoints<br>
	Endpoint	Método	Descrição<br>
	/api/stats	GET	📊 Estatísticas em tempo real<br>
	/api/config	GET	⚙️ Configuração atual<br>
	/api/save-config	POST	💾 Salvar nova configuração<br>
	/api/scan-wifi	GET	🔍 Listar redes WiFi<br>
	/api/restart-miner	POST	🔄 Reiniciar minerador<br>
	/api/enable-ap	POST	📶 Ativar modo AP<br>
	/api/delete-logs	POST	🗑️ Apagar logs<br>

🚨 Solução de Problemas<br>
❌ WiFi Não Conecta<br>

✅ Verifique: SSID e senha corretos<br>
✅ Certifique: Sinal WiFi adequado  <br>
✅ Use: Modo AP para reconfigurar<br>

❌ Pool Não Conecta<br>
✅ Verifique: Host e porta da pool<br>
✅ Confirme: Carteira e worker válidos<br>
✅ Teste: Conectividade de rede<br>

❌ Baixo Hashrate<br>
⚠️  ESP32 tem poder limitado (~50 H/s)<br>
⚠️  Hashrate varia com dificuldade<br>
✅ Verifique: Estabilidade da alimentação<br>

⚠️ Limitações & Considerações<br>
Aspecto	Detalhe<br>
🎯 Performance	~50 H/s (típico para ESP32)<br>
💰 Rentabilidade	Mais educacional que lucrativo<br>
⚡ Energia	Consome ~500mA durante mineração<br>
📶 Rede	Requer conexão WiFi estável<br>
🌡️ Temperatura	Pode aquecer durante operação contínua<br>








