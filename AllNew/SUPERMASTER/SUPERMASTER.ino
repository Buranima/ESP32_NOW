#include <Arduino.h>
#include <string.h>
#include <driver/uart.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// กำหนดขาพอร์ต UART
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)  // ขนาดบัฟเฟอร์สำหรับรับส่งข้อมูล UART

// โครงสร้างข้อมูลที่ใช้ส่งผ่าน ESP-NOW และ UART
typedef struct structMessage {
  uint8_t superMasterMAC[6];  // MAC Address ของ Super Master
  uint8_t masterMAC[6];       // MAC Address ของ Master
  uint8_t slaveMAC[6];        // MAC Address ของ Slave
  char adcMessage[16];        // ข้อความที่ใช้เก็บค่า ADC หรือข้อมูลอื่นๆ
} structMessage;
structMessage receivedMessage;  // ตัวแปรสำหรับเก็บข้อมูลที่ได้รับ

uint8_t noMasterMAC[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // MAC Address กรณีไม่มี Master

// ฟังก์ชั่น callbackUART() สำหรับรับข้อมูลจาก UART
void callbackUART() {
  uint8_t uartBuffer[sizeof(receivedMessage)];  // บัฟเฟอร์สำหรับเก็บข้อมูล UART
  // อ่านข้อมูลจาก UART1 และเก็บในบัฟเฟอร์
  int uartLength = uart_read_bytes(UART_NUM_1, uartBuffer, sizeof(receivedMessage), 20 / portTICK_RATE_MS);
  if (uartLength > 0) {
    memcpy(&receivedMessage, uartBuffer, sizeof(receivedMessage));  // คัดลอกข้อมูลไปยังโครงสร้าง structMessage

    // แสดงผลข้อมูลที่ได้รับจาก UART
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receivedMessage.superMasterMAC[0], receivedMessage.superMasterMAC[1],
                  receivedMessage.superMasterMAC[2], receivedMessage.superMasterMAC[3],
                  receivedMessage.superMasterMAC[4], receivedMessage.superMasterMAC[5]);

    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receivedMessage.masterMAC[0], receivedMessage.masterMAC[1],
                  receivedMessage.masterMAC[2], receivedMessage.masterMAC[3],
                  receivedMessage.masterMAC[4], receivedMessage.masterMAC[5]);

    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  receivedMessage.slaveMAC[0], receivedMessage.slaveMAC[1],
                  receivedMessage.slaveMAC[2], receivedMessage.slaveMAC[3],
                  receivedMessage.slaveMAC[4], receivedMessage.slaveMAC[5]);

    Serial.printf("String Message: %s\n", receivedMessage.adcMessage);

    sendESPNOWMessage();  // ส่งข้อมูลผ่าน ESP-NOW
  }
}

// ฟังก์ชั่น callbackESPNOW() สำหรับรับข้อมูลจาก ESP-NOW
void callbackESPNOW(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));  // คัดลอกข้อมูลที่ได้รับจาก ESP-NOW

  // แสดงผลข้อมูลที่ได้รับจาก ESP-NOW
  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.superMasterMAC[0], receivedMessage.superMasterMAC[1],
                receivedMessage.superMasterMAC[2], receivedMessage.superMasterMAC[3],
                receivedMessage.superMasterMAC[4], receivedMessage.superMasterMAC[5]);

  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.masterMAC[0], receivedMessage.masterMAC[1],
                receivedMessage.masterMAC[2], receivedMessage.masterMAC[3],
                receivedMessage.masterMAC[4], receivedMessage.masterMAC[5]);

  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.slaveMAC[0], receivedMessage.slaveMAC[1],
                receivedMessage.slaveMAC[2], receivedMessage.slaveMAC[3],
                receivedMessage.slaveMAC[4], receivedMessage.slaveMAC[5]);

  Serial.printf("String Message: %s\n", receivedMessage.adcMessage);

  sendUARTMessage();  // ส่งข้อมูลผ่าน UART
}

// ฟังก์ชั่น setup() สำหรับการตั้งค่าเริ่มต้น
void setup() {
  Serial.begin(115200);  // เริ่มต้น Serial Monitor
  WiFi.mode(WIFI_STA);   // ตั้งค่า WiFi เป็นโหมดสถานี (STA)
  setupUART();           // เรียกใช้ฟังก์ชั่น setupUART() เพื่อเซ็ตอัพ UART

  // ตรวจสอบการเริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // ลงทะเบียน callback สำหรับรับข้อมูลผ่าน ESP-NOW
  esp_now_register_recv_cb(callbackESPNOW);
}

// ฟังก์ชั่น setupUART() สำหรับการตั้งค่า UART
void setupUART() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,                    // กำหนด baud rate ที่ 115200
    .data_bits = UART_DATA_8_BITS,          // กำหนด data bits เป็น 8
    .parity = UART_PARITY_DISABLE,          // ปิดการใช้งาน parity bit
    .stop_bits = UART_STOP_BITS_1,          // ใช้ stop bit 1 บิต
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // ปิดการควบคุมการไหลของข้อมูล
  };

  uart_param_config(UART_NUM_1, &uart_config);                                                 // ตั้งค่าการสื่อสาร UART
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  // กำหนดขาพอร์ตสำหรับ TX และ RX
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);                                    // ติดตั้ง driver ของ UART พร้อมบัฟเฟอร์
}

// ฟังก์ชั่น sendESPNOWMessage() สำหรับส่งข้อมูลผ่าน ESP-NOW
void sendESPNOWMessage() {
  // ตรวจสอบว่ามี Master MAC Address หรือไม่
  if (memcmp(noMasterMAC, receivedMessage.masterMAC, 6) == 0) {
    // ไม่มี Master ให้สื่อสารกับ Slave โดยตรง
    if (esp_now_is_peer_exist(receivedMessage.slaveMAC)) {
      Serial.println("Peer already exists, no need to add.");
    } else {
      // เพิ่ม Peer ใหม่หากยังไม่มี
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));        // ตั้งค่า peerInfo เป็นศูนย์
      memcpy(peerInfo.peer_addr, receivedMessage.slaveMAC, 6);  // กำหนด MAC Address ของ Slave
      peerInfo.channel = 0;                                     // ตั้งค่าช่องสัญญาณ
      peerInfo.encrypt = false;                                 // ไม่ใช้การเข้ารหัส
      peerInfo.ifidx = WIFI_IF_STA;                             // ใช้โหมด STA สำหรับการสื่อสาร

      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
      } else {
        Serial.println("Peer added successfully.");
      }
    }

    // ส่งข้อมูลผ่าน ESP-NOW
    esp_err_t result = esp_now_send(receivedMessage.slaveMAC, (uint8_t*)&receivedMessage, sizeof(receivedMessage));
    if (result == ESP_OK) {
      Serial.println("Message sent successfully");
    } else {
      Serial.printf("Error sending message: %d\n", result);
    }
  } else {
    // กรณีมี Master ให้สื่อสารกับ Master ได้ที่นี่
    // เขียน logic ที่ต้องการสำหรับการสื่อสารกับ Master ที่นี่
  }
}

// ฟังก์ชั่น sendUARTMessage() สำหรับส่งข้อมูลผ่าน UART
void sendUARTMessage() {
  uint8_t uartBuffer[sizeof(receivedMessage)];
  memcpy(uartBuffer, &receivedMessage, sizeof(structMessage));                // คัดลอกข้อมูลไปยังบัฟเฟอร์ UART
  uart_write_bytes(UART_NUM_1, (const char*)uartBuffer, sizeof(uartBuffer));  // ส่งข้อมูลผ่าน UART
}

// ฟังก์ชั่น loop() สำหรับการทำงานหลักในแต่ละรอบ
void loop() {
  callbackUART();  // ตรวจสอบและรับข้อมูลจาก UART ในแต่ละรอบ loop
}