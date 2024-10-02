#include <Arduino.h>
#include <string.h>
#include <driver/uart.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// กำหนดพินสำหรับ UART
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)

// โครงสร้างข้อมูลสำหรับส่งข้อความระหว่าง ESP-NOW และ UART
typedef struct Message {
  uint8_t superMasterMacAddress[6];  // เก็บ MAC Address ของ Super Master
  uint8_t masterMacAddress[6];       // เก็บ MAC Address ของ Master
  uint8_t slaveMacAddress[6];        // เก็บ MAC Address ของ Slave
  char stringMessage[16];            // เก็บข้อความที่ต้องการส่ง
} Message;

// ประกาศตัวแปรสำหรับเก็บข้อมูลที่รับและส่งผ่าน ESP-NOW และ UART
Message dataFromESPNOW;
Message dataFromUART;
Message dataToSendESPNOW;
Message dataToSendUART;

// MAC Address ที่ใช้แทนว่าไม่มี Master
uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int retries = 0;                   // ตัวนับการลองส่งซ้ำ
const int maxRetries = 3;          // จำนวนครั้งสูงสุดที่ลองส่งซ้ำ
String adcValueString;             // เก็บค่า ADC ในรูปแบบ String
unsigned long previousMillis = 0;  // ใช้สำหรับการจับเวลา
const long interval = 30;          // ระยะเวลาในแต่ละรอบการส่งข้อมูล

// ฟังก์ชัน callback สำหรับการรับข้อมูลจาก UART
void receiveUARTData() {
  uint8_t uartBuffer[sizeof(dataFromUART)];
  int uartLength = uart_read_bytes(UART_NUM_1, uartBuffer, sizeof(dataFromUART), 20 / portTICK_RATE_MS);

  // หากมีข้อมูลเข้ามา
  if (uartLength > 0) {
    memcpy(&dataFromUART, uartBuffer, sizeof(dataFromUART));  // คัดลอกข้อมูลที่รับมาไปยัง dataFromUART

    // แสดงข้อมูลที่รับจาก UART ผ่าน Serial Monitor
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataFromUART.superMasterMacAddress[0], dataFromUART.superMasterMacAddress[1],
                  dataFromUART.superMasterMacAddress[2], dataFromUART.superMasterMacAddress[3],
                  dataFromUART.superMasterMacAddress[4], dataFromUART.superMasterMacAddress[5]);
    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataFromUART.masterMacAddress[0], dataFromUART.masterMacAddress[1],
                  dataFromUART.masterMacAddress[2], dataFromUART.masterMacAddress[3],
                  dataFromUART.masterMacAddress[4], dataFromUART.masterMacAddress[5]);
    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  dataFromUART.slaveMacAddress[0], dataFromUART.slaveMacAddress[1],
                  dataFromUART.slaveMacAddress[2], dataFromUART.slaveMacAddress[3],
                  dataFromUART.slaveMacAddress[4], dataFromUART.slaveMacAddress[5]);
    Serial.printf("ข้อความ: %s\n", dataFromUART.stringMessage);

    // ส่งข้อมูลผ่าน ESP-NOW
    sendESPNOWMessage();
  }
}

// ฟังก์ชัน callback สำหรับการรับข้อมูลจาก ESP-NOW
void receiveESPNOWData(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  memcpy(&dataFromESPNOW, incomingData, sizeof(dataFromESPNOW));  // คัดลอกข้อมูลที่รับมาไปยัง dataFromESPNOW

  // แสดงข้อมูลที่รับจาก ESP-NOW ผ่าน Serial Monitor
  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                dataFromESPNOW.superMasterMacAddress[0], dataFromESPNOW.superMasterMacAddress[1],
                dataFromESPNOW.superMasterMacAddress[2], dataFromESPNOW.superMasterMacAddress[3],
                dataFromESPNOW.superMasterMacAddress[4], dataFromESPNOW.superMasterMacAddress[5]);
  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                dataFromESPNOW.masterMacAddress[0], dataFromESPNOW.masterMacAddress[1],
                dataFromESPNOW.masterMacAddress[2], dataFromESPNOW.masterMacAddress[3],
                dataFromESPNOW.masterMacAddress[4], dataFromESPNOW.masterMacAddress[5]);
  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                dataFromESPNOW.slaveMacAddress[0], dataFromESPNOW.slaveMacAddress[1],
                dataFromESPNOW.slaveMacAddress[2], dataFromESPNOW.slaveMacAddress[3],
                dataFromESPNOW.slaveMacAddress[4], dataFromESPNOW.slaveMacAddress[5]);
  Serial.printf("ข้อความ: %s\n", dataFromESPNOW.stringMessage);

  // คัดลอกข้อมูลเพื่อเตรียมส่งผ่าน UART
  memcpy(&dataToSendUART, &dataFromESPNOW, sizeof(dataFromESPNOW));
  sendUARTMessage();
}

// ฟังก์ชันสำหรับตั้งค่า UART
void setupUART() {
  const uart_config_t uartConfig = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  uart_param_config(UART_NUM_1, &uartConfig);
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);
}

// ฟังก์ชันสำหรับส่งข้อมูลผ่าน ESP-NOW
void sendESPNOWMessage() {
  memcpy(&dataToSendESPNOW, &dataFromUART, sizeof(dataFromUART));  // คัดลอกข้อมูลเพื่อเตรียมส่งผ่าน ESP-NOW

  // ตรวจสอบว่า Master มีอยู่หรือไม่
  if (memcmp(noMaster, dataToSendESPNOW.masterMacAddress, 6) == 0) {
    if (esp_now_is_peer_exist(dataToSendESPNOW.slaveMacAddress)) {
      Serial.println("Peer นี้มีอยู่แล้ว");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, dataToSendESPNOW.slaveMacAddress, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ไม่สามารถเพิ่ม Peer ได้");
        return;
      } else {
        Serial.println("เพิ่ม Peer สำเร็จ");
      }
    }
    retries = 0;
    while (retries < maxRetries) {
      unsigned long currentMillis = millis();
      if ((currentMillis - previousMillis >= interval) || (previousMillis > currentMillis)) {
        previousMillis = currentMillis;
        esp_err_t result = esp_now_send(dataToSendESPNOW.slaveMacAddress, (uint8_t*)&dataToSendESPNOW, sizeof(dataToSendESPNOW));
        if (result == ESP_OK) {
          Serial.println("ส่งข้อความสำเร็จ");
        } else {
          Serial.printf("ข้อผิดพลาดในการส่งข้อความ: %d\n", result);
        }
      }
    }
    retries++;
    if (retries > maxRetries) {
      adcValueString = "null";
      strcpy(dataToSendESPNOW.stringMessage, adcValueString.c_str());
      memcpy(&dataToSendUART, &dataToSendESPNOW, sizeof(dataToSendESPNOW));
      sendUARTMessage();
    }
  } else {
    if (esp_now_is_peer_exist(dataToSendESPNOW.masterMacAddress)) {
      Serial.println("Peer นี้มีอยู่แล้ว");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, dataToSendESPNOW.masterMacAddress, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ไม่สามารถเพิ่ม Peer ได้");
        return;
      } else {
        Serial.println("เพิ่ม Peer สำเร็จ");
      }
    }
    retries = 0;
    while (retries < maxRetries) {
      unsigned long currentMillis = millis();
      if ((currentMillis - previousMillis >= interval) || (previousMillis > currentMillis)) {
        previousMillis = currentMillis;
        esp_err_t result = esp_now_send(dataToSendESPNOW.masterMacAddress, (uint8_t*)&dataToSendESPNOW, sizeof(dataToSendESPNOW));
        if (result == ESP_OK) {
          Serial.println("ส่งข้อความสำเร็จ");
        } else {
          Serial.printf("ข้อผิดพลาดในการส่งข้อความ: %d\n", result);
        }
      }
    }
    retries++;
    if (retries > maxRetries) {
      adcValueString = "null";
      strcpy(dataToSendESPNOW.stringMessage, adcValueString.c_str());
      memcpy(&dataToSendUART, &dataToSendESPNOW, sizeof(dataToSendESPNOW));
      sendUARTMessage();
    }
  }
}

// ฟังก์ชันสำหรับส่งข้อมูลผ่าน UART
void sendUARTMessage() {
  uint8_t uartBuffer[sizeof(dataToSendUART)];
  memcpy(uartBuffer, &dataToSendUART, sizeof(dataToSendUART));
  uart_write_bytes(UART_NUM_1, (const char*)uartBuffer, sizeof(uartBuffer));
}

// ฟังก์ชัน callback เมื่อส่ง ESP-NOW สำเร็จหรือล้มเหลว
void callbackSendESPNOW(const uint8_t* macAddresscallbackSendESPNOW, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ส่งข้อมูลสำเร็จ!");
    retries = 0;
  } else {
    Serial.println("การส่งข้อมูลล้มเหลว!");
    retries++;
  }
}

void setup() {
  Serial.begin(115200);  // เริ่มต้นการสื่อสารผ่าน Serial Monitor
  WiFi.mode(WIFI_STA);   // ตั้งค่าโหมด Wi-Fi เป็นโหมดสถานี
  setupUART();           // เรียกใช้ฟังก์ชันตั้งค่า UART

  // ตรวจสอบว่าการเริ่มต้น ESP-NOW สำเร็จหรือไม่
  if (esp_now_init() != ESP_OK) {
    Serial.println("การเริ่มต้น ESP-NOW ล้มเหลว");
    return;
  }

  esp_wifi_set_max_tx_power(84);                 // กำหนดพลังส่งข้อมูล Wi-Fi
  esp_now_register_recv_cb(receiveESPNOWData);   // ลงทะเบียนฟังก์ชัน callback สำหรับรับข้อมูลจาก ESP-NOW
  esp_now_register_send_cb(callbackSendESPNOW);  // ลงทะเบียนฟังก์ชัน callback สำหรับส่งข้อมูล ESP-NOW
}

void loop() {
  receiveUARTData();  // เรียกใช้ฟังก์ชันรับข้อมูลจาก UART ใน loop
}