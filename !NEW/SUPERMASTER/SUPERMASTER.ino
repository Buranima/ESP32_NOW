#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>

// ข้อมูล WiFi
const char* ssid = "BURANIMA";
const char* password = "12345678";

// ข้อมูล MQTT
const char* mqtt_server = "141.98.17.127";
const int mqtt_port = 28813;
const char* mqtt_user = "techlabs";
const char* mqtt_pass = "ASDzxc!@#QwE123";
const char* topicSubscribe = "CPS485/ESP32";
const char* topicPublish = "CPS485/SERVER";

WiFiClient espClient;
PubSubClient client(espClient);

char macAddrMe[18];

void setup() {
  Serial.begin(115200);
  setup_wifi();
  getMACAddress();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback_mqtt);
}

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void reconnect_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi reconnected");
  }
}

void getMACAddress() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  snprintf(macAddrMe, sizeof(macAddrMe), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("MAC Address: ");
  Serial.println(macAddrMe);
}

void stringToMacArray(const char* macStr, uint8_t mac[6]) {
    sscanf(macStr, "0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(macAddrMe, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(topicSubscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void callback_mqtt(char* topicSubscribe, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  const char* modeString = doc["mode"];
  const char* macString = doc["mac"];
  uint8_t macFromMQTT[6];
  stringToMacArray(macString, macFromMQTT);
  Serial.println(modeString);
  Serial.println(macString);
}

void sendMQTTMessage(String message) {
  if (client.publish(topicPublish, message.c_str())) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Message failed to send");
  }
}

void loop() {
  reconnect_wifi();
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  // sendMQTTMessage("{\"on\":-99}");

  delay(5000);
}