#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// แทนที่ด้วยข้อมูลการเชื่อมต่อ WiFi ของคุณ
const char* ssid = "kittisak";
const char* password = "Oct241062";

// การตั้งค่า MQTT
const char* mqtt_server = "141.98.17.127"; // ที่อยู่ของ MQTT broker
const int mqtt_port = 28813;                    // พอร์ตของ MQTT broker
const char* mqtt_user = "techlabs";      // ชื่อผู้ใช้ MQTT (ถ้ามี)
const char* mqtt_pass = "ASDzxc!@#QwE123";      // รหัสผ่าน MQTT (ถ้ามี)
const char* mqtt_topic = "CPS485/SM";   // หัวข้อ MQTT ที่จะสมัครรับ

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  String payloadString;
  for (int i = 0; i < length; i++) {
    payloadString += (char)payload[i];
  } 
  Serial.println(payloadString);

  // แปลข้อมูล JSON
  DynamicJsonDocument doc(2048);  // เพิ่มขนาด buffer ถ้าจำเป็นสำหรับข้อมูลใหญ่ๆ
  DeserializationError error = deserializeJson(doc, payloadString);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // ประมวลผลข้อมูล JSON
  JsonObject root = doc.as<JsonObject>();

  for (JsonPair kv : root) {
    String macAddress = kv.key().c_str();
    JsonArray associatedAddresses = kv.value().as<JsonArray>();

    Serial.print("MAC Address: ");
    Serial.println(macAddress);
    Serial.println("Associated Addresses:");

    for (JsonVariant value : associatedAddresses) {
      Serial.print(" - ");
      Serial.println(value.as<String>());
    }
  }
}

