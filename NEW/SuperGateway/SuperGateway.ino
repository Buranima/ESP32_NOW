#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <string.h>
#include <driver/uart.h>

#define UART_TX_PIN 17   // พินสำหรับส่งข้อมูลผ่าน UART
#define UART_RX_PIN 16   // พินสำหรับรับข้อมูลผ่าน UART
#define BUF_SIZE (1024)  // ขนาดบัฟเฟอร์สำหรับ UART

// ข้อมูล WiFi
const char* ssid = "BURANIMA";      // ชื่อ WiFi
const char* password = "12345678";  // รหัสผ่าน WiFi

// ข้อมูล MQTT
const char* mqtt_server = "141.98.17.127";    // ที่อยู่เซิร์ฟเวอร์ MQTT
const int mqtt_port = 28813;                  // พอร์ตของเซิร์ฟเวอร์ MQTT
const char* mqtt_user = "techlabs";           // ชื่อผู้ใช้ MQTT
const char* mqtt_pass = "ASDzxc!@#QwE123";    // รหัสผ่าน MQTT
const char* topicSubscribe = "CPS485/ESP32";  // หัวข้อสำหรับสมัครรับข้อมูล
const char* topicPublish = "CPS485/SERVER";   // หัวข้อสำหรับส่งข้อมูล

WiFiClient espClient;                // ตัวเชื่อมต่อ WiFi
PubSubClient mqttClient(espClient);  // ตัวเชื่อมต่อ MQTT

// โครงสร้างสำหรับส่งข้อมูล
typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];  // ที่อยู่ MAC ของ Super Master
  uint8_t masterMacAddress[6];       // ที่อยู่ MAC ของ Master
  uint8_t slaveMacAddress[6];        // ที่อยู่ MAC ของ Slave
  char stringMessage[16];            // ข้อความที่ต้องการส่ง
} structMessageSend;

structMessageSend sendMQTTData;     // ข้อมูลสำหรับส่งผ่าน MQTT
structMessageSend receiveMQTTData;  // ข้อมูลที่รับจาก MQTT
structMessageSend sendUARTData;     // ข้อมูลสำหรับส่งผ่าน UART
structMessageSend receiveUARTData;  // ข้อมูลที่รับจาก UART

char macAddressMe[18];                                        // ที่อยู่ MAC ของตัวเอง
char valueADCChar[] = "null";                                 // ค่าที่ส่งผ่าน ADC
uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // ที่อยู่ MAC สำหรับ Master ที่ไม่มี
uint8_t macFromMQTT[6];                                       // ที่อยู่ MAC ที่รับจาก MQTT

void setup() {
  Serial.begin(115200);                          // เริ่มต้นการสื่อสารผ่าน Serial
  WiFi.mode(WIFI_STA);                           // ตั้งค่า WiFi เป็นโหมดสถานี
  setupWiFi();                                   // เรียกฟังก์ชันตั้งค่า WiFi
  WiFi.setTxPower(WIFI_POWER_20dBm);             // ตั้งค่าพลังส่ง WiFi
  setupUART();                                   // เรียกฟังก์ชันตั้งค่า UART
  getMACAddress();                               // รับที่อยู่ MAC ของตัวเอง
  mqttClient.setServer(mqtt_server, mqtt_port);  // ตั้งค่าเซิร์ฟเวอร์ MQTT
  mqttClient.setCallback(callbackMQTT);          // ตั้งฟังก์ชัน callback สำหรับ MQTT
  if (esp_now_init() != ESP_OK) {                // เริ่มต้น ESP-NOW
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }
}

void setupWiFi() {
  Serial.println("Connecting to WiFi...");  // แจ้งสถานะการเชื่อมต่อ WiFi
  WiFi.begin(ssid, password);               // เริ่มเชื่อมต่อ WiFi
  while (WiFi.status() != WL_CONNECTED) {   // รอจนกว่า WiFi จะเชื่อมต่อสำเร็จ
    delay(300);
    Serial.print(".");  // แสดงสถานะการเชื่อมต่อ
  }
  Serial.println("WiFi connected");  // แจ้งว่าการเชื่อมต่อ WiFi สำเร็จ
}

void setupUART() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,                    // ความเร็วในการส่งข้อมูล
    .data_bits = UART_DATA_8_BITS,          // จำนวนบิตข้อมูล
    .parity = UART_PARITY_DISABLE,          // ไม่มีการตรวจสอบ parity
    .stop_bits = UART_STOP_BITS_1,          // จำนวนบิตหยุด
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // ปิดการควบคุมการไหล
  };
  uart_param_config(UART_NUM_1, &uart_config);                                                 // ตั้งค่าการสื่อสาร UART
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  // กำหนดพิน UART
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);                                    // ติดตั้งไดรเวอร์ UART
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {                     // ตรวจสอบสถานะการเชื่อมต่อ WiFi
    Serial.println("WiFi disconnected, reconnecting...");  // แจ้งว่า WiFi ขาดการเชื่อมต่อ
    WiFi.disconnect();                                     // ตัดการเชื่อมต่อ WiFi
    WiFi.begin(ssid, password);                            // เริ่มเชื่อมต่อ WiFi ใหม่
    while (WiFi.status() != WL_CONNECTED) {                // รอจนกว่า WiFi จะเชื่อมต่อสำเร็จ
      delay(300);
      Serial.print(".");  // แสดงสถานะการเชื่อมต่อ
    }
    Serial.println("WiFi reconnected");  // แจ้งว่าการเชื่อมต่อ WiFi สำเร็จ
  }
}

void getMACAddress() {
  uint8_t mac[6];                                                                // ตัวแปรเก็บที่อยู่ MAC
  esp_wifi_get_mac(WIFI_IF_STA, mac);                                            // รับที่อยู่ MAC
  snprintf(macAddressMe, sizeof(macAddressMe), "%02X:%02X:%02X:%02X:%02X:%02X",  // แปลงที่อยู่ MAC เป็นรูปแบบสตริง
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("MAC Address: ");  // แสดงที่อยู่ MAC
  Serial.println(macAddressMe);
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {                                // ตรวจสอบการเชื่อมต่อ MQTT
    Serial.print("Attempting MQTT connection...");                 // แจ้งสถานะการเชื่อมต่อ MQTT
    if (mqttClient.connect(macAddressMe, mqtt_user, mqtt_pass)) {  // เชื่อมต่อกับเซิร์ฟเวอร์ MQTT
      Serial.println("connected");                                 // แจ้งว่าการเชื่อมต่อสำเร็จ
      mqttClient.subscribe(topicSubscribe);                        // สมัครรับข้อมูลจากหัวข้อที่ระบุ
    } else {
      Serial.print("failed, rc=");       // แจ้งว่าการเชื่อมต่อล้มเหลว
      Serial.print(mqttClient.state());  // แสดงสถานะการเชื่อมต่อ
      delay(300);                        // รอ 300 มิลลิวินาที
    }
  }
}

void parseMacAddress(const char* macAddressString, uint8_t* macAddressArray) {
  sscanf(macAddressString, "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x",  // แปลงที่อยู่ MAC จากสตริงเป็นอาร์เรย์
         &macAddressArray[0], &macAddressArray[1], &macAddressArray[2],
         &macAddressArray[3], &macAddressArray[4], &macAddressArray[5]);
}

void callbackMQTT(char* topicSubscribe, byte* payload, unsigned int length) {
  String message;                     // ตัวแปรสำหรับเก็บข้อความ
  for (int i = 0; i < length; i++) {  // รับข้อมูลจาก payload
    message += (char)payload[i];
  }
  Serial.println(message);                                     // แสดงข้อความที่รับ
  StaticJsonDocument<200> doc;                                 // สร้างเอกสาร JSON
  DeserializationError error = deserializeJson(doc, message);  // แปลงข้อความเป็น JSON
  if (error) {                                                 // ตรวจสอบข้อผิดพลาดในการแปลง JSON
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  const char* modeString = doc["mode"];                          // รับค่าของ mode
  const char* macSuperMasterAddressString = doc["supermaster"];  // รับที่อยู่ MAC ของ Super Master
  const char* macMasterAddressString = doc["master"];            // รับที่อยู่ MAC ของ Master
  const char* macSlaveAddressString = doc["slave"];              // รับที่อยู่ MAC ของ Slave

  parseMacAddress(macSuperMasterAddressString, receiveMQTTData.superMasterMacAddress);  // แปลงที่อยู่ MAC ของ Super Master
  parseMacAddress(macMasterAddressString, receiveMQTTData.masterMacAddress);            // แปลงที่อยู่ MAC ของ Master
  parseMacAddress(macSlaveAddressString, receiveMQTTData.slaveMacAddress);              // แปลงที่อยู่ MAC ของ Slave
  memcpy(&sendUARTData, &receiveMQTTData, sizeof(receiveMQTTData));                     // คัดลอกข้อมูลไปยังส่งผ่าน UART
  sendUARTDataFunction();                                                               // ส่งข้อมูลผ่าน UART
}

void sendMQTTDataFunction() {
  String mqttMessageSend;       // ตัวแปรสำหรับเก็บข้อความที่จะส่ง
  StaticJsonDocument<200> doc;  // สร้างเอกสาร JSON
  char macAddressString[18];    // ตัวแปรสำหรับเก็บที่อยู่ MAC ในรูปแบบสตริง
  float floatMessage;           // ตัวแปรสำหรับเก็บค่าลอยตัว

  snprintf(macAddressString, sizeof(macAddressString), "%02X:%02X:%02X:%02X:%02X:%02X",  // แปลงที่อยู่ MAC ของ Slave เป็นรูปแบบสตริง
           sendMQTTData.slaveMacAddress[0], sendMQTTData.slaveMacAddress[1],
           sendMQTTData.slaveMacAddress[2], sendMQTTData.slaveMacAddress[3],
           sendMQTTData.slaveMacAddress[4], sendMQTTData.slaveMacAddress[5]);

  if (memcmp(valueADCChar, sendMQTTData.stringMessage, 6) == 0) {  // ตรวจสอบค่าที่ส่งจาก ADC
    doc["result"] = sendMQTTData.stringMessage;                    // ถ้าเท่ากัน ส่งค่าที่เป็น string
  } else {
    floatMessage = atof(sendMQTTData.stringMessage);  // แปลงค่า string เป็น float
    doc["result"] = floatMessage;                     // ส่งค่าลอยตัว
  }
  doc["mac"] = macAddressString;                                    // เพิ่มที่อยู่ MAC ลงใน JSON
  serializeJson(doc, mqttMessageSend);                              // แปลง JSON เป็นสตริง
  Serial.printf("Slave MAC UART: %02X:%02X:%02X:%02X:%02X:%02X\n",  // แสดงที่อยู่ MAC ของ Slave
                sendMQTTData.slaveMacAddress[0], sendMQTTData.slaveMacAddress[1],
                sendMQTTData.slaveMacAddress[2], sendMQTTData.slaveMacAddress[3],
                sendMQTTData.slaveMacAddress[4], sendMQTTData.slaveMacAddress[5]);

  if (mqttClient.publish(topicPublish, mqttMessageSend.c_str())) {  // ส่งข้อมูลผ่าน MQTT
    Serial.println("Sent MQTT");                                    // แจ้งว่าการส่งข้อมูลสำเร็จ
  } else {
    Serial.println("MQTT Failed to send");  // แจ้งว่าการส่งข้อมูลล้มเหลว
  }
}

void sendUARTDataFunction() {
  strcpy(sendUARTData.stringMessage, "null");                                 // กำหนดค่า string เป็น "null"
  Serial.println("Send UART");                                                // แจ้งการส่งข้อมูลผ่าน UART
  uint8_t bufferUART[sizeof(sendUARTData)];                                   // สร้างบัฟเฟอร์สำหรับส่งข้อมูลผ่าน UART
  memcpy(bufferUART, &sendUARTData, sizeof(sendUARTData));                    // คัดลอกข้อมูลไปยังบัฟเฟอร์
  uart_write_bytes(UART_NUM_1, (const char*)bufferUART, sizeof(bufferUART));  // ส่งข้อมูลผ่าน UART
}

void callbackUART() {
  uint8_t bufferUART[sizeof(receiveUARTData)];                                                            // สร้างบัฟเฟอร์สำหรับรับข้อมูลผ่าน UART
  int lenUART = uart_read_bytes(UART_NUM_1, bufferUART, sizeof(receiveUARTData), 20 / portTICK_RATE_MS);  // อ่านข้อมูลจาก UART
  if (lenUART > 0) {                                                                                      // ตรวจสอบว่ามีข้อมูลที่รับเข้ามาหรือไม่
    memcpy(&receiveUARTData, bufferUART, sizeof(receiveUARTData));                                        // คัดลอกข้อมูลไปยังโครงสร้างรับ
    // แสดงผลข้อมูลที่ได้รับ
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receiveUARTData.superMasterMacAddress[0], receiveUARTData.superMasterMacAddress[1],
                  receiveUARTData.superMasterMacAddress[2], receiveUARTData.superMasterMacAddress[3],
                  receiveUARTData.superMasterMacAddress[4], receiveUARTData.superMasterMacAddress[5]);

    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receiveUARTData.masterMacAddress[0], receiveUARTData.masterMacAddress[1],
                  receiveUARTData.masterMacAddress[2], receiveUARTData.masterMacAddress[3],
                  receiveUARTData.masterMacAddress[4], receiveUARTData.masterMacAddress[5]);

    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receiveUARTData.slaveMacAddress[0], receiveUARTData.slaveMacAddress[1],
                  receiveUARTData.slaveMacAddress[2], receiveUARTData.slaveMacAddress[3],
                  receiveUARTData.slaveMacAddress[4], receiveUARTData.slaveMacAddress[5]);

    Serial.printf("String Message: %s\n", receiveUARTData.stringMessage);  // แสดงข้อความที่รับ
    memcpy(&sendMQTTData, &receiveUARTData, sizeof(receiveUARTData));      // คัดลอกข้อมูลไปยังส่งผ่าน MQTT
    sendMQTTDataFunction();                                                // ส่งข้อมูลผ่าน MQTT
  }
}

void loop() {
  reconnectWiFi();                // ตรวจสอบการเชื่อมต่อ WiFi
  if (!mqttClient.connected()) {  // ตรวจสอบการเชื่อมต่อ MQTT
    reconnectMQTT();              // เชื่อมต่อ MQTT ใหม่ถ้ายังไม่ได้เชื่อมต่อ
  }
  mqttClient.loop();  // ประมวลผลการเชื่อมต่อ MQTT
  callbackUART();     // ตรวจสอบการรับข้อมูลผ่าน UART
}