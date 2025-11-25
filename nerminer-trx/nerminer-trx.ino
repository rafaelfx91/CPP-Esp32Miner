#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "sha256_acelerado.h"

// ========== CONFIGURAÇÕES PADRÃO ==========
String wifi_ssid = "SUAREDE";
String wifi_password = "SUASENHA";
String trx_wallet = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";
String pool_host = "sha256.unmineable.com";
String pool_port = "13333";
String worker_name = "esp32miner";
String coin_type = "TRX";

// ========== PINOS DOS LEDs ==========
#define LED_AP_MODE      18
#define LED_WIFI_CONNECT 19
#define LED_MINING       22
#define LED_SHARES       23

WiFiClient poolClient;
WebServer server(80);

// ========== VARIÁVEIS GLOBAIS ==========
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long shares_rejected = 0;
unsigned long start_time = 0;
unsigned long last_log_save = 0;
unsigned long last_led_blink = 0;

bool poolConnected = false;
bool isMiningActive = false;
String current_job_id = "";
uint8_t job_target[32];
uint8_t job_header[80];
uint32_t current_nonce = 0;

bool isAPMode = false;
bool wifiConnected = false;

// ====================== FUNÇÕES AUXILIARES ======================
void hexToBytes(const char* hex, uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) sscanf(hex + 2 * i, "%2hhx", &bytes[i]);
}

void reverseBytes(uint8_t* data, size_t len) {
  for (size_t i = 0; i < len / 2; i++) {
    uint8_t temp = data[i];
    data[i] = data[len - 1 - i];
    data[len - 1 - i] = temp;
  }
}

// ====================== LEDs ======================
void setupLEDs() {
  pinMode(LED_AP_MODE, OUTPUT);
  pinMode(LED_WIFI_CONNECT, OUTPUT);
  pinMode(LED_MINING, OUTPUT);
  pinMode(LED_SHARES, OUTPUT);
  digitalWrite(LED_AP_MODE, LOW);
  digitalWrite(LED_WIFI_CONNECT, LOW);
  digitalWrite(LED_MINING, LOW);
  digitalWrite(LED_SHARES, LOW);
}

void updateLEDs() {
  unsigned long cm = millis();
  digitalWrite(LED_AP_MODE, isAPMode);
  digitalWrite(LED_WIFI_CONNECT, wifiConnected);
  if (poolConnected && isMiningActive) {
    if (cm - last_led_blink >= 500) {
      digitalWrite(LED_MINING, !digitalRead(LED_MINING));
      last_led_blink = cm;
    }
  } else digitalWrite(LED_MINING, LOW);
  digitalWrite(LED_SHARES, shares_submitted > 0);
}

// ====================== CONFIG ======================
void loadConfig() {
  if (!SPIFFS.begin(true)) return;
  if (SPIFFS.exists("/config.json")) {
    File f = SPIFFS.open("/config.json", "r");
    if (f) {
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, f) == DeserializationError::Ok) {
        wifi_ssid = doc["wifi_ssid"] | wifi_ssid;
        wifi_password = doc["wifi_password"] | wifi_password;
        trx_wallet = doc["wallet"] | trx_wallet;
        pool_host = doc["pool_host"] | pool_host;
        pool_port = doc["pool_port"] | pool_port;
        worker_name = doc["worker_name"] | worker_name;
        coin_type = doc["coin_type"] | coin_type;
      }
      f.close();
    }
  }
}

void saveConfig() {
  if (!SPIFFS.begin(true)) return;
  File f = SPIFFS.open("/config.json", "w");
  if (!f) return;
  DynamicJsonDocument doc(1024);
  doc["wifi_ssid"] = wifi_ssid;
  doc["wifi_password"] = wifi_password;
  doc["wallet"] = trx_wallet;
  doc["pool_host"] = pool_host;
  doc["pool_port"] = pool_port;
  doc["worker_name"] = worker_name;
  doc["coin_type"] = coin_type;
  serializeJson(doc, f);
  f.close();
}

// ====================== WiFi ======================
void startAPMode() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-Miner-Config", "12345678");
  isAPMode = true;
  Serial.println("Modo AP ativado → http://192.168.4.1");
}

bool connectToWiFi() {
  Serial.print("Conectando WiFi: " + wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t++ < 40) { delay(500); Serial.print("."); }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true; isAPMode = false;
    Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nFalha → Modo AP");
    startAPMode();
    return false;
  }
}

// ====================== POOL ======================
bool connectToMiningPool() {
  if (!wifiConnected) return false;
  if (poolClient.connect(pool_host.c_str(), pool_port.toInt())) {
    poolClient.print("{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32Miner/1.0\"]}\n");
    return true;
  }
  return false;
}
/*
void authorizeWorker() {
  String msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"" + coin_type + ":" + trx_wallet + "." + worker_name + "\",\"\"]}\n";
  poolClient.print(msg);
}*/
void authorizeWorker() {
  delay(2000);  // Espera conexão estável
  String msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"" + coin_type + ":" + trx_wallet + "." + worker_name + "\",\"\"]}\n";
  poolClient.print(msg);
  Serial.println("Enviando auth: " + msg);  // Debug
}

// ====================== MINERAÇÃO ======================
void processMiningNotify(JsonArray p) {
  current_job_id = p[0].as<String>();
  String prevhash = p[1].as<String>();
  String version = p[5].as<String>();
  String nbits   = p[6].as<String>();
  String ntime   = p[7].as<String>();
  String target_hex = p[8].as<String>();

  Serial.println("NOVO JOB: " + current_job_id);
  isMiningActive = true;
  memset(job_header, 0, 80);

  hexToBytes(version.c_str(), job_header, 4);     reverseBytes(job_header, 4);
  hexToBytes(prevhash.c_str(), job_header + 4, 32); reverseBytes(job_header + 4, 32);
  hexToBytes(ntime.c_str(), job_header + 68, 4);   reverseBytes(job_header + 68, 4);
  hexToBytes(nbits.c_str(), job_header + 72, 4);   reverseBytes(job_header + 72, 4);

  current_nonce = 0;
  memcpy(job_header + 76, &current_nonce, 4);

  memset(job_target, 0xFF, 32);
  if (target_hex.length() >= 64) {
    hexToBytes(target_hex.c_str(), job_target, 32);
    reverseBytes(job_target, 32);
  }
}

void handlePoolResponse() {
  while (poolClient.available()) {
    String line = poolClient.readStringUntil('\n');
    Serial.println("Pool: " + line);

    DynamicJsonDocument doc(4096);
    if (deserializeJson(doc, line)) continue;

    if (doc.containsKey("id")) {
      int id = doc["id"];
      if (id == 1) { delay(1000); authorizeWorker(); }
      else if (id == 2 && doc["result"] == true) { poolConnected = true; Serial.println("AUTORIZADO!"); }
      else if (id > 2) {
        if (doc["result"] == true) {
          shares_submitted++;
          Serial.println("SHARE ACEITO!!! Total: " + String(shares_submitted));
          saveLog();
        } else shares_rejected++;
      }
    }
    if (doc.containsKey("method") && String(doc["method"].as<const char*>()) == "mining.notify") {
      processMiningNotify(doc["params"].as<JsonArray>());
    }
  }
}

void processMiningJob() {
  if (current_job_id == "") return;
  uint8_t hash[32];
  for (int i = 0; i < 10000; i++) {
    memcpy(job_header + 76, &current_nonce, 4);
    calculate_double_sha256(job_header, hash);
    hashes_calculated++;
    current_nonce++;

    bool ok = true;
    for (int j = 0; j < 32; j++) {
      if (hash[j] > job_target[j]) { ok = false; break; }
      if (hash[j] < job_target[j]) break;
    }
    if (ok) {
      char nonce_hex[9];
      sprintf(nonce_hex, "%08x", current_nonce - 1);
      Serial.println("SHARE VÁLIDO! Nonce: " + String(nonce_hex));
      submitShare(current_job_id, String(nonce_hex));
    }
  }
}

void submitShare(String job, String nonce) {
  String msg = "{\"id\":" + String(millis()) + ",\"method\":\"mining.submit\",\"params\":[\"" +
               coin_type + ":" + trx_wallet + "." + worker_name + "\",\"" + job + "\",\"" + nonce + "\"]}\n";
  poolClient.print(msg);
}

void saveLog() {
  if (!SPIFFS.begin(true)) return;
  File f = SPIFFS.open("/miner_log.txt", "a");
  if (f) {
    f.printf("[%lu] Shares:%lu Rej:%lu %.1f H/s\n", millis()/1000, shares_submitted, shares_rejected,
             hashes_calculated/((millis()-start_time)/1000.0));
    f.close();
  }
}

// ====================== WEB SERVER COMPLETO ======================

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
                        <option value="BTC">Bitcoin (BTC)</option>
                        <option value="BCH">Bitcoin Cash (BCH)</option>
                        <option value="DOGE">Dogecoin (DOGE)</option>
                        <option value="ETH">Etherium (ETH)</option>
                        <option value="LTC">Litecoin (LTC)</option>
                        <option value="TRX">TRON (TRX)</option>
                        <option value="XMR">Monero (XMR)</option>
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
  DynamicJsonDocument doc(2048);
  float sec = (millis() - start_time) / 1000.0;
  float hr = sec > 0 ? hashes_calculated / sec : 0;

  doc["wifi_ssid"] = WiFi.SSID();
  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["uptime"] = String((millis()-start_time)/1000) + "s";
  doc["hashes"] = hashes_calculated;
  doc["hashrate"] = String(hr, 1);
  doc["shares"] = shares_submitted;
  doc["shares_rejected"] = shares_rejected;
  doc["worker"] = worker_name;
  doc["pool"] = pool_host + ":" + pool_port;
  doc["coin"] = coin_type;
  doc["wallet"] = trx_wallet;
  doc["ap_mode"] = isAPMode;
  doc["wifi_connected"] = wifiConnected;
  doc["pool_connected"] = poolConnected;

  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
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
  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleScanWifi() {
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.createNestedArray("networks");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleSaveConfig() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error == DeserializationError::Ok) {  // <--- CORRETO
      wifi_ssid     = doc["wifi_ssid"]     | wifi_ssid;
      wifi_password = doc["wifi_password"] | wifi_password;
      trx_wallet    = doc["wallet"]        | trx_wallet;
      pool_host     = doc["pool_host"]     | pool_host;
      pool_port     = doc["pool_port"]     | pool_port;
      worker_name   = doc["worker_name"]   | worker_name;
      coin_type     = doc["coin_type"]     | coin_type;
      
      saveConfig();
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuração salva! Reiniciando...\"}");
      delay(1500);
      ESP.restart();
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"JSON inválido\"}");
    }
  } else {
    server.send(400, "text/plain", "Nenhum dado recebido");
  }
}

void handleRestart() { server.send(200, "text/plain", "Reiniciando..."); delay(1000); ESP.restart(); }
void handleEnableAP() { startAPMode(); server.send(200, "text/plain", "Modo AP ativado!"); }
void handleDeleteLogs() { SPIFFS.remove("/miner_log.txt"); server.send(200, "text/plain", "Logs apagados!"); }

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/stats", handleApiStats);
  server.on("/api/config", handleApiConfig);
  server.on("/api/scan-wifi", handleScanWifi);
  server.on("/api/save-config", HTTP_POST, handleSaveConfig);
  server.on("/api/restart-miner", handleRestart);
  server.on("/api/enable-ap", handleEnableAP);
  server.on("/api/delete-logs", handleDeleteLogs);
  server.begin();
  Serial.println("WebServer COMPLETO iniciado!");
}

// ====================== SETUP & LOOP ======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP32 MINER TRX - VERSÃO FINAL COMPLETA 2025 ===");

  setupLEDs();
  if (!SPIFFS.begin(true)) Serial.println("Erro SPIFFS!");
  else Serial.println("SPIFFS OK!");

  loadConfig();
  init_sha256_accelerator();
  connectToWiFi();

  if (wifiConnected) MDNS.begin("esp32-miner");

  setupWebServer();
  start_time = millis();

  if (wifiConnected) connectToMiningPool();
}

void loop() {
  server.handleClient();
  updateLEDs();

  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    startAPMode();
  }

  if (wifiConnected && poolClient.connected()) {
    handlePoolResponse();
    if (current_job_id != "") processMiningJob();
  } else if (wifiConnected && !poolClient.connected()) {
    delay(5000);
    connectToMiningPool();
  }

  if (millis() - last_log_save > 300000) { saveLog(); last_log_save = millis(); }
  delay(10);
}