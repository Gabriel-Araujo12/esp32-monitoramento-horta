#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define LUM_SENSOR 34
#define UMI_SENSOR 35

DHT dht(26, DHT11);

// Configurações WiFi
// CENSURADO
const int wifi_timeout = 10000;

// Configurações MQTT
// CENSURADO
const int mqtt_timeout = 20000;

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

unsigned long lastMQTTReconnect = 0;
unsigned long lastPublish = 0;
const unsigned long publish_interval = 20000;

void connectWiFi(){
  if (WiFi.status() == WL_CONNECTED) return;
  
  Serial.print("Conectando à rede WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  unsigned long startMillis = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMillis < wifi_timeout)) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED){
    Serial.println("\nConectado à rede WiFi com IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFalha na conexão WiFi!");
  }
}

void connectMQTT(){
  if (mqtt_client.connected()) return;
  
  Serial.print("Conectando ao broker MQTT...");
  if (mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
    Serial.println("Conectado!");
  } else {
    Serial.println("Falha na conexão ao broker: Estado=" + String(mqtt_client.state()));
  }
}

void setup() {
  Serial.begin(115200);

  connectWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);

  dht.begin();
}

void loop() {
  // Verificar e manter conexão Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    delay(500);  // Pequeno atraso para estabilizar
  }

  // Tentar reconectar MQTT se desconectado (com intervalo mínimo)
  if (!mqtt_client.connected()) {
    unsigned long currentTime = millis();
    if (currentTime - lastMQTTReconnect > mqtt_timeout) {
      lastMQTTReconnect = currentTime;
      connectMQTT();
    }
  }

  // Mantém a conexão MQTT (obrigatório)
  mqtt_client.loop();

  // Código dos sensores
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("Umidade do ar: ");
  Serial.print(h);
  Serial.print(" / Temperatura do ar: ");
  Serial.println(t);

  int lum = analogRead(LUM_SENSOR);
  Serial.print("Luminosidade do ambiente: ");
  Serial.println(lum);

  int umi = analogRead(UMI_SENSOR);
  Serial.print("Umidade do solo: ");
  Serial.println(umi);
  Serial.println();

  // Publicar variável aleatória se conectado ao MQTT
  if (mqtt_client.connected()) {
    unsigned long currentTime = millis();
    if (currentTime - lastPublish > publish_interval) {
      lastPublish = currentTime;
      
      String payload_1 = String(umi);
      String payload_2 = String(lum);
      String payload_3 = String(h);
      String payload_4 = String(t);
      if (mqtt_client.publish(mqtt_topic_1, payload_1.c_str(), true) && mqtt_client.publish(mqtt_topic_2, payload_2.c_str(), true) && mqtt_client.publish(mqtt_topic_3, payload_3.c_str(), true) && mqtt_client.publish(mqtt_topic_4, payload_4.c_str(), true)) {
        Serial.println("Publicou: " + payload_1);
      } else {
        Serial.println("Falha ao publicar");
      }
    }
  }

  delay(10);
}