#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <driver/uart.h>
#include <esp_wifi.h>
#include <string.h>

// กำหนดพอร์ต UART
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)
#define UART_TIMEOUT_MS 1000

// กำหนดขา LED
// const int led_PIN_1 = 4;  // GPIO 32
// const int led_PIN_2 = 5;  // GPIO 33

typedef struct struct_message {
  uint8_t dataESPNOW[250];
} struct_message;
struct_message dataToSendESPNOW;

bool responseReceived = false;
uint8_t uartBuffer[BUF_SIZE];
char clientId[25];
char macSlaveUrat[18];
int retryCount = 0;
String jsonStringRecv;

// ฟังก์ชั่น callback สำหรับการรับข้อมูลผ่าน ESP-NOW
void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  responseReceived = true;
  Serial.println("ได้รับการตอบกลับจาก Slave แล้ว");

  // แปลง byte array เป็นสตริง
  jsonStringRecv = String((char *)incomingData);
}

// ฟังก์ชั่นการเชื่อมต่อ ESP-NOW
void setupESPNOW() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_wifi_set_max_tx_power(84);
  esp_now_register_recv_cb(onDataRecv);  // ลงทะเบียน callback สำหรับรับข้อมูล
}

// ฟังก์ชั่นสำหรับรับข้อมูลจาก UART และส่งต่อด้วย ESP-NOW
void readUARTAndSendESPNOW() {
  int len = uart_read_bytes(UART_NUM_1, uartBuffer, BUF_SIZE, UART_TIMEOUT_MS / portTICK_RATE_MS);

  if (len > 0) {
    Serial.println("รับข้อมูลจาก Uart");

    // แปลงข้อมูลที่รับจาก UART เป็น String
    String uartMessage = String((char *)uartBuffer);
    Serial.print("ข้อมูลที่รับมาจาก Uart: ");
    Serial.println(uartMessage);

    // สร้าง JSON document ขนาดที่เหมาะสมกับข้อมูลที่เราจะ parse
    StaticJsonDocument<1024> docUart;

    // Parse ข้อความ JSON จาก MQTT payload
    DeserializationError error = deserializeJson(docUart, uartMessage);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // เข้าถึง JsonArray
    JsonArray macSlaves = docUart["mac-slave"].as<JsonArray>();

    for (JsonVariant value : macSlaves) {
      Serial.println("");
      Serial.println(value.as<String>());

      // สร้าง JSON
      StaticJsonDocument<256> docESPNOW;
      docESPNOW["mac-uart"] = clientId;
      docESPNOW["message"] = "Getdata";

      // แปลง JSON เป็นสตริง
      String jsonString;
      serializeJson(docESPNOW, jsonString);

      // แปลงสตริงเป็น byte array
      int lenJsonString = jsonString.length() + 1;
      memcpy(dataToSendESPNOW.dataESPNOW, jsonString.c_str(), lenJsonString);

      uint8_t macAddressArray[6];

      for (int i = 0; i < 6; i++) {
        String byteString = value.as<String>().substring(i * 2, (i * 2) + 2);
        macAddressArray[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
      }

      Serial.print("MAC Address: ");
      for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", macAddressArray[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.println("");

      retryCount = 0;
      responseReceived = false;

      while (retryCount < 3 && !responseReceived) {
        Serial.printf("ส่งข้อมูลด้วย ESP-NOW ไปยัง Slave ครั้งที่: %d\n", retryCount + 1);

        // ตรวจสอบว่ามี Peer ที่เป็น Uart อยู่แล้วหรือไม่
        if (esp_now_is_peer_exist(macAddressArray)) {
          Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
        } else {

          // ถ้าไม่มี Peer ให้ทำการเพิ่ม Uart เป็น Peer
          esp_now_peer_info_t peerInfo;
          memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
          memcpy(peerInfo.peer_addr, macAddressArray, 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
          peerInfo.ifidx = WIFI_IF_STA;

          // เพิ่ม Peer ให้กับ ESP-NOW
          if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
            return;
          } else {
            Serial.println("จับคู่ ESPNOW สำเร็จ");
          }
        }

        // ส่งข้อมูลด้วย ESP-NOW ไปยัง Slave
        esp_err_t result = esp_now_send(macAddressArray, (uint8_t *)&dataToSendESPNOW, sizeof(dataToSendESPNOW));
        if (result == ESP_OK) {
          Serial.println("ส่งข้อมูลด้วย ESP-NOW ไปยัง Slave สำเร็จ");
        } else {
          Serial.println("ส่งข้อมูลด้วย ESP-NOW ไปยัง Slave ไม่สำเร็จ");
        }
        Serial.println("ข้อมูลที่ส่งไปยัง Slave: " + jsonString);
        String byteString = value.as<String>();
        memcpy(macSlaveUrat, byteString.c_str(), byteString.length());
        delay(200);
        retryCount++;
      }

      sendToUart();
    }
  }
}

void sendToUart() {
  // สร้าง JSON document
  StaticJsonDocument<256> docResult;

  if (responseReceived) {
    Serial.println(jsonStringRecv);
    DeserializationError error = deserializeJson(docResult, jsonStringRecv);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    Serial.println("MAC SLAVE: " + String(docResult["mac-slave"]));
    Serial.println("Result: " + String(docResult["result"]));
  } else {
    docResult["mac-slave"] = macSlaveUrat;
    Serial.println("MAC SLAVE: " + String(docResult["mac-slave"]));
    docResult["result"] = "null";
    Serial.println("Result: " + String(docResult["result"]));
  }

  // แปลง JSON document ใหม่เป็นสตริง
  String jsonString;
  serializeJson(docResult, jsonString);  // แปลง JSON document เป็นสตริง

  // ส่งข้อมูลผ่าน UART
  Serial.println("ข้อมูลที่ส่งไปยัง Uart: " + jsonString);
  uart_write_bytes(UART_NUM_1, jsonString.c_str(), jsonString.length());
  Serial.println("");
}

// ฟังก์ชั่น setupUART สำหรับตั้งค่า UART
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

void setup() {
  Serial.begin(115200);

  // ตั้งค่า UART
  setupUART();
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  // ดึง MAC Address ของ ESP32 และเก็บในตัวแปร clientId
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(clientId, 25, "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("Client ID: ");
  Serial.println(clientId);  // แสดง MAC Address ที่ใช้เป็น clientId

  // ตั้งค่า ESP-NOW
  setupESPNOW();

  // ตั้งค่าขา GPIO ให้เป็น OUTPUT เพื่อควบคุม LED
  // pinMode(led_PIN_1, OUTPUT);
  // pinMode(led_PIN_2, OUTPUT);

  // เปิดไฟ LED
  // digitalWrite(led_PIN_1, HIGH);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
  // digitalWrite(led_PIN_2, HIGH);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
}

void loop() {
  // อ่านข้อมูลจาก UART และส่งไปยัง ESP-NOW
  readUARTAndSendESPNOW();
}
