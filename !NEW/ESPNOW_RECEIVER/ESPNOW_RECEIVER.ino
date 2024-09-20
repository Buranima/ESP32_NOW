#include <esp_now.h>
#include <WiFi.h>

// Struct ข้อมูลที่รับ
typedef struct struct_message {
  char a[32];
  int b;
  float c;
} struct_message;

// ตัวแปรสำหรับเก็บข้อมูลที่รับ
struct_message myData;

// ฟังก์ชันสำหรับ callback เมื่อรับข้อมูล
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("รับจาก: ");
  Serial.print(mac[0], HEX); Serial.print(":");
  Serial.print(mac[1], HEX); Serial.print(":");
  Serial.print(mac[2], HEX); Serial.print(":");
  Serial.print(mac[3], HEX); Serial.print(":");
  Serial.print(mac[4], HEX); Serial.print(":");
  Serial.print(mac[5], HEX);
  Serial.println();
  
  Serial.printf("ข้อความ: %s\n", myData.a);
  Serial.printf("ตัวเลข: %d\n", myData.b);
  Serial.printf("ค่า float: %.2f\n", myData.c);
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