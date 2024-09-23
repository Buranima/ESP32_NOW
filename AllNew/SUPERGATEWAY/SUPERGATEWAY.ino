#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <string.h>
#include <driver/uart.h>

#define UART_TX_PIN 17   // ขา TX สำหรับ UART
#define UART_RX_PIN 16   // ขา RX สำหรับ UART
#define BUF_SIZE (1024)  // ขนาดบัฟเฟอร์ของ UART

// ข้อมูลการเชื่อมต่อ WiFi
const char* wifi_ssid = "Redmipin";
const char* wifi_password = "0868316631";

// ข้อมูลการเชื่อมต่อ MQTT server
const char* mqtt_server_ip = "141.98.17.127";
const int mqtt_server_port = 28813;
const char* mqtt_username = "techlabs";
const char* mqtt_password = "ASDzxc!@#QwE123";
const char* mqtt_subscribe_topic = "CPS485/ESP32";
const char* mqtt_publish_topic = "CPS485/SERVER";

// สร้างตัวแปรสำหรับ WiFi และ MQTT
WiFiClient espWiFiClient;
PubSubClient mqttClient(espWiFiClient);

// โครงสร้างข้อมูลที่จะส่ง
typedef struct {
  uint8_t superMasterMAC[6];
  uint8_t masterMAC[6];
  uint8_t slaveMAC[6];
  char message[16];
} DataMessage;

DataMessage dataMessage;  // ข้อมูลที่จะส่งผ่าน UART หรือ MQTT

// เก็บ MAC Address ของ ESP32
char espMacAddressStr[18];
// MAC Address สำรองถ้าไม่มี master
uint8_t noMasterMAC[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// เก็บ MAC Address ของ SuperMaster, Master, Slave
uint8_t storedDeviceMACs[1][3][6] = {
  {
    // อุปกรณ์ชุดที่ 1
    { 0xcc, 0xdb, 0xa7, 0x33, 0x93, 0xfc },  // SuperMaster MAC
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },  // Master MAC
    { 0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x88 }   // Slave MAC
  }
};

// ฟังก์ชันการตั้งค่าเริ่มต้นสำหรับ WiFi และ UART
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);                                     // ตั้งค่า WiFi เป็นโหมดสถานี (STA)
  connectToWiFi();                                         // เชื่อมต่อ WiFi
  setupUART();                                             // ตั้งค่า UART
  fetchESP32MacAddress();                                  // ดึง MAC Address ของ ESP32
  mqttClient.setServer(mqtt_server_ip, mqtt_server_port);  // ตั้งค่าเซิร์ฟเวอร์ MQTT
  mqttClient.setCallback(handleMQTTMessage);               // ตั้งค่า callback สำหรับการรับข้อความ MQTT

  // เริ่มต้น ESP-NOW สำหรับการสื่อสารแบบไร้สาย
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }
}

// ฟังก์ชันเชื่อมต่อ WiFi
void connectToWiFi() {
  delay(10);
  Serial.println("กำลังเชื่อมต่อ WiFi...");
  WiFi.begin(wifi_ssid, wifi_password);  // เริ่มเชื่อมต่อ WiFi
  // รอจนกว่าจะเชื่อมต่อสำเร็จ
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("เชื่อมต่อ WiFi สำเร็จ");
}

// ฟังก์ชันตั้งค่า UART
void setupUART() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,                    // อัตรา baud
    .data_bits = UART_DATA_8_BITS,          // ข้อมูล 8 บิต
    .parity = UART_PARITY_DISABLE,          // ไม่ใช้ parity
    .stop_bits = UART_STOP_BITS_1,          // ใช้ stop bit 1 บิต
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // ไม่ใช้ flow control
  };
  uart_param_config(UART_NUM_1, &uart_config);                                                 // ตั้งค่าพารามิเตอร์ UART
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  // ตั้งค่าขา TX และ RX
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);                                    // ติดตั้ง driver สำหรับ UART
}

// ฟังก์ชันเชื่อมต่อ WiFi ใหม่ถ้าการเชื่อมต่อขาด
void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi หลุด, กำลังเชื่อมต่อใหม่...");
    WiFi.disconnect();
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("เชื่อมต่อ WiFi ใหม่สำเร็จ");
  }
}

// ฟังก์ชันดึง MAC Address ของ ESP32
void fetchESP32MacAddress() {
  uint8_t mac[6];                      // บัฟเฟอร์สำหรับ MAC address
  esp_wifi_get_mac(WIFI_IF_STA, mac);  // ดึง MAC address ของ ESP32
  snprintf(espMacAddressStr, sizeof(espMacAddressStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);  // แปลงเป็นรูปแบบที่อ่านได้
  Serial.print("MAC Address ของ ESP32: ");
  Serial.println(espMacAddressStr);
}

// ฟังก์ชันเชื่อมต่อ MQTT ใหม่ถ้าการเชื่อมต่อขาด
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("กำลังเชื่อมต่อ MQTT...");
    if (mqttClient.connect(espMacAddressStr, mqtt_username, mqtt_password)) {
      Serial.println("เชื่อมต่อ MQTT สำเร็จ");
      mqttClient.subscribe(mqtt_subscribe_topic);  // สมัครสมาชิกหัวข้อที่ต้องการ
    } else {
      Serial.print("การเชื่อมต่อล้มเหลว, rc=");
      Serial.print(mqttClient.state());  // พิมพ์สถานะการเชื่อมต่อผิดพลาด
      delay(500);
    }
  }
}

// ฟังก์ชัน callback เมื่อได้รับข้อความ MQTT
void handleMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String receivedMessage;
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }
  Serial.println(receivedMessage);

  // แปลงข้อความ JSON ที่ได้รับ
  StaticJsonDocument<200> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, receivedMessage);
  if (error) {
    Serial.print("ไม่สามารถแปลง JSON ได้: ");
    Serial.println(error.c_str());
    return;
  }

  // ดึงค่า mode และ MAC Address จาก JSON
  const char* requestMode = jsonDoc["mode"];
  const char* requestedMAC = jsonDoc["mac"];

  // แปลง MAC Address จาก string เป็น array
  uint8_t macFromMQTT[6];
  convertStringToMac(requestedMAC, macFromMQTT);

  // ถ้าโหมดเป็น "RequestData" ให้ส่งข้อมูลผ่าน UART
  if (strcmp(requestMode, "RequestData") == 0) {
    sendUARTMessage(macFromMQTT);
  }
}

// ฟังก์ชันส่งข้อความ MQTT
void sendMQTTMessage() {
  String messageToSend;
  StaticJsonDocument<200> jsonDoc;

  // แปลง MAC Address ของ slave เป็น string ที่อ่านได้
  char slaveMacStr[18];
  snprintf(slaveMacStr, sizeof(slaveMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           dataMessage.slaveMAC[0], dataMessage.slaveMAC[1], dataMessage.slaveMAC[2],
           dataMessage.slaveMAC[3], dataMessage.slaveMAC[4], dataMessage.slaveMAC[5]);

  // แปลงข้อความเป็นตัวเลข (ถ้ามี)
  float messageAsFloat = atof(dataMessage.message);
  jsonDoc["mac"] = slaveMacStr;        // เพิ่ม MAC address ใน JSON
  jsonDoc["result"] = messageAsFloat;  // เพิ่มผลลัพธ์ใน JSON

  // แปลง JSON และส่งผ่าน MQTT
  serializeJson(jsonDoc, messageToSend);
  if (mqttClient.publish(mqtt_publish_topic, messageToSend.c_str())) {
    Serial.println("ส่งข้อความ MQTT สำเร็จ");
  } else {
    Serial.println("การส่งข้อความ MQTT ล้มเหลว");
  }
}

// ฟังก์ชันส่งข้อความผ่าน UART ตาม MAC Address ที่ได้รับ
void sendUARTMessage(const uint8_t* macFromMQTT) {
  // ตรวจสอบว่า MAC ที่ขอตรงกับ MAC ที่เก็บไว้หรือไม่
  for (int i = 0; i < 1; i++) {
    if (memcmp(storedDeviceMACs[i][2], macFromMQTT, 6) == 0) {
      // ส่งข้อความผ่าน UART
      uart_write_bytes(UART_NUM_1, (const char*)&storedDeviceMACs[i], sizeof(storedDeviceMACs[i]));
      Serial.println("ส่งข้อมูลผ่าน UART สำเร็จ");
      break;
    }
  }
}

// ฟังก์ชันรับข้อมูลจาก UART และส่งต่อผ่าน MQTT
void handleUARTData() {
  uint8_t uartBuffer[BUF_SIZE];
  int len = uart_read_bytes(UART_NUM_1, uartBuffer, sizeof(uartBuffer), 20 / portTICK_RATE_MS);
  if (len > 0) {
    // คัดลอกข้อมูลที่รับมาเก็บไว้ในโครงสร้างข้อมูล
    memcpy(&dataMessage, uartBuffer, sizeof(dataMessage));

    // พิมพ์ MAC Address ที่รับมาและข้อความ
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataMessage.superMasterMAC[0], dataMessage.superMasterMAC[1],
                  dataMessage.superMasterMAC[2], dataMessage.superMasterMAC[3],
                  dataMessage.superMasterMAC[4], dataMessage.superMasterMAC[5]);

    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataMessage.masterMAC[0], dataMessage.masterMAC[1],
                  dataMessage.masterMAC[2], dataMessage.masterMAC[3],
                  dataMessage.masterMAC[4], dataMessage.masterMAC[5]);

    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataMessage.slaveMAC[0], dataMessage.slaveMAC[1],
                  dataMessage.slaveMAC[2], dataMessage.slaveMAC[3],
                  dataMessage.slaveMAC[4], dataMessage.slaveMAC[5]);

    Serial.printf("Message: %s\n", dataMessage.message);
    sendMQTTMessage();  // ส่งข้อมูลที่รับมาไปยัง MQTT
  }
}

// ฟังก์ชันแปลง MAC Address จาก string เป็น byte array
void convertStringToMac(const char* macStr, uint8_t mac[6]) {
  sscanf(macStr, "0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

// ฟังก์ชันหลัก loop
void loop() {
  reconnectWiFi();  // ตรวจสอบการเชื่อมต่อ WiFi
  if (!mqttClient.connected()) {
    reconnectMQTT();  // ตรวจสอบการเชื่อมต่อ MQTT
  }
  mqttClient.loop();  // ประมวลผลข้อความ MQTT
  handleUARTData();   // อ่านข้อมูลจาก UART
}