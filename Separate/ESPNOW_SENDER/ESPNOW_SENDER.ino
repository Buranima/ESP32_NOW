#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// โครงสร้างข้อมูลสำหรับส่งข้อมูล
typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];
  uint8_t masterMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[8];
} structMessageSend;

structMessageSend myDataMessageSend;

// กำหนด MAC Address ของตัวรับ (ESP32 ที่จะรับข้อมูล)
uint8_t receiverMacAddress[] = { 0xcc, 0xdb, 0xa7, 0x32, 0xb1, 0x54 };  // ใส่ MAC address ของตัวรับที่นี่

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // ตั้ง WiFi ให้ทำงานในโหมด STA (Station)

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  // เพิ่ม peer ที่เราจะส่งข้อมูลไป
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // กำหนดค่าในโครงสร้างข้อมูลที่เราจะส่ง
  uint8_t mac1[6] = { 0xcc, 0xdb, 0xa7, 0x32, 0xa6, 0x04 };
  uint8_t mac2[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t mac3[6] = { 0xcc, 0xdb, 0xa7, 0x32, 0xb1, 0x54 };
  memcpy(myDataMessageSend.superMasterMacAddress, mac1, 6);  // Super master MAC
  memcpy(myDataMessageSend.masterMacAddress, mac2, 6);       // Master MAC
  memcpy(myDataMessageSend.slaveMacAddress, mac3, 6);        // Slave MAC
  strcpy(myDataMessageSend.stringMessage, "null");

  // ส่งข้อมูลทุก ๆ 5 วินาที
  esp_now_send(receiverMacAddress, (uint8_t *)&myDataMessageSend, sizeof(myDataMessageSend));
  Serial.println("Message sent");
}

void loop() {
  delay(5000);

  // ส่งข้อมูลอีกครั้งใน loop
  esp_now_send(receiverMacAddress, (uint8_t *)&myDataMessageSend, sizeof(myDataMessageSend));
  Serial.println("Message sent again");
}