#include <esp_now.h>
#include <WiFi.h>

// Struct ข้อมูลที่จะส่ง
typedef struct struct_message {
  char a[32];
  int b;
  float c;
} struct_message;

// ตัวแปรสำหรับเก็บข้อมูลที่จะส่ง
struct_message myData;

// กำหนด MAC Address ของ Receiver
uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

// ฟังก์ชันสำหรับ callback เมื่อส่งข้อมูลเสร็จ
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ส่งข้อมูลไปที่: ");
  Serial.print(mac_addr[0], HEX); Serial.print(":");
  Serial.print(mac_addr[1], HEX); Serial.print(":");
  Serial.print(mac_addr[2], HEX); Serial.print(":");
  Serial.print(mac_addr[3], HEX); Serial.print(":");
  Serial.print(mac_addr[4], HEX); Serial.print(":");
  Serial.print(mac_addr[5], HEX);
  Serial.print(" ส่ง: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "สำเร็จ" : "ล้มเหลว");
}

void setup() {
  // เริ่มต้น Serial Monitor
  Serial.begin(115200);

  // เริ่มต้น Wi-Fi
  WiFi.mode(WIFI_STA);

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }

  // กำหนด callback เมื่อส่งข้อมูล
  esp_now_register_send_cb(OnDataSent);

  // เพิ่ม peer (Receiver)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ไม่สามารถเพิ่ม peer ได้");
    return;
  }

  // ข้อมูลที่จะส่ง
  strcpy(myData.a, "Hello Receiver");
  myData.b = 123;
  myData.c = 456.78;
}

void loop() {
  // ส่งข้อมูล
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("ส่งข้อมูลสำเร็จ");
  } else {
    Serial.println("ส่งข้อมูลล้มเหลว");
  }

  // รอ 2 วินาทีก่อนส่งครั้งต่อไป
  delay(2000);
}