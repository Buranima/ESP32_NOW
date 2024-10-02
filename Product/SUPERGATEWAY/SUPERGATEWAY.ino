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
  uint8_t masterMacAddress[6];
  uint8_t middleMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[16];
} structMessageSend;
structMessageSend myDatasendMQTTMessage;
structMessageSend myDatacallbackMQTT;
structMessageSend myDatasendUARTMessage;
structMessageSend myDatacallbackUART;

char macAddressMe[6];
char valueADCChar[] = "null";
uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uint8_t macFromMQTT[6];

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setupWiFi();
  WiFi.setTxPower(WIFI_POWER_20dBm);
  setupUART();
  getMACAddress();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callbackMQTT);
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

void reconnectWiFi() {
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

void reconnectMQTT() {
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

void parseMacAddress(const char* macAddressString, uint8_t* macAddressArray) {
  sscanf(macAddressString, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
         &macAddressArray[0], &macAddressArray[1], &macAddressArray[2],
         &macAddressArray[3], &macAddressArray[4], &macAddressArray[5]);
}

void callbackMQTT(char* topicSubscribe, byte* payload, unsigned int length) {
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
  const char* macMasterAddressString = doc["master"];
  const char* macUartAddressString = doc["middle"];
  const char* macSlaveAddressString = doc["slave"];
  if ((memcmp(macAddressMe, myDatacallbackMQTT.masterMacAddress, 6) == 0 && strcmp(modeString, "Reaquestdata") == 0) {
    parseMacAddress(macMasterAddressString, myDatacallbackMQTT.masterMacAddress);
    // parseMacAddress(macUartAddressString, myDatacallbackMQTT.uartMacAddress);
    // parseMacAddress(macSlaveAddressString, myDatacallbackMQTT.slaveMacAddress);
    memcpy(&myDatasendUARTMessage, &myDatacallbackMQTT, sizeof(myDatacallbackMQTT));
    sendUARTMessage();
  }
  // if (strcmp(modeString, "Reaquestdata") == 0) {
  //   parseMacAddress(macMasterAddressString, myDatacallbackMQTT.masterMacAddress);
  //   parseMacAddress(macUartAddressString, myDatacallbackMQTT.uartMacAddress);
  //   parseMacAddress(macSlaveAddressString, myDatacallbackMQTT.slaveMacAddress);
  //   memcpy(&myDatasendUARTMessage, &myDatacallbackMQTT, sizeof(myDatacallbackMQTT));
  //   sendUARTMessage();
  // }
}

void sendMQTTMessage() {
  String mqttMessageSend;
  StaticJsonDocument<200> doc;
  char macAddressString[18];
  float floatMessage;
  snprintf(macAddressString, sizeof(macAddressString), "%02X:%02X:%02X:%02X:%02X:%02X",
           myDatasendMQTTMessage.slaveMacAddress[0], myDatasendMQTTMessage.slaveMacAddress[1],
           myDatasendMQTTMessage.slaveMacAddress[2], myDatasendMQTTMessage.slaveMacAddress[3],
           myDatasendMQTTMessage.slaveMacAddress[4], myDatasendMQTTMessage.slaveMacAddress[5]);

  if (memcmp(valueADCChar, myDatasendMQTTMessage.stringMessage, 6) == 0) {
    doc["result"] = myDatasendMQTTMessage.stringMessage;
  } else {
    floatMessage = atof(myDatasendMQTTMessage.stringMessage);
    doc["result"] = floatMessage;
  }
  doc["mac"] = macAddressString;
  serializeJson(doc, mqttMessageSend);
  Serial.printf("Slave MAC UART: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDatasendMQTTMessage.slaveMacAddress[0], myDatasendMQTTMessage.slaveMacAddress[1],
                myDatasendMQTTMessage.slaveMacAddress[2], myDatasendMQTTMessage.slaveMacAddress[3],
                myDatasendMQTTMessage.slaveMacAddress[4], myDatasendMQTTMessage.slaveMacAddress[5]);
  if (client.publish(topicPublish, mqttMessageSend.c_str())) {
    Serial.println("Sent MQTT");
  } else {
    Serial.println("MQTT Failed to send");
  }
}

void sendUARTMessage() {
  strcpy(myDatasendUARTMessage.stringMessage, "null");
  Serial.println("Send UART");
  uint8_t bufferUART[sizeof(myDatasendUARTMessage)];
  memcpy(bufferUART, &myDatasendUARTMessage, sizeof(myDatasendUARTMessage));
  uart_write_bytes(UART_NUM_1, (const char*)bufferUART, sizeof(bufferUART));
}

void callbackUART() {
  uint8_t bufferUART[sizeof(myDatacallbackUART)];
  int lenUART = uart_read_bytes(UART_NUM_1, bufferUART, sizeof(myDatacallbackUART), 20 / portTICK_RATE_MS);
  if (lenUART > 0) {
    memcpy(&myDatacallbackUART, bufferUART, sizeof(myDatacallbackUART));
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.superMasterMacAddress[0], myDatacallbackUART.superMasterMacAddress[1],
                  myDatacallbackUART.superMasterMacAddress[2], myDatacallbackUART.superMasterMacAddress[3],
                  myDatacallbackUART.superMasterMacAddress[4], myDatacallbackUART.superMasterMacAddress[5]);
    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.masterMacAddress[0], myDatacallbackUART.masterMacAddress[1],
                  myDatacallbackUART.masterMacAddress[2], myDatacallbackUART.masterMacAddress[3],
                  myDatacallbackUART.masterMacAddress[4], myDatacallbackUART.masterMacAddress[5]);
    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.slaveMacAddress[0], myDatacallbackUART.slaveMacAddress[1],
                  myDatacallbackUART.slaveMacAddress[2], myDatacallbackUART.slaveMacAddress[3],
                  myDatacallbackUART.slaveMacAddress[4], myDatacallbackUART.slaveMacAddress[5]);
    Serial.printf("String Message: %s\n", myDatacallbackUART.stringMessage);
    memcpy(&myDatasendMQTTMessage, &myDatacallbackUART, sizeof(myDatacallbackUART));
    sendMQTTMessage();
  }
}

void loop() {
  reconnectWiFi();
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  callbackUART();
}