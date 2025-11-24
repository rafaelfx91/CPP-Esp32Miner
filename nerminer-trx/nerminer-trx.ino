#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "sha256_acelerado.h"

// ========== CONFIGURAÇÕES PADRÃO ==========
String wifi_ssid = "SUA_REDE";
String wifi_password = "SUA_SENHA";
String trx_wallet = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";
String pool_host = "sha256.unmineable.com";
String pool_port = "3333";
String worker_name = "esp32-miner#cub7-5a3h";
String coin_type = "TRX";
// ==========================================

// ========== PINOS DOS LEDs ==========
#define LED_AP_MODE      18  // D18 - Modo AP
#define LED_WIFI_CONNECT 19  // D19 - WiFi Conectado
#define LED_MINING       22  // D22 - Minerando
#define LED_SHARES       23  // D23 - Shares Aceitos
// ====================================

WiFiClient poolClient;
WebServer server(80);

// Variáveis globais
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long shares_rejected = 0;
unsigned long start_time = 0;
unsigned long last_log_save = 0;
unsigned long last_pool_activity = 0;
unsigned long last_led_blink = 0;
bool poolConnected = false;
bool isMiningActive = false;
String current_job_id = "";

// Variáveis para mineração real
uint8_t job_target[32];
uint8_t job_header[80];
uint32_t current_nonce = 0;

// Variáveis do sistema WiFi
bool isAPMode = false;
bool wifiConnected = false;

// Funções auxiliares
void hexToBytes(const char* hex, uint8_t* bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
    }
}

void reverseBytes(uint8_t* data, size_t len) {
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t temp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = temp;
    }
}

// ========== SISTEMA DE LEDs ==========
void setupLEDs() {
  pinMode(LED_AP_MODE, OUTPUT);
  pinMode(LED_WIFI_CONNECT, OUTPUT);
  pinMode(LED_MINING, OUTPUT);
  pinMode(LED_SHARES, OUTPUT);
  
  // Inicializar todos desligados
  digitalWrite(LED_AP_MODE, LOW);
  digitalWrite(LED_WIFI_CONNECT, LOW);
  digitalWrite(LED_MINING, LOW);
  digitalWrite(LED_SHARES, LOW);
  
  Serial.println("✅ LEDs inicializados: D18(AP), D19(WiFi), D22(Minerando), D23(Shares)");
}

void updateLEDs() {
  unsigned long currentMillis = millis();
  
  // D18 - Modo AP (Amarelo fixo)
  digitalWrite(LED_AP_MODE, isAPMode);
  
  // D19 - WiFi Conectado (Verde fixo)
  digitalWrite(LED_WIFI_CONNECT, wifiConnected);
  
  // D22 - Minerando (Verde piscante)
  if (poolConnected && isMiningActive) {
    // Piscar a cada 500ms
    if (currentMillis - last_led_blink >= 500) {
      digitalWrite(LED_MINING, !digitalRead(LED_MINING));
      last_led_blink = currentMillis;
    }
  } else {
    digitalWrite(LED_MINING, LOW);
  }
  
  // D23 - Shares Aceitos (Verde fixo quando > 0)
  digitalWrite(LED_SHARES, shares_submitted > 0);
}

// ========== SISTEMA DE CONFIGURAÇÃO ==========
void loadConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Erro ao montar SPIFFS");
    return;
  }
  
  if (SPIFFS.exists("/config.json")) {
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, configFile);
      
      if (!error) {
        wifi_ssid = doc["wifi_ssid"] | "AAAAA";
        wifi_password = doc["wifi_password"] | "Penademorcego5";
        trx_wallet = doc["wallet"] | "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";
        pool_host = doc["pool_host"] | "sha256.unmineable.com";
        pool_port = doc["pool_port"] | "3333";
        worker_name = doc["worker_name"] | "esp32-miner#cub7-5a3h";
        coin_type = doc["coin_type"] | "TRX";
        
        Serial.println("✅ Configuração carregada!");
      }
      configFile.close();
    }
  }
}

void saveConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Erro ao montar SPIFFS");
    return;
  }
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("❌ Erro ao salvar configuração");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  doc["wifi_ssid"] = wifi_ssid;
  doc["wifi_password"] = wifi_password;
  doc["wallet"] = trx_wallet;
  doc["pool_host"] = pool_host;
  doc["pool_port"] = pool_port;
  doc["worker_name"] = worker_name;
  doc["coin_type"] = coin_type;
  
  serializeJson(doc, configFile);
  configFile.close();
  
  Serial.println("✅ Configuração salva!");
}

// ========== SISTEMA WiFi DUAL (STA + AP) ==========
bool connectToWiFi() {
  Serial.println("📡 Tentando conectar WiFi: " + wifi_ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    updateLEDs(); // Atualizar LEDs durante tentativa
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    isAPMode = false;
    Serial.println("\n✅ WiFi Conectado! IP: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\n❌ Falha na conexão WiFi - Ativando modo AP");
    startAPMode();
    return false;
  }
}

void startAPMode() {
  Serial.println("🌐 Iniciando modo Access Point...");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-Miner-Config", "12345678");
  
  isAPMode = true;
  wifiConnected = false;
  
  Serial.println("✅ Modo AP Ativo!");
  Serial.println("📶 SSID: ESP32-Miner-Config");
  Serial.println("🔑 Senha: 12345678");
  Serial.println("🌐 Acesse: http://192.168.4.1");
}

// ========== MINERAÇÃO ==========
bool connectToMiningPool() {
  if (!wifiConnected) {
    Serial.println("❌ Sem conexão WiFi - Não é possível conectar à pool");
    return false;
  }
  
  Serial.println("🔗 Conectando à pool " + pool_host + ":" + pool_port);
  
  if (!poolClient.connect(pool_host.c_str(), pool_port.toInt())) {
    Serial.println("❌ Falha na conexão com a pool");
    return false;
  }
  
  Serial.println("✅ Conectado à pool!");
  last_pool_activity = millis();
  
  String subscribe_msg = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32Miner/1.0.0\"]}\n";
  poolClient.print(subscribe_msg);
  Serial.println("📤 Enviando subscription...");
  
  return true;
}

void authorizeWorker() {
  String auth_msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"" + coin_type + ":" + trx_wallet + "." + worker_name + "\",\"\"]}\n";
  poolClient.print(auth_msg);
  Serial.println("🔑 Autorizando worker: " + coin_type + ":" + trx_wallet + "." + worker_name);
}

void submitHashrate() {
  if (!wifiConnected) return;
  
  unsigned long current_time = millis();
  float elapsed_sec = (current_time - start_time) / 1000.0;
  float hashrate = elapsed_sec > 0 ? (float)hashes_calculated / elapsed_sec : 0;
  
  String hashrate_msg = "{\"id\":6,\"method\":\"mining.hashrate\",\"params\":[\"" + String((int)hashrate) + "\"]}\n";
  poolClient.print(hashrate_msg);
  Serial.println("📊 Reportando hashrate: " + String(hashrate, 1) + " H/s");
}

bool isPoolReallyConnected() {
  return poolClient.connected() && (millis() - last_pool_activity < 120000);
}

void processMiningNotify(JsonArray params) {
    current_job_id = String(params[0].as<const char*>());
    String prevhash = params[1].as<const char*>();
    String coinb1 = params[2].as<const char*>();
    String coinb2 = params[3].as<const char*>();
    String version = params[5].as<const char*>();
    String nbits = params[6].as<const char*>();
    String ntime = params[7].as<const char*>();
    
    Serial.println("🎯 NOVO TRABALHO: " + current_job_id);
    isMiningActive = true;
    
    // Montar cabeçalho de 80 bytes
    memset(job_header, 0, 80);
    
    // Version (4 bytes)
    hexToBytes(version.c_str(), job_header, 4);
    reverseBytes(job_header, 4);
    
    // PrevHash (32 bytes)
    hexToBytes(prevhash.c_str(), job_header + 4, 32);
    reverseBytes(job_header + 4, 32);
    
    // Merkle Root (32 bytes)
    memset(job_header + 36, 0, 32);
    
    // Time (4 bytes)
    hexToBytes(ntime.c_str(), job_header + 68, 4);
    reverseBytes(job_header + 68, 4);
    
    // Bits (4 bytes)
    hexToBytes(nbits.c_str(), job_header + 72, 4);
    reverseBytes(job_header + 72, 4);
    
    // Nonce inicial (4 bytes)
    current_nonce = 0;
    memcpy(job_header + 76, &current_nonce, 4);
    
    // Target para dificuldade 16384
    memset(job_target, 0x00, 32);
    job_target[0] = 0x00;
    job_target[1] = 0x00; 
    job_target[2] = 0x00;
    job_target[3] = 0x00;
    job_target[4] = 0x00;
    job_target[5] = 0x00;
    job_target[6] = 0x00;
    job_target[7] = 0x00;
    job_target[29] = 0xFF;
    job_target[30] = 0xFF;
    job_target[31] = 0xFF;

    Serial.println("✅ Trabalho configurado. Iniciando mineração real SHA-256!");
}

void handlePoolResponse() {
  while (poolClient.available()) {
    String response = poolClient.readStringUntil('\n');
    Serial.println("📥 Pool: " + response);
    last_pool_activity = millis();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print("❌ JSON parse failed: ");
      Serial.println(error.c_str());
      return;
    }
    
    if (doc.containsKey("id")) {
      int msg_id = doc["id"];
      
      if (msg_id == 1) {
        Serial.println("✅ Subscription aceito!");
        delay(1000);
        authorizeWorker();
      }
      else if (msg_id == 2) {
        bool auth_result = doc["result"];
        if (auth_result) {
          Serial.println("🎉 WORKER AUTORIZADO! Iniciando mineração...");
          poolConnected = true;
          submitHashrate();
        } else {
          Serial.println("❌ Falha na autorização");
        }
      }
      else if (msg_id > 2) {
        bool share_result = doc["result"];
        if (share_result) {
          shares_submitted++;
          Serial.println("✅ SHARE ACEITO! Total: " + String(shares_submitted));
          saveLog();
        } else {
          shares_rejected++;
          Serial.println("❌ SHARE REJEITADO! Total rejeitados: " + String(shares_rejected));
        }
      }
    }
    
    if (doc.containsKey("method") && String(doc["method"].as<const char*>()) == "mining.notify") {
      if (doc["params"].is<JsonArray>() && doc["params"].size() > 0) {
        processMiningNotify(doc["params"].as<JsonArray>());
      }
    }
  }
}

void processMiningJob() {
  if (current_job_id == "") return;
  
  uint32_t nonce_batch_size = 1000;
  uint8_t hash_result[32];
  
  for(uint32_t i = 0; i < nonce_batch_size; i++) {
    memcpy(job_header + 76, &current_nonce, 4);
    
    calculate_double_sha256(job_header, hash_result);
    
    hashes_calculated++;
    current_nonce++;
    
    bool is_valid = true;
    for (int j = 31; j >= 0; j--) {
      if (hash_result[j] < job_target[j]) {
        is_valid = true;
        break;
      }
      if (hash_result[j] > job_target[j]) {
        is_valid = false;
        break;
      }
    }
    
    if (is_valid) {
      char nonce_hex[9];
      sprintf(nonce_hex, "%08x", current_nonce - 1);
      submitShare(current_job_id, String(nonce_hex));
      break;
    }
    
    if (hashes_calculated % 100 == 0) {
      unsigned long current_time = millis();
      float elapsed_sec = (current_time - start_time) / 1000.0;
      float hashrate = elapsed_sec > 0 ? (float)hashes_calculated / elapsed_sec : 0;
      
      Serial.print("⛏️ ");
      Serial.print(hashes_calculated);
      Serial.print(" hashes | ");
      Serial.print(hashrate, 1);
      Serial.print(" H/s | Shares: ");
      Serial.print(shares_submitted);
      Serial.print(" | Rejeitados: ");
      Serial.println(shares_rejected);
    }
  }
}

void submitShare(String job_id, String nonce) {
  String share_msg = "{\"id\":" + String(millis()) + ",\"method\":\"mining.submit\",\"params\":[\"" + coin_type + ":" + 
                    trx_wallet + "." + worker_name + "\",\"" + job_id + "\",\"" + nonce + "\"]}\n";
  poolClient.print(share_msg);
  Serial.println("📤 Enviando share REAL: " + nonce);
}

void saveLog() {
  if (!SPIFFS.begin(true)) return;
  
  File file = SPIFFS.open("/miner_log.txt", "a");
  if (!file) return;
  
  unsigned long current_time = millis();
  unsigned long minutes = (current_time - start_time) / 60000;
  float hashrate = minutes > 0 ? (float)hashes_calculated / (minutes * 60) : 0;
  
  String log_entry = "[" + String(millis()) + "] ";
  log_entry += "Hashes: " + String(hashes_calculated) + " | ";
  log_entry += "Shares: " + String(shares_submitted) + " | ";
  log_entry += "Rejeitados: " + String(shares_rejected) + " | ";
  log_entry += "Hashrate: " + String(hashrate, 2) + " H/s | ";
  log_entry += "Tempo: " + String(minutes) + " min\n";
  
  file.println(log_entry);
  file.close();
}

void deleteLogs() {
  if (!SPIFFS.begin(true)) return;
  SPIFFS.remove("/miner_log.txt");
  Serial.println("✅ Logs apagados!");
}

// ========== SERVIDOR WEB ==========
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="pt-br">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 Miner</title>
      <style>
          * { margin: 0; padding: 0; box-sizing: border-box; }
          body { 
              background: #121212; 
              color: #ffffff; 
              font-family: Arial, sans-serif;
              padding: 20px;
          }
          .container { max-width: 1000px; margin: 0 auto; }
          .header { 
              text-align: center; 
              margin-bottom: 20px;
              padding: 20px;
              background: #1e1e1e;
              border-radius: 10px;
              border: 2px solid #00ff00;
          }
          .ap-mode { border-color: #ff9900 !important; }
          .card { 
              background: #1e1e1e; 
              padding: 20px; 
              margin: 15px 0; 
              border-radius: 10px;
              border-left: 4px solid #00ff00;
          }
          .led-status { 
              display: flex; 
              justify-content: space-around; 
              margin: 15px 0;
              padding: 15px;
              background: #2d2d2d;
              border-radius: 8px;
          }
          .led-item { text-align: center; }
          .led-label { font-size: 12px; color: #cccccc; margin-bottom: 5px; }
          .led-indicator { 
              width: 20px; 
              height: 20px; 
              border-radius: 50%; 
              margin: 0 auto;
              border: 2px solid #444;
          }
          .led-on { background: #00ff00; box-shadow: 0 0 10px #00ff00; }
          .led-off { background: #333; }
          .led-ap { background: #ff9900; box-shadow: 0 0 10px #ff9900; }
          .led-blinking { animation: blink 1s infinite; }
          @keyframes blink { 
              0% { opacity: 1; } 
              50% { opacity: 0.3; } 
              100% { opacity: 1; } 
          }
          .config-grid { 
              display: grid; 
              grid-template-columns: 1fr 1fr; 
              gap: 15px;
              margin-bottom: 15px;
          }
          .form-group { margin-bottom: 12px; }
          .form-label { 
              display: block; 
              margin-bottom: 5px; 
              color: #cccccc;
              font-weight: bold;
          }
          .form-input { 
              width: 100%; 
              padding: 10px; 
              border: 1px solid #444; 
              border-radius: 5px; 
              background: #2d2d2d; 
              color: white;
          }
          .status { display: flex; justify-content: space-between; margin: 5px 0; }
          .label { color: #cccccc; }
          .value { font-weight: bold; }
          .connected { color: #00ff00; }
          .disconnected { color: #ff0000; }
          .button { 
              background: #00ff00; 
              color: #000; 
              border: none; 
              padding: 12px 24px; 
              margin: 5px;
              border-radius: 5px; 
              cursor: pointer;
              font-weight: bold;
              font-size: 14px;
          }
          .button.delete { background: #ff4444; color: white; }
          .button.secondary { background: #2196F3; color: white; }
          .button.ap { background: #ff9900; color: black; }
          .stats-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
          .badge { 
              background: #4CAF50; 
              color: white; 
              padding: 3px 8px; 
              border-radius: 10px; 
              font-size: 11px;
              margin-left: 5px;
          }
          .ap-badge { background: #ff9900; }
          .button-group { text-align: center; margin: 15px 0; }
          .network-list { max-height: 200px; overflow-y: auto; margin: 10px 0; }
          .network-item { padding: 8px; border: 1px solid #444; margin: 5px 0; border-radius: 5px; cursor: pointer; }
          .network-item:hover { background: #2d2d2d; }
          @media (max-width: 768px) { 
              .config-grid, .stats-grid { grid-template-columns: 1fr; } 
          }
      </style>
      <script>
          function updateLEDs() {
              // Simular LEDs baseado no status
              const apLed = document.getElementById('ledAP');
              const wifiLed = document.getElementById('ledWiFi');
              const miningLed = document.getElementById('ledMining');
              const sharesLed = document.getElementById('ledShares');
              
              // Esses seriam atualizados via API em produção
              // Por enquanto é apenas visual
          }
          
          function loadConfig() {
              fetch('/api/config')
                  .then(response => response.json())
                  .then(data => {
                      document.getElementById('wifiSsid').value = data.wifi_ssid || '';
                      document.getElementById('wifiPassword').value = data.wifi_password || '';
                      document.getElementById('poolHost').value = data.pool_host || '';
                      document.getElementById('poolPort').value = data.pool_port || '';
                      document.getElementById('wallet').value = data.wallet || '';
                      document.getElementById('coinType').value = data.coin_type || 'TRX';
                      document.getElementById('workerName').value = data.worker_name || '';
                  });
          }
          
          function scanNetworks() {
              fetch('/api/scan-wifi')
                  .then(response => response.json())
                  .then(data => {
                      const list = document.getElementById('networkList');
                      list.innerHTML = '';
                      data.networks.forEach(network => {
                          const item = document.createElement('div');
                          item.className = 'network-item';
                          item.innerHTML = `📶 ${network.ssid} (${network.rssi}dBm) ${network.encrypted ? '🔒' : '🔓'}`;
                          item.onclick = () => {
                              document.getElementById('wifiSsid').value = network.ssid;
                          };
                          list.appendChild(item);
                      });
                  });
          }
          
          function saveConfig() {
              const config = {
                  wifi_ssid: document.getElementById('wifiSsid').value,
                  wifi_password: document.getElementById('wifiPassword').value,
                  pool_host: document.getElementById('poolHost').value,
                  pool_port: document.getElementById('poolPort').value,
                  wallet: document.getElementById('wallet').value,
                  coin_type: document.getElementById('coinType').value,
                  worker_name: document.getElementById('workerName').value
              };
              
              fetch('/api/save-config', {
                  method: 'POST',
                  headers: { 'Content-Type': 'application/json' },
                  body: JSON.stringify(config)
              })
              .then(response => response.json())
              .then(data => {
                  alert(data.message);
                  if(data.success) {
                      setTimeout(() => {
                          fetch('/api/restart-miner')
                              .then(() => alert('Reiniciando minerador...'));
                      }, 1000);
                  }
              });
          }
          
          function updateStats() {
              fetch('/api/stats')
                  .then(response => response.json())
                  .then(data => {
                      // Atualizar modo AP/STA
                      const header = document.getElementById('header');
                      if(data.ap_mode) {
                          header.className = 'header ap-mode';
                          document.getElementById('apIndicator').style.display = 'block';
                          document.getElementById('staIndicator').style.display = 'none';
                      } else {
                          header.className = 'header';
                          document.getElementById('apIndicator').style.display = 'none';
                          document.getElementById('staIndicator').style.display = 'block';
                      }
                      
                      // Atualizar LEDs visuais
                      document.getElementById('ledAP').className = data.ap_mode ? 
                          'led-indicator led-ap' : 'led-indicator led-off';
                      document.getElementById('ledWiFi').className = data.wifi_connected ? 
                          'led-indicator led-on' : 'led-indicator led-off';
                      document.getElementById('ledMining').className = data.pool_connected ? 
                          'led-indicator led-on led-blinking' : 'led-indicator led-off';
                      document.getElementById('ledShares').className = data.shares > 0 ? 
                          'led-indicator led-on' : 'led-indicator led-off';
                      
                      // Atualizar stats
                      document.getElementById('wifiStatus').innerHTML = data.wifi_ssid;
                      document.getElementById('connectionStatus').innerHTML = data.pool_connected ? 
                          '<span class="connected">● Conectado e Minerando</span>' : 
                          '<span class="disconnected">● Desconectado</span>';
                      document.getElementById('ipAddress').innerHTML = data.ip_address;
                      document.getElementById('macAddress').innerHTML = data.mac_address;
                      document.getElementById('uptime').innerHTML = data.uptime;
                      document.getElementById('hashes').innerHTML = data.hashes.toLocaleString();
                      document.getElementById('hashrate').innerHTML = data.hashrate + ' H/s';
                      document.getElementById('shares').innerHTML = data.shares;
                      document.getElementById('rejected').innerHTML = data.shares_rejected;
                      document.getElementById('currentWorker').innerHTML = data.worker;
                      document.getElementById('currentPool').innerHTML = data.pool;
                      document.getElementById('currentCoin').innerHTML = data.coin;
                      document.getElementById('currentWallet').innerHTML = data.wallet;
                  });
          }
          
          function deleteLogs() {
              if(confirm('Apagar todos os logs?')) {
                  fetch('/api/delete-logs').then(() => alert('Logs apagados!'));
              }
          }
          
          function restartMiner() {
              if(confirm('Reiniciar minerador com configuração atual?')) {
                  fetch('/api/restart-miner').then(() => alert('Reiniciando...'));
              }
          }
          
          function enableAPMode() {
              if(confirm('Ativar modo AP por 10 minutos?')) {
                  fetch('/api/enable-ap').then(() => alert('Modo AP ativado!'));
              }
          }
          
          setInterval(updateStats, 2000);
          document.addEventListener('DOMContentLoaded', () => {
              loadConfig();
              updateStats();
              scanNetworks();
          });
      </script>
  </head>
  <body>
      <div class="container">
          <div id="header" class="header">
              <h1>⚡ ESP32 Miner</h1>
              <div id="apIndicator" style="display: none;">
                  <p>🔧 <strong>Modo Configuração</strong> - Conectado via WiFi do ESP32</p>
                  <p>📶 SSID: <strong>ESP32-Miner-Config</strong> | IP: <strong>192.168.4.1</strong></p>
              </div>
              <div id="staIndicator" style="display: none;">
                  <p>⛏️ <strong>Modo Mineração</strong> - Conectado na rede WiFi</p>
              </div>
          </div>
          
          <div class="card">
              <h2>💡 Status dos LEDs</h2>
              <div class="led-status">
                  <div class="led-item">
                      <div class="led-label">D18 - Modo AP</div>
                      <div id="ledAP" class="led-indicator led-off"></div>
                  </div>
                  <div class="led-item">
                      <div class="led-label">D19 - WiFi</div>
                      <div id="ledWiFi" class="led-indicator led-off"></div>
                  </div>
                  <div class="led-item">
                      <div class="led-label">D22 - Minerando</div>
                      <div id="ledMining" class="led-indicator led-off"></div>
                  </div>
                  <div class="led-item">
                      <div class="led-label">D23 - Shares</div>
                      <div id="ledShares" class="led-indicator led-off"></div>
                  </div>
              </div>
          </div>
          
          <div class="card">
              <h2>📡 Status da Conexão</h2>
              <div class="status">
                  <span class="label">WiFi:</span>
                  <span class="value" id="wifiStatus">...</span>
              </div>
              <div class="status">
                  <span class="label">Status Pool:</span>
                  <span class="value" id="connectionStatus">...</span>
              </div>
              <div class="status">
                  <span class="label">IP Local:</span>
                  <span class="value" id="ipAddress">...</span>
              </div>
              <div class="status">
                  <span class="label">MAC:</span>
                  <span class="value" id="macAddress">...</span>
              </div>
              <div class="status">
                  <span class="label">Tempo Online:</span>
                  <span class="value" id="uptime">...</span>
              </div>
          </div>
          
          <div class="card">
              <h2>⛏️ Status da Mineração</h2>
              <div class="stats-grid">
                  <div class="status">
                      <span class="label">Hashes Calculados:</span>
                      <span class="value" id="hashes">0</span>
                  </div>
                  <div class="status">
                      <span class="label">Hashrate:</span>
                      <span class="value" id="hashrate">0 H/s</span>
                  </div>
                  <div class="status">
                      <span class="label">Shares Aceitos:</span>
                      <span class="value" id="shares">0</span>
                  </div>
                  <div class="status">
                      <span class="label">Shares Rejeitados:</span>
                      <span class="value" id="rejected">0</span>
                  </div>
                  <div class="status">
                      <span class="label">Worker Atual:</span>
                      <span class="value" id="currentWorker">...</span>
                  </div>
                  <div class="status">
                      <span class="label">Pool Atual:</span>
                      <span class="value" id="currentPool">...</span>
                  </div>
                  <div class="status">
                      <span class="label">Coin:</span>
                      <span class="value" id="currentCoin">...</span>
                  </div>
                  <div class="status">
                      <span class="label">Carteira:</span>
                      <span class="value" id="currentWallet">...</span>
                  </div>
              </div>
          </div>
          
          <div class="card">
              <h2>⚙️ Configuração do Minerador</h2>
              
              <h3 style="color: #ff9900; margin: 15px 0 10px 0;">📶 Configuração WiFi</h3>
              <button class="button ap" onclick="scanNetworks()">🔍 Buscar Redes WiFi</button>
              <div id="networkList" class="network-list"></div>
              
              <div class="config-grid">
                  <div class="form-group">
                      <label class="form-label">SSID WiFi:</label>
                      <input type="text" id="wifiSsid" class="form-input" placeholder="Nome da rede WiFi">
                  </div>
                  <div class="form-group">
                      <label class="form-label">Senha WiFi:</label>
                      <input type="password" id="wifiPassword" class="form-input" placeholder="Senha da rede">
                  </div>
                  <div class="form-group">
                      <label class="form-label">Pool:</label>
                      <input type="text" id="poolHost" class="form-input" placeholder="ex: sha256.unmineable.com">
                  </div>
                  <div class="form-group">
                      <label class="form-label">Porta:</label>
                      <input type="text" id="poolPort" class="form-input" placeholder="ex: 3333">
                  </div>
                  <div class="form-group">
                      <label class="form-label">Carteira:</label>
                      <input type="text" id="wallet" class="form-input" placeholder="Sua carteira">
                  </div>
                  <div class="form-group">
                      <label class="form-label">Coin:</label>
                      <select id="coinType" class="form-input">
                          <option value="TRX">TRON (TRX)</option>
                          <option value="BTC">Bitcoin (BTC)</option>
                          <option value="DOGE">Dogecoin (DOGE)</option>
                      </select>
                  </div>
                  <div class="form-group">
                      <label class="form-label">Worker Name:</label>
                      <input type="text" id="workerName" class="form-input" placeholder="ex: miner01#referral">
                  </div>
              </div>
              <div class="button-group">
                  <button class="button" onclick="saveConfig()">💾 Salvar Configuração</button>
                  <button class="button secondary" onclick="restartMiner()">🔄 Reiniciar Minerador</button>
                  <button class="button ap" onclick="enableAPMode()">📶 Ativar Modo AP</button>
              </div>
          </div>
          
          <div class="card">
              <h2>🛠️ Controles</h2>
              <div class="button-group">
                  <button class="button" onclick="updateStats()">🔄 Atualizar Status</button>
                  <button class="button delete" onclick="deleteLogs()">🗑️ Apagar Logs</button>
              </div>
          </div>
      </div>
  </body>
  </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}
void handleApiStats() {
  DynamicJsonDocument doc(1024);
  
  unsigned long current_time = millis();
  unsigned long uptime_seconds = (current_time - start_time) / 1000;
  unsigned long hours = uptime_seconds / 3600;
  unsigned long minutes = (uptime_seconds % 3600) / 60;
  unsigned long seconds = uptime_seconds % 60;
  
  String uptime_str = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
  float hashrate = uptime_seconds > 0 ? (float)hashes_calculated / uptime_seconds : 0;
  
  doc["wifi_ssid"] = WiFi.SSID();
  doc["pool_connected"] = isPoolReallyConnected();
  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["uptime"] = uptime_str;
  doc["hashes"] = hashes_calculated;
  doc["hashrate"] = String(hashrate, 2);
  doc["shares"] = shares_submitted;
  doc["shares_rejected"] = shares_rejected;
  doc["worker"] = worker_name;
  doc["pool"] = pool_host + ":" + pool_port;
  doc["coin"] = coin_type;
  doc["wallet"] = trx_wallet;
  doc["ap_mode"] = isAPMode;
  doc["wifi_connected"] = wifiConnected;
  doc["mining_active"] = isMiningActive;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleApiConfig() {
  DynamicJsonDocument doc(1024);
  doc["wifi_ssid"] = wifi_ssid;
  doc["wifi_password"] = wifi_password;
  doc["pool_host"] = pool_host;
  doc["pool_port"] = pool_port;
  doc["wallet"] = trx_wallet;
  doc["coin_type"] = coin_type;
  doc["worker_name"] = worker_name;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleScanWifi() {
  DynamicJsonDocument doc(1024);
  JsonArray networks = doc.createNestedArray("networks");
  
  if (wifiConnected) {
    int numNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numNetworks; i++) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSaveConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (!error) {
      wifi_ssid = doc["wifi_ssid"].as<String>();
      wifi_password = doc["wifi_password"].as<String>();
      pool_host = doc["pool_host"].as<String>();
      pool_port = doc["pool_port"].as<String>();
      trx_wallet = doc["wallet"].as<String>();
      coin_type = doc["coin_type"].as<String>();
      worker_name = doc["worker_name"].as<String>();
      
      saveConfig();
      
      DynamicJsonDocument responseDoc(256);
      responseDoc["success"] = true;
      responseDoc["message"] = "Configuração salva!";
      
      String response;
      serializeJson(responseDoc, response);
      server.send(200, "application/json", response);
      return;
    }
  }
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["success"] = false;
  responseDoc["message"] = "Erro ao salvar configuração";
  String response;
  serializeJson(responseDoc, response);
  server.send(400, "application/json", response);
}

void handleRestartMiner() {
  Serial.println("🔄 Reiniciando minerador com nova configuração...");
  
  // Desconectar e reconectar
  if (poolClient.connected()) {
    poolClient.stop();
  }
  poolConnected = false;
  isMiningActive = false;
  
  // Reconectar WiFi se necessário
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Tentar conectar à pool
  if (wifiConnected && connectToMiningPool()) {
    Serial.println("✅ Minerador reiniciado com sucesso!");
  }
  
  server.send(200, "application/json", "{\"status\":\"restarted\"}");
}

void handleEnableAP() {
  Serial.println("📶 Ativando modo AP por solicitação do usuário...");
  startAPMode();
  server.send(200, "application/json", "{\"message\":\"Modo AP ativado\"}");
}

void handleDeleteLogs() {
  deleteLogs();
  server.send(200, "application/json", "{\"message\":\"Logs apagados\"}");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/stats", handleApiStats);
  server.on("/api/config", handleApiConfig);
  server.on("/api/scan-wifi", handleScanWifi);
  server.on("/api/save-config", HTTP_POST, handleSaveConfig);
  server.on("/api/restart-miner", handleRestartMiner);
  server.on("/api/enable-ap", handleEnableAP);
  server.on("/api/delete-logs", handleDeleteLogs);
  
  server.begin();
  Serial.println("✅ Servidor HTTP iniciado!");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n⚡ ESP32 MINER COM LEDs ⚡");
  Serial.println("=========================");
  
  // Inicializar LEDs primeiro
  setupLEDs();
  
  // Piscar LEDs inicialmente para teste
  Serial.println("🔴 Testando LEDs...");
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_AP_MODE, HIGH);
    digitalWrite(LED_WIFI_CONNECT, HIGH);
    digitalWrite(LED_MINING, HIGH);
    digitalWrite(LED_SHARES, HIGH);
    delay(200);
    digitalWrite(LED_AP_MODE, LOW);
    digitalWrite(LED_WIFI_CONNECT, LOW);
    digitalWrite(LED_MINING, LOW);
    digitalWrite(LED_SHARES, LOW);
    delay(200);
  }
  
  // Carregar configuração salva
  loadConfig();
  
  // Inicializar acelerador SHA-256
  init_sha256_accelerator();
  Serial.println("✅ Acelerador SHA-256 inicializado!");
  
  // Conectar WiFi (tenta STA, fallback para AP)
  if (!connectToWiFi()) {
    Serial.println("🔄 Iniciando no modo AP (configuração)");
  }
  
  // Configurar mDNS apenas se conectado no WiFi
  if (wifiConnected && !MDNS.begin("esp32-miner")) {
    Serial.println("❌ Erro ao iniciar mDNS");
  } else if (wifiConnected) {
    Serial.println("✅ mDNS: http://esp32-miner.local");
  }
  
  start_time = millis();
  setupWebServer();
  
  // Conectar à pool apenas se WiFi conectado
  if (wifiConnected && connectToMiningPool()) {
    Serial.println("⛏️ Minerador pronto!");
  } else if (!wifiConnected) {
    Serial.println("⏳ Aguardando configuração WiFi via modo AP...");
  }
  
  Serial.println("💡 LEDs configurados:");
  Serial.println("   D18 - Amarelo: Modo AP");
  Serial.println("   D19 - Verde: WiFi Conectado");
  Serial.println("   D22 - Verde Piscante: Minerando");
  Serial.println("   D23 - Verde Fixo: Shares Aceitos");
}

void loop() {
  server.handleClient();
  
  // Atualizar LEDs continuamente
  updateLEDs();
  
  // Verificar se WiFi caiu e ativar AP se necessário
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi desconectado - Ativando modo AP");
    wifiConnected = false;
    isMiningActive = false;
    poolConnected = false;
    startAPMode();
  }
  
  // Processar mineração apenas se WiFi conectado
  if (wifiConnected && poolClient.connected()) {
    handlePoolResponse();
    
    static unsigned long last_ping = 0;
    if (millis() - last_ping > 30000) {
      poolClient.print("{\"id\":99,\"method\":\"mining.ping\",\"params\":[]}\n");
      last_ping = millis();
    }
    
    if (current_job_id != "") {
      processMiningJob();
    } else {
      isMiningActive = false;
    }
    
    static unsigned long last_hashrate_update = 0;
    if (millis() - last_hashrate_update > 120000) {
      submitHashrate();
      last_hashrate_update = millis();
    }
  } else if (wifiConnected && !poolClient.connected()) {
    // Tentar reconectar à pool
    isMiningActive = false;
    Serial.println("🔁 Reconectando à pool...");
    poolConnected = false;
    if (connectToMiningPool()) {
      delay(2000);
    }
    delay(5000);
  }
  
  // Salvar log apenas se minerando
  if (wifiConnected && millis() - last_log_save > 300000) {
    saveLog();
    last_log_save = millis();
  }
  
  delay(50);
}