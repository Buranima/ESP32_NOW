#include <esp_now.h>
#include <WiFi.h>

// Struct ข้อมูลที่รับ
typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];
  uint8_t masterMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[8];
} structMessageSend;

// ตัวแปรสำหรับเก็บข้อมูลที่รับ
structMessageSend myData;

// ฟังก์ชันสำหรับ callback เมื่อรับข้อมูล
void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  // Serial.print("รับจาก: ");
  // Serial.print(info->src.addr[0], HEX); Serial.print(":");
  // Serial.print(info->src.addr[1], HEX); Serial.print(":");
  // Serial.print(info->src.addr[2], HEX); Serial.print(":");
  // Serial.print(info->src.addr[3], HEX); Serial.print(":");
  // Serial.print(info->src.addr[4], HEX); Serial.print(":");
  // Serial.print(info->src.addr[5], HEX);
  // Serial.println();

  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
    myData.superMasterMacAddress[0], myData.superMasterMacAddress[1], 
    myData.superMasterMacAddress[2], myData.superMasterMacAddress[3], 
    myData.superMasterMacAddress[4], myData.superMasterMacAddress[5]);
  
  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
    myData.masterMacAddress[0], myData.masterMacAddress[1], 
    myData.masterMacAddress[2], myData.masterMacAddress[3], 
    myData.masterMacAddress[4], myData.masterMacAddress[5]);
  
  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
    myData.slaveMacAddress[0], myData.slaveMacAddress[1], 
    myData.slaveMacAddress[2], myData.slaveMacAddress[3], 
    myData.slaveMacAddress[4], myData.slaveMacAddress[5]);
  
  Serial.printf("ข้อความ: %s\n", myData.stringMessage);
}

void setup() {
  // เริ่มต้น Serial Monitor
  Serial.begin(115200);

  // เริ่มต้น Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }

  // กำหนด callback เมื่อรับข้อมูล
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // ไม่ต้องทำอะไรใน loop
}