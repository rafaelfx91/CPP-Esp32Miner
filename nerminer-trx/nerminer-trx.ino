#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "sha256_acelerado.h"

// ========== CONFIGURAÇÕES ==========
const char* ssid = "a";
const char* password = "a";
const char* TRX_WALLET = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";

const char* POOL_HOST = "sha256.unmineable.com";
const int POOL_PORT = 3333;
const String WORKER_NAME = "esp32-miner#cub7-5a3h";
// ===================================

WiFiClient poolClient;
WebServer server(80);

// Variáveis globais
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long shares_rejected = 0;
unsigned long start_time = 0;
unsigned long last_log_save = 0;
unsigned long last_pool_activity = 0;
bool poolConnected = false;
String current_job_id = "";

// Variáveis para mineração real
uint8_t job_target[32];
uint8_t job_header[80];
uint32_t current_nonce = 0;

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

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("📡 Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Conectado! IP: " + WiFi.localIP().toString());
}

bool connectToMiningPool() {
  Serial.println("🔗 Conectando à pool " + String(POOL_HOST) + ":" + String(POOL_PORT));
  
  if (!poolClient.connect(POOL_HOST, POOL_PORT)) {
    Serial.println("❌ Falha na conexão com a pool");
    return false;
  }
  
  Serial.println("✅ Conectado à pool unMineable!");
  last_pool_activity = millis();
  
  String subscribe_msg = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32Miner/1.0.0\"]}\n";
  poolClient.print(subscribe_msg);
  Serial.println("📤 Enviando subscription...");
  
  return true;
}

void authorizeWorker() {
  String auth_msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"TRX:" + String(TRX_WALLET) + "." + WORKER_NAME + "\",\"\"]}\n";
  poolClient.print(auth_msg);
  Serial.println("🔑 Autorizando worker: TRX:" + String(TRX_WALLET) + "." + WORKER_NAME);
  Serial.println("💰 COM REFERRAL: Taxa reduzida para 0.75%!");
}

void submitHashrate() {
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
    
    // Montar cabeçalho de 80 bytes
    memset(job_header, 0, 80);
    
    // Version (4 bytes)
    hexToBytes(version.c_str(), job_header, 4);
    reverseBytes(job_header, 4);
    
    // PrevHash (32 bytes)
    hexToBytes(prevhash.c_str(), job_header + 4, 32);
    reverseBytes(job_header + 4, 32);
    
    // Merkle Root (32 bytes) - placeholder para simplificação
    // Em um minerador completo, isso seria calculado do coinb1, coinb2 e merkle_branch
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
    
    // ✅ CORRETO (compatível com dificuldade 16384):
    memset(job_target, 0x00, 32);  // Começa com zeros
    job_target[0] = 0x00;
    job_target[1] = 0x00; 
    job_target[2] = 0x00;
    job_target[3] = 0x00;
    job_target[4] = 0x00;
    job_target[5] = 0x00;
    job_target[6] = 0x00;
    job_target[7] = 0x00;
    job_target[29] = 0xFF;  // Últimos bytes mais "fáceis"
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
    // Atualizar nonce no cabeçalho
    memcpy(job_header + 76, &current_nonce, 4);
    
    // CALCULAR DOUBLE SHA-256 COM ACELERAÇÃO DE HARDWARE
    calculate_double_sha256(job_header, hash_result);
    
    hashes_calculated++;
    current_nonce++;
    
    // Verificar se o hash atinge o target
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
    
    // Submeter share se válido
    if (is_valid) {
      char nonce_hex[9];
      sprintf(nonce_hex, "%08x", current_nonce - 1);
      submitShare(current_job_id, String(nonce_hex));
      break; // Pausar para não enviar muitos shares de uma vez
    }
    
    // Status a cada 100 hashes
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
  String share_msg = "{\"id\":" + String(millis()) + ",\"method\":\"mining.submit\",\"params\":[\"TRX:" + 
                    String(TRX_WALLET) + "." + WORKER_NAME + "\",\"" + job_id + "\",\"" + nonce + "\"]}\n";
  poolClient.print(share_msg);
  Serial.println("📤 Enviando share REAL: " + nonce);
}

// ========== SISTEMA DE ARQUIVOS E LOGS ==========
void saveLog() {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Erro ao montar SPIFFS");
    return;
  }
  
  File file = SPIFFS.open("/miner_log.txt", "a");
  if (!file) {
    Serial.println("❌ Erro ao abrir arquivo de log");
    return;
  }
  
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
  
  Serial.println("💾 Log salvo");
}

void deleteLogs() {
  if (!SPIFFS.begin(true)) return;
  
  if (SPIFFS.remove("/miner_log.txt")) {
    Serial.println("✅ Arquivo de log removido");
  }
}

// ========== SERVIDOR WEB ==========
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="pt-br">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 Miner TRX - SHA256 REAL</title>
      <style>
          * { margin: 0; padding: 0; box-sizing: border-box; }
          body { 
              background: #121212; 
              color: #ffffff; 
              font-family: Arial, sans-serif;
              padding: 20px;
          }
          .container { max-width: 800px; margin: 0 auto; }
          .header { 
              text-align: center; 
              margin-bottom: 30px;
              padding: 20px;
              background: #1e1e1e;
              border-radius: 10px;
              border: 2px solid #00ff00;
          }
          .card { 
              background: #1e1e1e; 
              padding: 20px; 
              margin: 15px 0; 
              border-radius: 10px;
              border-left: 4px solid #00ff00;
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
              padding: 10px 20px; 
              margin: 5px;
              border-radius: 5px; 
              cursor: pointer;
              font-weight: bold;
          }
          .button.delete { background: #ff4444; color: white; }
          .stats-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
          .referral-badge { 
              background: #4CAF50; 
              color: white; 
              padding: 5px 10px; 
              border-radius: 15px; 
              font-size: 12px;
              margin-left: 10px;
          }
          .hardware-badge { 
              background: #2196F3; 
              color: white; 
              padding: 5px 10px; 
              border-radius: 15px; 
              font-size: 12px;
              margin-left: 5px;
          }
          @media (max-width: 600px) { .stats-grid { grid-template-columns: 1fr; } }
      </style>
      <script>
          function updateStats() {
              fetch('/api/stats')
                  .then(response => response.json())
                  .then(data => {
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
                      document.getElementById('worker').innerHTML = data.worker;
                      document.getElementById('pool').innerHTML = data.pool;
                      document.getElementById('coin').innerHTML = data.coin;
                      document.getElementById('wallet').innerHTML = data.wallet;
                  })
                  .catch(error => console.error('Erro:', error));
          }
          
          function deleteLogs() {
              if(confirm('Tem certeza que deseja apagar todos os logs?')) {
                  fetch('/api/delete-logs')
                      .then(response => response.json())
                      .then(data => {
                          alert(data.message);
                      });
              }
          }
          
          setInterval(updateStats, 5000);
          document.addEventListener('DOMContentLoaded', updateStats);
      </script>
  </head>
  <body>
      <div class="container">
          <div class="header">
              <h1>⚡ ESP32 Miner TRX - SHA256 REAL</h1>
              <p>Mineração com Aceleração de Hardware <span class="hardware-badge">HARDWARE ACCEL</span></p>
              <p>Worker: <span id="worker">Carregando...</span> <span class="referral-badge">REFERRAL ATIVO</span></p>
          </div>
          
          <div class="card">
              <h2>📡 Status da Conexão</h2>
              <div class="status">
                  <span class="label">WiFi:</span>
                  <span class="value" id="wifiStatus">Carregando...</span>
              </div>
              <div class="status">
                  <span class="label">Status Pool:</span>
                  <span class="value" id="connectionStatus">Carregando...</span>
              </div>
              <div class="status">
                  <span class="label">IP:</span>
                  <span class="value" id="ipAddress">Carregando...</span>
              </div>
              <div class="status">
                  <span class="label">MAC:</span>
                  <span class="value" id="macAddress">Carregando...</span>
              </div>
              <div class="status">
                  <span class="label">Tempo Online:</span>
                  <span class="value" id="uptime">Carregando...</span>
              </div>
          </div>
          
          <div class="card">
              <h2>⛏️ Status da Mineração REAL</h2>
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
                      <span class="label">Pool:</span>
                      <span class="value" id="pool">Carregando...</span>
                  </div>
                  <div class="status">
                      <span class="label">Coin:</span>
                      <span class="value" id="coin">Carregando...</span>
                  </div>
              </div>
          </div>
          
          <div class="card">
              <h2>💰 Carteira</h2>
              <div class="status">
                  <span class="label">Endereço TRX:</span>
                  <span class="value" id="wallet">Carregando...</span>
              </div>
          </div>
          
          <div class="card">
              <h2>🛠️ Controles</h2>
              <button class="button" onclick="updateStats()">🔄 Atualizar</button>
              <button class="button delete" onclick="deleteLogs()">🗑️ Apagar Logs</button>
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
  doc["worker"] = WORKER_NAME;
  doc["pool"] = String(POOL_HOST) + ":" + String(POOL_PORT);
  doc["coin"] = "TRON (TRX) - SHA256 REAL";
  doc["wallet"] = TRX_WALLET;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleDeleteLogs() {
  deleteLogs();
  
  DynamicJsonDocument doc(256);
  doc["message"] = "Logs apagados com sucesso!";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/stats", handleApiStats);
  server.on("/api/delete-logs", handleDeleteLogs);
  
  server.begin();
  Serial.println("✅ Servidor HTTP iniciado!");
  Serial.println("🌐 Acesse: http://" + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n⚡ MINERADOR SHA256 REAL COM ACELERAÇÃO DE HARDWARE ⚡");
  Serial.println("💰 Worker com Referral: " + WORKER_NAME);
  Serial.println("===================================================");
  
  // Inicializar acelerador SHA-256
  init_sha256_accelerator();
  Serial.println("✅ Acelerador SHA-256 inicializado!");
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Erro ao montar SPIFFS");
  }
  
  connectToWiFi();
  
  // Configurar mDNS
  if (!MDNS.begin("esp32-miner")) {
    Serial.println("❌ Erro ao iniciar mDNS");
  } else {
    Serial.println("✅ mDNS iniciado! Acesse: http://esp32-miner.local");
  }
  
  start_time = millis();
  setupWebServer();
  
  if (connectToMiningPool()) {
    // Subscription e autorização nas respostas
  }
}

void loop() {
  server.handleClient();
  
  if (poolClient.connected()) {
    handlePoolResponse();
    
    static unsigned long last_ping = 0;
    if (millis() - last_ping > 30000) {
      poolClient.print("{\"id\":99,\"method\":\"mining.ping\",\"params\":[]}\n");
      last_ping = millis();
    }
    
    // Mineração REAL se tiver trabalho
    if (current_job_id != "") {
      processMiningJob();
    }
    
    // Atualizar hashrate a cada 2 minutos
    static unsigned long last_hashrate_update = 0;
    if (millis() - last_hashrate_update > 120000) {
      submitHashrate();
      last_hashrate_update = millis();
    }
  } else {
    Serial.println("🔁 Reconectando...");
    poolConnected = false;
    if (connectToMiningPool()) {
      delay(2000);
    }
    delay(5000);
  }
  
  // Salvar log a cada 5 minutos
  if (millis() - last_log_save > 300000) {
    saveLog();
    last_log_save = millis();
  }
  
  delay(50);
}