#include <esp_now.h>
#include <WiFi.h>

// MAC Address ของตัวแม่ที่รับได้
uint8_t mac_mother[6] = {0, 0, 0, 0, 0, 0};

// ข้อมูลที่จะส่งกลับไป
int response = 1;

// ฟังก์ชัน callback สำหรับรับข้อมูลจากตัวแม่
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  Serial.println("Data received from master");

  // ตรวจสอบว่าเป็น MAC Address (ข้อมูลมีขนาด 6 ไบต์)
  if (len == sizeof(mac_mother)) {
    // เก็บค่า MAC Address ของตัวแม่ที่ได้รับ
    memcpy(mac_mother, recv_info->src_addr, 6);

    // แสดง MAC Address ของตัวแม่
    Serial.print("Master MAC Address received: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X:", mac_mother[i]);
    }
    Serial.println();

    // เพิ่มตัวแม่เป็น peer (ต้องทำเพื่อส่งข้อมูลกลับ)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_mother, 6);  // กำหนด MAC Address ของตัวแม่
    peerInfo.channel = 0;  // ใช้ default channel
    peerInfo.encrypt = false;  // ไม่เข้ารหัส

    // ตรวจสอบการเพิ่ม peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      //Serial.println("Failed to add master as peer. Attempting to remove previous peer and retry.");
      // หากล้มเหลว ลองลบ peer ที่มีอยู่และลองใหม่
      esp_now_del_peer(mac_mother);
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add master as peer after retrying");
        return;
      }
    }

    // ส่งค่า 1 กลับไปยังตัวแม่
    esp_err_t result = esp_now_send(mac_mother, (uint8_t *)&response, sizeof(response));
    if (result == ESP_OK) {
      Serial.println("Send success");
    } else {
      Serial.println("Send failed");
    }
  }
}

// ฟังก์ชัน callback สำหรับการส่งข้อมูล
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // ลงทะเบียนฟังก์ชัน callback สำหรับส่งและรับข้อมูล
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // ไม่มีการทำงานใน loop
}
