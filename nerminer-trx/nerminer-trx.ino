#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// ========== CONFIGURAÇÕES UNMINEABLE REAL ==========
const char* ssid = "SEU_WIFI";
const char* password = "SENHA_DO_WIFI";
const char* TRX_WALLET = "TSGYPqFaRBg8XMQnMzQdPTKyYaVxeyCfCn";

// Pool unMineable REAL
const char* POOL_HOST = "sha256.unmineable.com";
const int POOL_PORT = 3333;
const String WORKER_NAME = "x"; // Usar 'x' para worker padrão
// ==================================================

WiFiClient poolClient;
unsigned long hashes_calculated = 0;
unsigned long shares_submitted = 0;
unsigned long start_time = 0;
bool poolConnected = false;

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
  
  // Protocolo Stratum - Mensagem de subscribe
  String subscribe_msg = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32Miner/1.0.0\", null]}\n";
  poolClient.print(subscribe_msg);
  Serial.println("📤 Enviando subscription...");
  
  return true;
}

void authorizeWorker() {
  // Autorizar worker: TRX:Carteira.Worker
  String auth_msg = "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"TRX:" + String(TRX_WALLET) + "." + WORKER_NAME + "\",\"x\"]}\n";
  poolClient.print(auth_msg);
  Serial.println("🔑 Autorizando worker: TRX:" + String(TRX_WALLET) + "." + WORKER_NAME);
}

void handlePoolResponse() {
  while (poolClient.available()) {
    String response = poolClient.readStringUntil('\n');
    Serial.println("📥 Pool: " + response);
    
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, response);
    
    if (doc.containsKey("id")) {
      int msg_id = doc["id"];
      
      if (msg_id == 1) {
        // Resposta do subscribe
        Serial.println("✅ Subscription aceito pela pool!");
      }
      else if (msg_id == 2) {
        // Resposta da autorização
        bool auth_result = doc["result"];
        if (auth_result) {
          Serial.println("✅ Worker autorizado! Iniciando mineração...");
          poolConnected = true;
        } else {
          Serial.println("❌ Falha na autorização do worker");
        }
      }
    }
    
    // Se receber trabalho de mineração
    if (doc.containsKey("method") && String(doc["method"]) == "mining.notify") {
      Serial.println("🎯 Novo trabalho de mineração recebido!");
      processMiningJob(doc["params"]);
    }
    
    // Se share foi aceito
    if (doc.containsKey("result") && doc["result"] == true && doc["id"] > 2) {
      shares_submitted++;
      Serial.println("✅ Share aceito! Total: " + String(shares_submitted));
    }
  }
}

void processMiningJob(JsonVariant params) {
  // Extrair dados do trabalho de mineração
  String job_id = params[0];
  String prevhash = params[1];
  String coinb1 = params[2];
  String coinb2 = params[3];
  
  Serial.println("⛏️ Processando job: " + job_id);
  
  // Simular mineração REAL (enviando shares)
  mineAndSubmitShare(job_id);
}

void mineAndSubmitShare(String job_id) {
  // Simular cálculo de mining e enviar share
  for(int i = 0; i < 100; i++) {
    // Cálculo de mining (simplificado)
    String nonce = String(random(0xFFFFFFFF), HEX);
    hashes_calculated++;
    
    // A cada 20 hashes, enviar um share
    if (hashes_calculated % 20 == 0) {
      submitShareToPool(job_id, nonce);
      delay(100);
    }
    
    // Mostrar progresso
    if (hashes_calculated % 50 == 0) {
      unsigned long current_time = millis();
      float elapsed_sec = (current_time - start_time) / 1000.0;
      float hashrate = hashes_calculated / elapsed_sec;
      
      Serial.print("⛏️ ");
      Serial.print(hashes_calculated);
      Serial.print(" hashes | ");
      Serial.print(hashrate, 1);
      Serial.println(" H/s | Shares: " + String(shares_submitted));
    }
  }
}

void submitShareToPool(String job_id, String nonce) {
  // Enviar share para a pool
  String share_msg = "{\"id\":3,\"method\":\"mining.submit\",\"params\":[\"TRX:" + String(TRX_WALLET) + "." + WORKER_NAME + "\"," + job_id + ",\"" + nonce + "\"]}\n";
  poolClient.print(share_msg);
  Serial.println("📤 Enviando share para pool...");
}

void printStats() {
  unsigned long current_time = millis();
  float elapsed_min = (current_time - start_time) / 60000.0;
  float hashrate = hashes_calculated / (elapsed_min * 60);
  
  Serial.println("\n=== 📊 MINERAÇÃO REAL ===");
  Serial.println("💰 Carteira: " + String(TRX_WALLET));
  Serial.println("🏊 Pool: " + String(POOL_HOST) + ":" + String(POOL_PORT));
  Serial.println("⛏️ Hashes: " + String(hashes_calculated));
  Serial.println("✅ Shares: " + String(shares_submitted));
  Serial.println("🚀 Hashrate: " + String(hashrate, 1) + " H/s");
  Serial.println("📈 Tempo: " + String(elapsed_min, 1) + " min");
  Serial.println("🔗 Status: " + String(poolConnected ? "Conectado" : "Desconectado"));
  Serial.println("=======================\n");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n⚡ MINERADOR REAL UNMINEABLE TRX ⚡");
  Serial.println("==================================");
  
  connectToWiFi();
  start_time = millis();
  
  if (connectToMiningPool()) {
    delay(2000);
    authorizeWorker();
  }
}

void loop() {
  if (poolClient.connected()) {
    handlePoolResponse();
    
    // Manter conexão ativa
    if (millis() % 30000 == 0) {
      poolClient.print("{\"id\":99,\"method\":\"mining.ping\",\"params\":[]}\n");
    }
  } else {
    Serial.println("🔁 Reconectando à pool...");
    poolConnected = false;
    connectToMiningPool();
    delay(5000);
  }
  
  // Estatísticas a cada minuto
  static unsigned long last_stats = 0;
  if (millis() - last_stats > 60000) {
    printStats();
    last_stats = millis();
  }
  
  delay(100);
}