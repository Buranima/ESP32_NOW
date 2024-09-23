#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// โครงสร้างข้อมูลที่จะใช้ส่งผ่าน ESP-NOW
typedef struct structMessage {
  uint8_t superMasterMAC[6];  // MAC Address ของ Super Master
  uint8_t masterMAC[6];       // MAC Address ของ Master
  uint8_t slaveMAC[6];        // MAC Address ของ Slave
  char adcValue[16];          // ข้อความที่เก็บค่าของ ADC
} structMessage;
structMessage receivedMessage;  // ตัวแปรสำหรับเก็บข้อมูลที่ได้รับผ่าน ESP-NOW

uint8_t noMasterMAC[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // MAC Address แบบไม่มี Master

// ฟังก์ชั่น callback ที่จะถูกเรียกใช้เมื่อได้รับข้อมูลผ่าน ESP-NOW
void onReceiveESPNOW(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  // คัดลอกข้อมูลที่ได้รับมาจาก incomingData ไปยังโครงสร้าง structMessage
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

  // แสดง MAC Address ของ Super Master
  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.superMasterMAC[0], receivedMessage.superMasterMAC[1],
                receivedMessage.superMasterMAC[2], receivedMessage.superMasterMAC[3],
                receivedMessage.superMasterMAC[4], receivedMessage.superMasterMAC[5]);

  // แสดง MAC Address ของ Master
  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.masterMAC[0], receivedMessage.masterMAC[1],
                receivedMessage.masterMAC[2], receivedMessage.masterMAC[3],
                receivedMessage.masterMAC[4], receivedMessage.masterMAC[5]);

  // แสดง MAC Address ของ Slave
  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.slaveMAC[0], receivedMessage.slaveMAC[1],
                receivedMessage.slaveMAC[2], receivedMessage.slaveMAC[3],
                receivedMessage.slaveMAC[4], receivedMessage.slaveMAC[5]);

  // แสดงค่าที่อ่านจาก ADC ซึ่งเก็บในรูปแบบ string
  Serial.printf("ADC Value: %s\n", receivedMessage.adcValue);

  // เรียกใช้ฟังก์ชั่นเพื่อส่งข้อความกลับผ่าน ESP-NOW
  sendESPNOWMessage();
}

// ฟังก์ชั่นสำหรับส่งข้อมูลผ่าน ESP-NOW
void sendESPNOWMessage() {
  updateADCValue();  // อ่านค่า ADC ใหม่ก่อนส่งข้อมูล

  // ตรวจสอบว่ามี Master MAC Address หรือไม่ (ถ้าไม่มีจะเป็น 0xff:0xff:0xff:0xff:0xff:0xff)
  if (memcmp(noMasterMAC, receivedMessage.masterMAC, 6) == 0) {
    // ไม่มี Master ให้สื่อสารกับ Super Master โดยตรง
    if (esp_now_is_peer_exist(receivedMessage.superMasterMAC)) {
      // ถ้า Peer (Super Master) มีอยู่แล้ว ไม่ต้องเพิ่มใหม่
      Serial.println("Peer already exists, no need to add.");
    } else {
      // ถ้า Peer ยังไม่มี ให้เพิ่มใหม่
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));              // ตั้งค่า peerInfo เป็นศูนย์
      memcpy(peerInfo.peer_addr, receivedMessage.superMasterMAC, 6);  // ใส่ MAC Address ของ Super Master
      peerInfo.channel = 0;                                           // กำหนดช่องสัญญาณสำหรับการสื่อสาร
      peerInfo.encrypt = false;                                       // ไม่ใช้การเข้ารหัสในการสื่อสาร
      peerInfo.ifidx = WIFI_IF_STA;                                   // ใช้การสื่อสารผ่านโหมดสถานี (STA)

      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        // หากไม่สามารถเพิ่ม Peer ได้
        Serial.println("Failed to add peer");
        return;
      } else {
        Serial.println("Peer added successfully.");
      }
    }

    // ส่งข้อมูลไปยัง Super Master
    esp_err_t result = esp_now_send(receivedMessage.superMasterMAC, (uint8_t*)&receivedMessage, sizeof(receivedMessage));
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

// ฟังก์ชั่นสำหรับการอ่านค่า ADC
void updateADCValue() {
  int adcIntValue = analogRead(34);                          // อ่านค่าจากพอร์ต ADC หมายเลข 34
  float adcFloatValue = (adcIntValue / 4095.0) * 3.3;        // แปลงค่า ADC เป็นโวลต์
  String adcStringValue = String(adcFloatValue, 2);          // แปลงค่า ADC เป็นสตริงที่มีทศนิยม 2 ตำแหน่ง
  strcpy(receivedMessage.adcValue, adcStringValue.c_str());  // คัดลอกค่า ADC ไปเก็บในโครงสร้าง structMessage
}

// ฟังก์ชั่น setup() จะถูกรันครั้งเดียวเมื่อเริ่มทำงาน
void setup() {
  Serial.begin(115200);  // เริ่มต้น Serial Monitor ที่ baud rate 115200
  WiFi.mode(WIFI_STA);   // ตั้งค่า WiFi เป็นโหมดสถานี (STA)

  // ตรวจสอบการเริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }

  // ลงทะเบียน callback สำหรับรับข้อมูล ESP-NOW
  esp_now_register_recv_cb(onReceiveESPNOW);
}

// ฟังก์ชั่น loop() จะถูกรันซ้ำๆ
void loop() {
  // โค้ดที่ทำงานภายใน loop
}