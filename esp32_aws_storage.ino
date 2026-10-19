#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <time.h>

// ================= CONFIGURAÇÕES DE REDE =================
const char* ssid = "sua_rede";
const char* password = "sua_senha";

// ================= CONFIGURAÇÕES AWS IoT =================
const char* mqtt_server = "server_mqtt";
const int mqtt_port = 8883;
const char* topico_publicacao = "sdk/test/js";

// COLOQUE AQUI OS NOMES EXATOS DOS ARQUIVOS QUE VOCÊ ENVIOU PELO WEB MANAGER!
const char* rootCA_path = "/.pem";
const char* cert_path   = "/.cert.pem"; // <-- MUDE PARA O NOME DO SEU CERTIFICADO NOVO
const char* key_path    = "/.private.key";  // <-- MUDE PARA O NOME DA SUA CHAVE NOVA

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

unsigned long lastPublishTime = 0;
const long publishInterval = 5000; // Envia dados a cada 5 segundos

// ================= FUNÇÕES AUXILIARES =================

// Lê arquivos do LittleFS
String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.printf("❌ Erro ao abrir o arquivo: %s\n", path);
    return "";
  }
  String content = file.readString();
  file.close();
  content.replace("\r", ""); // Remove quebras de linha do Windows
  return content;
}

// Sincroniza a hora do ESP32 via NTP (OBRIGATÓRIO PARA AWS)
void setClock() {
  Serial.print("⏳ Sincronizando relógio via NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" ✅ Relógio sincronizado!");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n🚀 Iniciando ESP32 AWS IoT Core");

  // 1. Inicializa LittleFS
  if (!LittleFS.begin()) {
    Serial.println("❌ Erro ao montar LittleFS!");
    while (1);
  }

  // 2. Conecta ao WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("🔄 Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado!");

  // 3. Sincroniza a Hora
  setClock();

  // 4. Carrega Certificados
  String rootCA = readFile(rootCA_path);
  String cert   = readFile(cert_path);
  String key    = readFile(key_path);

  if (rootCA.isEmpty() || cert.isEmpty() || key.isEmpty()) {
    Serial.println("❌ Falha grave: Um ou mais certificados não foram encontrados no LittleFS!");
    while (1);
  }

  secureClient.setCACert(rootCA.c_str());
  secureClient.setCertificate(cert.c_str());
  secureClient.setPrivateKey(key.c_str());

  // 5. Configura MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(512); // Buffer maior para o JSON
}

// ================= LOOP =================
void loop() {
  // Mantém a conexão MQTT ativa
  if (!mqttClient.connected()) {
    Serial.print("\n🔄 Conectando ao AWS IoT Core... ");
    
    // O Client ID OBRIGATORIAMENTE precisa começar com sdk-nodejs- por causa da sua Policy
    String clientId = "sdk-nodejs-esp32-device"; 
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("✅ Conectado com sucesso!");
    } else {
      Serial.print("❌ Falha, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
      return; // Pula o resto do loop até conseguir reconectar
    }
  }
  mqttClient.loop();

  // Publica dados a cada 5 segundos
  unsigned long now = millis();
  if (now - lastPublishTime > publishInterval) {
    lastPublishTime = now;

    // Gera dados simulados (temperatura e umidade)
    float temperatura = 20.0 + random(0, 100) / 10.0;
    float umidade = 50.0 + random(0, 200) / 10.0;

    // Monta o payload em formato JSON compatível com seu EC2/RDS
    char payload[150];
    snprintf(payload, sizeof(payload), "{\"temperatura\": %.1f, \"umidade\": %.1f, \"dispositivo\": \"ESP32_AWS\"}", temperatura, umidade);

    // Publica no tópico configurado
    if (mqttClient.publish(topico_publicacao, payload)) {
      Serial.printf("📤 Publicado em %s: %s\n", topico_publicacao, payload);
    } else {
      Serial.println("❌ Erro ao publicar dados.");
    }
  }
}