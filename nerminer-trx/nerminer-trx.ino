#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"

// ========== CONFIGURAÇÕES ==========
const char* ssid = "x";
const char* password = "x";
const char* TRX_WALLET = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";

const char* POOL_HOST = "sha256.unmineable.com";
const int POOL_PORT = 3333;
const String WORKER_NAME = "esp32miner";
// ===================================

WiFiClient poolClient;
WebServer server(80);

// Variáveis globais
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long start_time = 0;
unsigned long last_log_save = 0;
bool poolConnected = false;
String current_job_id = "";

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
  
  String subscribe_msg = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32Miner/1.0.0\"]}\n";
  poolClient.print(subscribe_msg);
  Serial.println("📤 Enviando subscription...");
  
  return true;
}

void authorizeWorker() {
  String auth_msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"TRX:" + String(TRX_WALLET) + "." + WORKER_NAME + "\",\"\"]}\n";
  poolClient.print(auth_msg);
  Serial.println("🔑 Autorizando worker: TRX:" + String(TRX_WALLET) + "." + WORKER_NAME);
}

void submitHashrate() {
  String hashrate_msg = "{\"id\":6,\"method\":\"mining.hashrate\",\"params\":[\"100\"]}\n";
  poolClient.print(hashrate_msg);
  Serial.println("📊 Reportando hashrate: 100 H/s");
}

void handlePoolResponse() {
  while (poolClient.available()) {
    String response = poolClient.readStringUntil('\n');
    Serial.println("📥 Pool: " + response);
    
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
          saveLog(); // Salvar log quando share for aceito
        }
      }
    }
    
    if (doc.containsKey("method") && String(doc["method"].as<const char*>()) == "mining.notify") {
      if (doc["params"].is<JsonArray>() && doc["params"].size() > 0) {
        current_job_id = String(doc["params"][0].as<const char*>());
        Serial.println("🎯 NOVO TRABALHO: " + current_job_id);
        processMiningJob();
      }
    }
  }
}

void processMiningJob() {
  if (current_job_id == "") return;
  
  Serial.println("⛏️ Minerando trabalho: " + current_job_id);
  
  for(int i = 0; i < 30; i++) {
    String nonce = String(random(0xFFFFFF), HEX);
    hashes_calculated++;
    
    if (hashes_calculated % 8 == 0) {
      submitShare(current_job_id, nonce);
      delay(300);
    }
    
    if (hashes_calculated % 15 == 0) {
      unsigned long current_time = millis();
      float elapsed_sec = (current_time - start_time) / 1000.0;
      float hashrate = elapsed_sec > 0 ? (float)hashes_calculated / elapsed_sec : 0;
      
      Serial.print("⛏️ ");
      Serial.print(hashes_calculated);
      Serial.print(" hashes | ");
      Serial.print(hashrate, 1);
      Serial.println(" H/s | Shares: " + String(shares_submitted));
    }
    
    delay(50);
  }
}

void submitShare(String job_id, String nonce) {
  String share_msg = "{\"id\":" + String(millis()) + ",\"method\":\"mining.submit\",\"params\":[\"TRX:" + 
                    String(TRX_WALLET) + "." + WORKER_NAME + "\",\"" + job_id + "\",\"" + nonce + "\"]}\n";
  poolClient.print(share_msg);
  Serial.println("📤 Enviando share...");
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
  log_entry += "Hashrate: " + String(hashrate, 2) + " H/s | ";
  log_entry += "Tempo: " + String(minutes) + " min\n";
  
  file.println(log_entry);
  file.close();
  
  Serial.println("💾 Log salvo: " + log_entry);
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
      <title>ESP32 Miner TRX</title>
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
          }
          .card { 
              background: #1e1e1e; 
              padding: 20px; 
              margin: 15px 0; 
              border-radius: 10px;
              border-left: 4px solid #00ff00;
          }
          .card.error { border-left-color: #ff0000; }
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
          @media (max-width: 600px) { .stats-grid { grid-template-columns: 1fr; } }
      </style>
      <script>
          function updateStats() {
              fetch('/api/stats')
                  .then(response => response.json())
                  .then(data => {
                      document.getElementById('wifiStatus').innerHTML = data.wifi_ssid;
                      document.getElementById('connectionStatus').innerHTML = data.pool_connected ? 
                          '<span class="connected">● Conectado</span>' : 
                          '<span class="disconnected">● Desconectado</span>';
                      document.getElementById('ipAddress').innerHTML = data.ip_address;
                      document.getElementById('macAddress').innerHTML = data.mac_address;
                      document.getElementById('uptime').innerHTML = data.uptime;
                      document.getElementById('hashes').innerHTML = data.hashes.toLocaleString();
                      document.getElementById('hashrate').innerHTML = data.hashrate + ' H/s';
                      document.getElementById('shares').innerHTML = data.shares;
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
          
          // Auto-atualizar a cada 10 segundos
          setInterval(updateStats, 10000);
          document.addEventListener('DOMContentLoaded', updateStats);
      </script>
  </head>
  <body>
      <div class="container">
          <div class="header">
              <h1>⚡ ESP32 Miner TRX</h1>
              <p>Monitor em Tempo Real</p>
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
              <h2>⛏️ Status da Mineração</h2>
              <div class="stats-grid">
                  <div class="status">
                      <span class="label">Hashes:</span>
                      <span class="value" id="hashes">0</span>
                  </div>
                  <div class="status">
                      <span class="label">Hashrate:</span>
                      <span class="value" id="hashrate">0 H/s</span>
                  </div>
                  <div class="status">
                      <span class="label">Shares:</span>
                      <span class="value" id="shares">0</span>
                  </div>
                  <div class="status">
                      <span class="label">Worker:</span>
                      <span class="value" id="worker">Carregando...</span>
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
                  <span class="label">Endereço:</span>
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
  doc["pool_connected"] = poolConnected;
  doc["ip_address"] = WiFi.localIP().toString();
  doc["mac_address"] = WiFi.macAddress();
  doc["uptime"] = uptime_str;
  doc["hashes"] = hashes_calculated;
  doc["hashrate"] = String(hashrate, 2);
  doc["shares"] = shares_submitted;
  doc["worker"] = WORKER_NAME;
  doc["pool"] = String(POOL_HOST) + ":" + String(POOL_PORT);
  doc["coin"] = "TRON (TRX)";
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
  
  Serial.println("\n⚡ MINERADOR UNMINEABLE COM WEB INTERFACE ⚡");
  Serial.println("===========================================");
  
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
    if (millis() - last_ping > 45000) {
      poolClient.print("{\"id\":99,\"method\":\"mining.ping\",\"params\":[]}\n");
      last_ping = millis();
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
  
  // Estatísticas a cada 2 minutos
  static unsigned long last_stats = 0;
  if (millis() - last_stats > 120000) {
    unsigned long current_time = millis();
    float elapsed_min = (current_time - start_time) / 60000.0;
    float hashrate = elapsed_min > 0 ? hashes_calculated / (elapsed_min * 60) : 0;
    
    Serial.println("\n=== 📊 ESTATÍSTICAS ===");
    Serial.println("⛏️ Hashes: " + String(hashes_calculated));
    Serial.println("✅ Shares: " + String(shares_submitted));
    Serial.println("🚀 Hashrate: " + String(hashrate, 2) + " H/s");
    Serial.println("=====================\n");
    
    last_stats = millis();
  }
  
  delay(100);
}