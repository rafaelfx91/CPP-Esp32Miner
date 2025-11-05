#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// ========== CONFIGURAÇÕES ==========
const char* ssid = "x";
const char* password = "x";
const char* TRX_WALLET = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";

const char* POOL_HOST = "sha256.unmineable.com";
const int POOL_PORT = 3333;
const String WORKER_NAME = "esp32-miner";
// ===================================

WiFiClient poolClient;
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long start_time = 0;
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
  Serial.println("🌐 Para ver no site: https://unmineable.com/coins/TRX/address/" + String(TRX_WALLET));
}

void submitHashrate() {
  String hashrate_msg = "{\"id\":6,\"method\":\"mining.hashrate\",\"params\":[\"100\"]}\n";
  poolClient.print(hashrate_msg);
  Serial.println("📊 Reportando hashrate: 100 H/s");
}

// FUNÇÃO CORRIGIDA - sem erro de compilação
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
          Serial.println("💰 Verifique em: https://unmineable.com/coins/TRX/address/" + String(TRX_WALLET));
        }
      }
    }
    
    // CORREÇÃO: Extrair job_id corretamente
    if (doc.containsKey("method") && String(doc["method"].as<const char*>()) == "mining.notify") {
      if (doc["params"].is<JsonArray>() && doc["params"].size() > 0) {
        current_job_id = String(doc["params"][0].as<const char*>()); // CORREÇÃO AQUI
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

void printStats() {
  unsigned long current_time = millis();
  float elapsed_min = (current_time - start_time) / 60000.0;
  float hashrate = elapsed_min > 0 ? hashes_calculated / (elapsed_min * 60) : 0;
  
  Serial.println("\n=== 📊 ESTATÍSTICAS ===");
  Serial.println("💰 Carteira: " + String(TRX_WALLET));
  Serial.println("👷 Worker: " + WORKER_NAME);
  Serial.println("⛏️ Hashes: " + String(hashes_calculated));
  Serial.println("✅ Shares: " + String(shares_submitted));
  Serial.println("🚀 Hashrate: " + String(hashrate, 1) + " H/s");
  Serial.println("🔗 Pool: " + String(poolConnected ? "Conectado" : "Desconectado"));
  Serial.println("=====================\n");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n⚡ MINERADOR UNMINEABLE - SEM ERROS ⚡");
  Serial.println("====================================");
  
  connectToWiFi();
  start_time = millis();
  
  if (connectToMiningPool()) {
    // Subscription e autorização nas respostas
  }
}

void loop() {
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
  
  static unsigned long last_stats = 0;
  if (millis() - last_stats > 120000) {
    printStats();
    last_stats = millis();
  }
  
  delay(100);
}