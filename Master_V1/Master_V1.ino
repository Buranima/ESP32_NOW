#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>  // ไลบรารีสำหรับการใช้ esp_wifi_get_mac

uint8_t mac_master[] = {0xe6, 0x25, 0x6f, 0xb6, 0x14, 0x3c};

// MAC Address ของตัวลูก
uint8_t mac_lux[50][6] = {
  { 0xa0, 0xdd, 0x6c, 0x0f, 0xe2, 0x78 },  // ตัวลูก 1
  { 0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x74 },  // ตัวลูก 2
  { 0xcc, 0xdb, 0xa7, 0x32, 0xc6, 0xa4 },  // ตัวลูก 3
  { 0xcc, 0xdb, 0xa7, 0x32, 0xa6, 0x04 },  // ตัวลูก 4
  { 0x1a, 0xd0, 0x7c, 0xe4, 0x99, 0x52 },  // ตัวลูก 5
  { 0x7f, 0xc6, 0x18, 0x9d, 0x4e, 0xa1 },  // ตัวลูก 6
  { 0x83, 0xe7, 0x24, 0xf2, 0x5d, 0x3b },  // ตัวลูก 7
  { 0xa1, 0xb9, 0x67, 0x7c, 0x20, 0xc5 },  // ตัวลูก 8
  { 0x5a, 0xcc, 0xd1, 0x8b, 0xe0, 0x97 },  // ตัวลูก 9
  { 0xf4, 0x3d, 0x92, 0xb5, 0x71, 0xfa },  // ตัวลูก 10
  { 0x6b, 0x10, 0x4c, 0xae, 0x29, 0x2f },  // ตัวลูก 11
  { 0x38, 0x5e, 0xbf, 0x61, 0xe8, 0x90 },  // ตัวลูก 12
  { 0xce, 0x13, 0xad, 0x40, 0xd4, 0x73 },  // ตัวลูก 13
  { 0xf8, 0x34, 0x0b, 0x57, 0x9f, 0xa6 },  // ตัวลูก 14
  { 0x90, 0xd8, 0xf3, 0xc1, 0x03, 0x68 },  // ตัวลูก 15
  { 0x7d, 0x4a, 0x8c, 0x5b, 0xf0, 0x95 },  // ตัวลูก 16
  { 0xab, 0x36, 0x2f, 0xe3, 0xb8, 0x01 },  // ตัวลูก 17
  { 0xd7, 0xf6, 0x49, 0x74, 0x6a, 0x2c },  // ตัวลูก 18
  { 0x81, 0x09, 0x57, 0xcc, 0xef, 0xa3 },  // ตัวลูก 19
  { 0x2d, 0xb5, 0x1f, 0x6e, 0xc2, 0xda },  // ตัวลูก 20
  { 0x69, 0x24, 0xa8, 0x98, 0x03, 0x91 },  // ตัวลูก 21
  { 0xe5, 0x7a, 0x4f, 0x2c, 0xb1, 0xd6 },  // ตัวลูก 22
  { 0xb4, 0x63, 0xe7, 0xad, 0x89, 0x17 },  // ตัวลูก 23
  { 0x37, 0x92, 0x5e, 0xd4, 0xfa, 0x08 },  // ตัวลูก 24
  { 0x1c, 0x7b, 0x86, 0xf1, 0xb2, 0x4d },  // ตัวลูก 25
  { 0xfa, 0xce, 0x20, 0x98, 0x65, 0xb7 },  // ตัวลูก 26
  { 0x4b, 0xf5, 0x3d, 0xa9, 0xe2, 0x78 },  // ตัวลูก 27
  { 0xd2, 0x84, 0x71, 0x2f, 0x16, 0x93 },  // ตัวลูก 28
  { 0x6f, 0xb0, 0xc7, 0xde, 0x5c, 0x44 },  // ตัวลูก 29
  { 0xa2, 0x39, 0x58, 0x1c, 0x6b, 0xf3 },  // ตัวลูก 30
  { 0x8d, 0xa6, 0x41, 0x33, 0xe0, 0xbd },  // ตัวลูก 31
  { 0x13, 0x74, 0xf9, 0xcd, 0x50, 0x28 },  // ตัวลูก 32
  { 0x99, 0xc5, 0x1a, 0xb2, 0x84, 0x6d },  // ตัวลูก 33
  { 0x26, 0x68, 0xdb, 0x3e, 0xf7, 0xa1 },  // ตัวลูก 34
  { 0x9e, 0x17, 0x34, 0xe5, 0x0d, 0xcb },  // ตัวลูก 35
  { 0xef, 0x48, 0x90, 0x77, 0x5b, 0x2a },  // ตัวลูก 36
  { 0xb8, 0x06, 0xa4, 0xc9, 0x12, 0x83 },  // ตัวลูก 37
  { 0xc1, 0x2d, 0x67, 0xea, 0x59, 0xf5 },  // ตัวลูก 38
  { 0xa3, 0x70, 0x9b, 0x21, 0x04, 0x5f },  // ตัวลูก 39
  { 0xe8, 0xf3, 0x11, 0xb7, 0x36, 0xd5 },  // ตัวลูก 40
  { 0x62, 0x9d, 0xbc, 0x48, 0x7a, 0x09 },  // ตัวลูก 41
  { 0x44, 0x53, 0x82, 0xff, 0x29, 0xd2 },  // ตัวลูก 42
  { 0xf0, 0xaf, 0xc1, 0xe0, 0x3a, 0x67 },  // ตัวลูก 43
  { 0xd9, 0x16, 0x0e, 0xbc, 0xfa, 0x85 },  // ตัวลูก 44
  { 0x7b, 0x32, 0x94, 0xcd, 0x68, 0x01 },  // ตัวลูก 45
  { 0x2f, 0x91, 0xa3, 0x5d, 0x47, 0xfe },  // ตัวลูก 46
  { 0xb3, 0x4c, 0x60, 0x3f, 0xda, 0x86 },  // ตัวลูก 47
  { 0x5e, 0xad, 0xe1, 0x28, 0x9a, 0x4b },  // ตัวลูก 48
  { 0xc4, 0xbb, 0xf4, 0x53, 0x62, 0x12 },  // ตัวลูก 49
  { 0xe6, 0x25, 0x6f, 0xb6, 0x14, 0x3c }   // ตัวลูก 50
};

// ข้อมูลที่จะส่ง
typedef struct struct_message {
  uint8_t macAddr[6];  // ส่ง MAC Address ของตัวลูกไปด้วย
  float value;         // ค่าทศนิยมที่ได้รับจากตัวลูก
} struct_message;

struct_message myData;

// เก็บข้อมูลการตอบกลับ
struct response_data {
  uint8_t macAddr[6];
  float value;    // ค่าทศนิยม 1 ตำแหน่ง
  bool received;  // บ่งบอกว่ามีการตอบกลับหรือไม่
} responses[50];

// ฟังก์ชัน callback สำหรับการส่งข้อมูล
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("Last Packet Send Status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// ฟังก์ชัน callback สำหรับการรับข้อมูลจากตัวลูก
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  float receivedValue = *((float *)data);  // รับค่าทศนิยม 1 ตำแหน่ง

  // บันทึกการตอบกลับ
  for (int i = 0; i < 50; i++) {
    if (memcmp(recv_info->src_addr, mac_lux[i], 6) == 0) {
      memcpy(responses[i].macAddr, recv_info->src_addr, 6);
      responses[i].value = receivedValue;
      responses[i].received = true;

      // หลังจากรับข้อมูลแล้ว ส่งข้อมูลไปยัง ESP-NOW ตัวที่สอง (master ตัวใหม่)
      struct_message responseData;
      memcpy(responseData.macAddr, responses[i].macAddr, 6);  // ส่ง MAC Address ของลูกที่ตอบกลับ
      responseData.value = responses[i].value;  // ส่งค่าทศนิยมไปด้วย

      esp_err_t result = esp_now_send(mac_master, (uint8_t *)&responseData, sizeof(responseData));  // ส่งข้อมูลไปยังตัว master ใหม่
      if (result == ESP_OK) {
        Serial.println("Data sent to master");
      } else {
        Serial.println("Failed to send data to master");
      }
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    //Serial.println("Error initializing ESP-NOW");
    return;
  }

  // ลงทะเบียนฟังก์ชัน callback สำหรับการส่งและรับข้อมูล
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // เพิ่ม peer สำหรับแต่ละลูก
  for (int i = 0; i < 50; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_lux[i], 6);  // กำหนด MAC Address ของลูกแต่ละตัว
    peerInfo.channel = 0;                       // ใช้ default channel
    peerInfo.encrypt = false;                   // ไม่เข้ารหัส

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      //Serial.println("Failed to add peer");
      return;
    }

    // ตั้งค่าเริ่มต้นของการตอบกลับ
    memset(responses[i].macAddr, 0, sizeof(responses[i].macAddr));
    responses[i].value = 0.0;
    responses[i].received = false;
  }
}

void sendToSlave(int slaveIndex) {
  esp_wifi_get_mac(WIFI_IF_STA, myData.macAddr);
  int retries = 0;
  bool ackReceived = false;

  while (retries < 3 && !ackReceived) {
    esp_err_t result = esp_now_send(mac_lux[slaveIndex], (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.printf("Data sent to slave %d\n", slaveIndex + 1);
    } else {
      //Serial.println("Send failed");
    }

    // รอการตอบกลับ
    unsigned long waitStart = millis();
    while (millis() - waitStart < 30) {  // รอการตอบกลับเป็นเวลา 0.03 วินาที
      if (responses[slaveIndex].received) {
        ackReceived = true;
        break;
      }
    }

    retries++;
  }

  if (!ackReceived) {
    //Serial.printf("No response from slave %d after 3 attempts\n", slaveIndex + 1);
    responses[slaveIndex].received = false;  // บันทึกว่าไม่มีการตอบกลับ
  }
}

void loop() {
  for (int i = 0; i < 50; i++) {
    sendToSlave(i);  // ส่งข้อมูลไปยังแต่ละตัว
    // delay(500);  // รอเวลาระหว่างการส่ง
  }

  // แสดงผลลัพธ์
  Serial.println("\nResponses:");
  for (int i = 0; i < 50; i++) {
    if (responses[i].received) {
      Serial.printf("Slave %d: MAC: ", i + 1);
      for (int j = 0; j < 6; j++) {
        Serial.printf("%02X:", responses[i].macAddr[j]);
      }
      Serial.printf(" Value: %.1f\n", responses[i].value);
    } else {
      Serial.printf("Slave %d: No response (null)\n", i + 1);
    }
  }
  // เคลียร์ค่าของ responses หลังจากแสดงผล
  for (int i = 0; i < 50; i++) {
    memset(responses[i].macAddr, 0, sizeof(responses[i].macAddr));
    responses[i].value = 0.0;
    responses[i].received = false;
  }

  // หน่วงเวลาสำหรับ loop ต่อไป
  delay(3000);
}