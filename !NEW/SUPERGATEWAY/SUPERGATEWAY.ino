#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <string.h>
#include <driver/uart.h>

#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)

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

typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];
  uint8_t masterMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[16];
} structMessageSend;

structMessageSend myDataMessageSend;

char macAddressMe[18];
uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uint8_t macAddressADC[1][3][6] = {
  {
    // ชุดที่ 1
    { 0xcc, 0xdb, 0xa7, 0x32, 0xa6, 0x04 },  // macAddressSuperMaster
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },  // macAddressMaster
    { 0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x74 }   // macAddressSlave
  }
  // { // ชุดที่ 2
  //     {0xA4, 0xB1, 0xC2, 0x01, 0x12, 0x23}, // macAddressSuperMaster
  //     {0x34, 0x45, 0x56, 0x67, 0x78, 0x89}, // macAddressMaster
  //     {0x67, 0x78, 0x89, 0x9A, 0xAB, 0xBC}  // macAddressSlave
  // }
};


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setupWiFi();
  setupUART();
  getMACAddress();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback_mqtt);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }
}

void setupWiFi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void setupUART() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);
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
  snprintf(macAddressMe, sizeof(macAddressMe), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("MAC Address: ");
  Serial.println(macAddressMe);
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(macAddressMe, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(topicSubscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(500);
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
  if (strcmp(modeString, "Reaquestdata") == 0) {
    sendUARTMessage(macFromMQTT);
  }
}

void sendMQTTMessage(String message) {
  if (client.publish(topicPublish, message.c_str())) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Message failed to send");
  }
}

void sendUARTMessage(const uint8_t* macFromMQTT) {
  for (int i = 0; i < 1; i++) {
    if (memcmp(macFromMQTT, macAddressADC[i][2], 6) == 0) {
      memcpy(myDataMessageSend.superMasterMacAddress, macAddressADC[i][0], 6);
      memcpy(myDataMessageSend.masterMacAddress, macAddressADC[i][1], 6);
      memcpy(myDataMessageSend.slaveMacAddress, macAddressADC[i][2], 6);
      strcpy(myDataMessageSend.stringMessage, "null");
      if (memcmp(noMaster, macAddressADC[i][1], 6) == 0) {
        Serial.println("Send UART");
        uint8_t bufferUART[sizeof(myDataMessageSend)];
        memcpy(bufferUART, &myDataMessageSend, sizeof(structMessageSend));
        uart_write_bytes(UART_NUM_1, (const char*)bufferUART, sizeof(bufferUART));
      }
      // else {
      // }
    }
  }
}

void stringToMacArray(const char* macString, uint8_t mac[6]) {
  sscanf(macString, "0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

void loop() {
  reconnect_wifi();
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
}