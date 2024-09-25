#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

// โครงสร้างข้อมูลสำหรับเก็บข้อมูล MAC และข้อความที่จะส่ง
typedef struct structMessage {
  uint8_t superMasterMac[6];  // MAC ของ Super Master
  uint8_t masterMac[6];       // MAC ของ Master
  uint8_t slaveMac[6];        // MAC ของ Slave
  char adcMessage[16];        // ข้อความที่ใช้ส่ง ADC ค่า
} structMessage;

structMessage receivedMessage;  // ตัวแปรสำหรับเก็บข้อมูลที่ได้รับ
structMessage sendMessage;      // ตัวแปรสำหรับเก็บข้อมูลที่ต้องการส่ง

uint8_t noMasterMAC[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // MAC ไม่มี Master

// ฟังก์ชัน callback เมื่อตัว ESP32 ได้รับข้อมูลผ่าน ESP-NOW
void onReceiveESPNOW(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  // คัดลอกข้อมูลที่รับมาเก็บในตัวแปร receivedMessage
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

  // แสดงผลข้อมูล MAC ที่รับมา
  Serial.printf("Super Master MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.superMasterMac[0], receivedMessage.superMasterMac[1],
                receivedMessage.superMasterMac[2], receivedMessage.superMasterMac[3],
                receivedMessage.superMasterMac[4], receivedMessage.superMasterMac[5]);
  Serial.printf("Master MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.masterMac[0], receivedMessage.masterMac[1],
                receivedMessage.masterMac[2], receivedMessage.masterMac[3],
                receivedMessage.masterMac[4], receivedMessage.masterMac[5]);
  Serial.printf("Slave MACADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receivedMessage.slaveMac[0], receivedMessage.slaveMac[1],
                receivedMessage.slaveMac[2], receivedMessage.slaveMac[3],
                receivedMessage.slaveMac[4], receivedMessage.slaveMac[5]);

  // แสดงข้อความที่รับมา
  Serial.printf("ข้อความที่ได้รับมา: %s\n", receivedMessage.adcMessage);

  // ส่งข้อมูลต่อไปยังปลายทาง
  sendESPNOWMessage();
}

// ฟังก์ชันสำหรับส่งข้อมูลผ่าน ESP-NOW
void sendESPNOWMessage() {
  // คัดลอกข้อมูลจากตัวแปร receivedMessage ไปที่ sendMessage
  memcpy(&sendMessage, &receivedMessage, sizeof(receivedMessage));

  // อ่านค่าจาก ADC และเก็บลงในตัวแปร sendMessage
  readADCValue();

  // ตรวจสอบว่า MAC ของ Master ไม่มีอยู่หรือไม่ (เช็คว่าเป็น noMasterMAC หรือเปล่า)
  if (memcmp(noMasterMAC, sendMessage.masterMac, 6) == 0) {
    // ตรวจสอบว่ามี Peer ที่เป็น Super Master อยู่แล้วหรือไม่
    if (esp_now_is_peer_exist(sendMessage.superMasterMac)) {
      Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
    } else {
      // ถ้าไม่มี Peer ให้ทำการเพิ่ม Super Master เป็น Peer
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, sendMessage.superMasterMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;

      // เพิ่ม Peer ให้กับ ESP-NOW
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
        return;
      } else {
        Serial.println("จับคู่ ESPNOW สำเร็จ");
      }
    }

    // ส่งข้อมูลไปยัง Super Master
    esp_err_t result = esp_now_send(sendMessage.superMasterMac, (uint8_t*)&sendMessage, sizeof(sendMessage));
    if (result == ESP_OK) {
      Serial.println("อุปกรณ์นี้ส่ง ESPNOW ได้สำเร็จ");
    } else {
      Serial.printf("อุปกรณ์นี้ส่ง ESPNOW ไม่สำเร็จเนื่องจาก: %d\n", result);
    }
  } else {
    // ตรวจสอบว่า Peer ของ Master มีอยู่แล้วหรือไม่
    if (esp_now_is_peer_exist(sendMessage.masterMac)) {
      Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
    } else {
      // ถ้าไม่มี Peer ให้ทำการเพิ่ม Master เป็น Peer
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, sendMessage.masterMac, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;

      // เพิ่ม Peer ให้กับ ESP-NOW
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("จับคู่ ESPNOW ไม่สำเร็จ");
        return;
      } else {
        Serial.println("จับคู่ ESPNOW สำเร็จ");
      }
    }

    // ส่งข้อมูลไปยัง Master
    esp_err_t result = esp_now_send(sendMessage.masterMac, (uint8_t*)&sendMessage, sizeof(sendMessage));
    if (result == ESP_OK) {
      Serial.println("อุปกรณ์นี้ส่ง ESPNOW ได้สำเร็จ");
    } else {
      Serial.printf("อุปกรณ์นี้ส่ง ESPNOW ไม่สำเร็จเนื่องจาก: %d\n", result);
    }
  }
}

// ฟังก์ชันสำหรับอ่านค่าจาก ADC และแปลงค่าเป็น String
void readADCValue() {
  int adcValueInt = analogRead(34);                        // อ่านค่า ADC จากขา 34
  float adcValueFloat = (adcValueInt / 4095.0) * 3.3;      // แปลงค่าจาก 0-4095 เป็นแรงดันไฟฟ้า
  String adcValueString = String(adcValueFloat, 2);        // แปลงค่าแรงดันเป็น String ที่มีทศนิยม 2 ตำแหน่ง
  strcpy(sendMessage.adcMessage, adcValueString.c_str());  // คัดลอกข้อความไปเก็บใน sendMessage
  Serial.printf("ค่าที่อ่านได้จาก ADC: %f\n", adcValueFloat);
}

void setup() {
  Serial.begin(115200);  // เริ่มต้นการสื่อสาร Serial
  WiFi.mode(WIFI_STA);   // ตั้งค่า WiFi ให้เป็นโหมดสถานี (Station)

  // เริ่มต้น ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถใช้งาน ESP-NOW ได้");
    return;
  }

  esp_wifi_set_max_tx_power(84);              // ตั้งค่ากำลังส่ง WiFi สูงสุด
  esp_now_register_recv_cb(onReceiveESPNOW);  // ลงทะเบียนฟังก์ชัน callback สำหรับการรับข้อมูล ESP-NOW
}

void loop() {
  // ไม่มีการทำงานใน loop
}