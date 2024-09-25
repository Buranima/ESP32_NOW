#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

// โครงสร้างสำหรับจัดเก็บข้อมูล MAC Address และข้อความ
typedef struct structMessage {
  uint8_t superMasterMac[6];  // MAC ของ Super Master
  uint8_t masterMac[6];       // MAC ของ Master
  uint8_t slaveMac[6];        // MAC ของ Slave
  char adcMessage[16];        // ข้อความจาก ADC
} structMessage;

structMessage receivedData;  // ตัวแปรสำหรับเก็บข้อมูลที่รับมา
structMessage sendingData;   // ตัวแปรสำหรับเก็บข้อมูลที่จะส่ง

char emptyMessage[] = "null";    // ข้อความเปล่า
int retryCount = 0;              // จำนวนครั้งที่ลองส่งซ้ำ
const int maxRetryCount = 3;     // กำหนดจำนวนครั้งสูงสุดในการส่งซ้ำ
String adcValueString;           // ตัวแปรเก็บข้อความค่า ADC ที่แปลงแล้ว
unsigned long lastSendTime = 0;  // ตัวแปรเก็บเวลาการส่งครั้งล่าสุด
const long sendInterval = 30;    // กำหนดช่วงเวลาการส่ง (มิลลิวินาที)

// ฟังก์ชัน callback เมื่อมีการรับข้อมูลผ่าน ESP-NOW
void onReceiveData(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  // คัดลอกข้อมูลที่รับมาเก็บไว้ใน receivedData
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  // แสดงข้อมูลที่รับมาใน Serial Monitor
  Serial.printf("Super Master MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedData.superMasterMac[0], receivedData.superMasterMac[1],
                receivedData.superMasterMac[2], receivedData.superMasterMac[3],
                receivedData.superMasterMac[4], receivedData.superMasterMac[5]);
  Serial.printf("Master MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedData.masterMac[0], receivedData.masterMac[1],
                receivedData.masterMac[2], receivedData.masterMac[3],
                receivedData.masterMac[4], receivedData.masterMac[5]);
  Serial.printf("Slave MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedData.slaveMac[0], receivedData.slaveMac[1],
                receivedData.slaveMac[2], receivedData.slaveMac[3],
                receivedData.slaveMac[4], receivedData.slaveMac[5]);
  Serial.printf("ข้อความที่ได้รับมา: %s\n", receivedData.adcMessage);

  // ส่งข้อมูลกลับไป
  sendData();
}

// ฟังก์ชันส่งข้อมูลผ่าน ESP-NOW
void sendData() {
  // คัดลอกข้อมูลที่รับมาจาก receivedData ไปที่ sendingData
  memcpy(&sendingData, &receivedData, sizeof(receivedData));

  // ตรวจสอบว่า message เป็น "null" หรือไม่
  if (memcmp(emptyMessage, sendingData.adcMessage, 16) == 0) {
    // ถ้ายังไม่มี peer ที่เป็น Slave ให้เพิ่ม peer ใหม่
    if (!esp_now_is_peer_exist(sendingData.slaveMac)) {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, sendingData.slaveMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;

      // เพิ่ม Peer
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
        return;
      } else {
        Serial.println("จับคู่ ESPNOW สำเร็จ");
      }
    } else {
      Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
    }

    retryCount = 0;  // รีเซ็ตจำนวนครั้งการลองส่งใหม่

    // ลองส่งข้อมูลใหม่
    while (retryCount < maxRetryCount) {
      unsigned long currentMillis = millis();
      if ((currentMillis - lastSendTime >= sendInterval) || (lastSendTime > currentMillis)) {
        lastSendTime = currentMillis;

        // ส่งข้อมูลไปยัง Slave
        esp_err_t result = esp_now_send(sendingData.slaveMac, (uint8_t*)&sendingData, sizeof(sendingData));
        if (result == ESP_OK) {
          Serial.println("อุปกรณ์นี้ส่ง ESPNOW ได้สำเร็จ");
        } else {
          Serial.printf("อุปกรณ์นี้ส่ง ESPNOW ไม่สำเร็จเนื่องจาก: %d\n", result);
        }
      }
      retryCount++;
    }

    // ถ้าส่งไม่สำเร็จตามจำนวนครั้งที่กำหนด ให้ส่งข้อมูลไปที่ Super Master
    if (retryCount >= maxRetryCount) {
      adcValueString = "null";
      strcpy(sendingData.adcMessage, adcValueString.c_str());

      // ตรวจสอบว่า peer ของ Super Master มีอยู่หรือไม่ ถ้าไม่มีก็เพิ่ม
      if (!esp_now_is_peer_exist(sendingData.superMasterMac)) {
        esp_now_peer_info_t peerInfo;
        memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
        memcpy(peerInfo.peer_addr, sendingData.superMasterMac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;

        // เพิ่ม Peer สำหรับ Super Master
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
          return;
        } else {
          Serial.println("จับคู่ ESPNOW สำเร็จ");
        }
      } else {
        Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
      }

      // ส่งข้อมูลไปยัง Super Master
      esp_err_t result = esp_now_send(sendingData.superMasterMac, (uint8_t*)&sendingData, sizeof(sendingData));
      if (result == ESP_OK) {
        Serial.println("อุปกรณ์นี้ส่ง ESPNOW ได้สำเร็จ");
      } else {
        Serial.printf("อุปกรณ์นี้ส่ง ESPNOW ไม่สำเร็จเนื่องจาก: %d\n", result);
      }
    }
  } else {
    // ส่งข้อมูลไปยัง Super Master หาก message ไม่เป็น "null"
    if (!esp_now_is_peer_exist(sendingData.superMasterMac)) {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, sendingData.superMasterMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;

      // เพิ่ม Peer สำหรับ Super Master
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
        return;
      } else {
        Serial.println("จับคู่ ESPNOW สำเร็จ");
      }
    } else {
      Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
    }

    // ส่งข้อมูลไปยัง Super Master
    esp_err_t result = esp_now_send(sendingData.superMasterMac, (uint8_t*)&sendingData, sizeof(sendingData));
    if (result == ESP_OK) {
      Serial.println("อุปกรณ์นี้ส่ง ESPNOW ได้สำเร็จ");
    } else {
      Serial.printf("อุปกรณ์นี้ส่ง ESPNOW ไม่สำเร็จเนื่องจาก: %d\n", result);
    }
  }
}

// ฟังก์ชัน callback เมื่อส่งข้อมูลผ่าน ESP-NOW
void onSendData(const uint8_t* macAddr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("อุปกรณ์นี้ส่ง ESPNOW ไปยังอุปกรณ์ที่จับคู่ได้สำเร็จ");
    retryCount = 0;  // รีเซ็ตจำนวนครั้งการส่ง
  } else {
    Serial.println("อุปกรณ์นี้ส่ง ESPNOW ไปยังอุปกรณ์ที่จับคู่ไม่สำเร็จ");
    retryCount++;  // เพิ่มจำนวนครั้งที่ล้มเหลว
  }
}

// ฟังก์ชันตั้งค่าเริ่มต้น
void setup() {
  Serial.begin(115200);  // เริ่มต้น Serial
  WiFi.mode(WIFI_STA);   // ตั้งค่า WiFi เป็นโหมดสถานี

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถใช้งาน ESP-NOW ได้");
    return;
  }

  esp_wifi_set_max_tx_power(84);            // ตั้งค่ากำลังส่งสูงสุดของ WiFi
  esp_now_register_recv_cb(onReceiveData);  // ลงทะเบียน callback สำหรับการรับข้อมูล
  esp_now_register_send_cb(onSendData);     // ลงทะเบียน callback สำหรับการส่งข้อมูล
}

void loop() {
  // ไม่มีการทำงานใน loop
}