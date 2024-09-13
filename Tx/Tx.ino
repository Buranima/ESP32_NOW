#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>  // ไลบรารีสำหรับการใช้ esp_wifi_get_mac

// MAC Address ของตัวลูก
uint8_t mac_lux[2][6] = {
  { 0xa0, 0xdd, 0x6c, 0x0f, 0xe2, 0x78 },  // ตัวลูก 1
  { 0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x74 }   // ตัวลูก 2
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 3
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 4
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 5
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 6
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 6
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 7
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 8
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 9
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 10
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 11
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 12
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 13
  //{ 0x, 0x, 0x, 0x, 0x, 0x }   // ตัวลูก 14

};

// ข้อมูลที่จะส่ง
typedef struct struct_message {
  uint8_t macAddr[6];  // ส่ง MAC Address ของตัวแม่ไปให้ตัวลูก
} struct_message;

struct_message myData;

unsigned long previousMillis = 0;  // ตัวแปรสำหรับจับเวลา
const long interval = 100;         // ตั้งเวลาเป็น 2 วินาที (2000 มิลลิวินาที)

// ฟังก์ชัน callback สำหรับการส่งข้อมูล
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ฟังก์ชัน callback สำหรับการรับข้อมูลจากตัวลูก
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  Serial.print("Received data from MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X:", recv_info->src_addr[i]);
  }
  Serial.print(" with data: ");
  Serial.println(*data);  // แสดงค่าที่รับจากลูก
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // ลงทะเบียนฟังก์ชัน callback สำหรับการส่งและรับข้อมูล
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // เพิ่ม peer สำหรับแต่ละลูก
  for (int i = 0; i < 2; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_lux[i], 6);  // กำหนด MAC Address ของลูกแต่ละตัว
    peerInfo.channel = 0;                       // ใช้ default channel
    peerInfo.encrypt = false;                   // ไม่เข้ารหัส

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // หาก currentMillis น้อยกว่า previousMillis (เพราะถูกรีเซ็ต)
  if (currentMillis < previousMillis) {
    // ตั้งค่า previousMillis ใหม่เพื่อจัดการกับการรีเซ็ต
    previousMillis = currentMillis;
  }
  // ตรวจสอบว่า 2 วินาทีผ่านไปหรือยัง
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // อัปเดตเวลาที่บันทึกไว้


    // ส่ง MAC Address ของตัวแม่ไปให้ตัวลูกทุก 2 วินาที
    esp_wifi_get_mac(WIFI_IF_STA, myData.macAddr);
    for (int i = 0; i < 2; i++) {
      esp_err_t result = esp_now_send(mac_lux[i], (uint8_t *)&myData, sizeof(myData));
      if (result == ESP_OK) {
        Serial.println("Data sent to slave");
      } else {
        Serial.println("Send failed");
      }
      delayMicroseconds(30);
    }
  }
}
