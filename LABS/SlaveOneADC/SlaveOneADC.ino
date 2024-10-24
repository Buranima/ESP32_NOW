#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <string.h>

// กำหนดขา ADC
const int adc_PIN_1 = 32;  // GPIO 32
const int adc_PIN_2 = 33;  // GPIO 33

// กำหนดขา LED
const int led_PIN_1 = 4;  // GPIO 32
const int led_PIN_2 = 5;  // GPIO 33

typedef struct struct_message {
  uint8_t dataESPNOW[250];  // ขนาดสูงสุด 250 ไบต์
} struct_message;
struct_message dataToSendESPNOW;

char clientId[25];
float temperature;

// สร้าง JSON document
StaticJsonDocument<256> docLED;

// ฟังก์ชั่น callback สำหรับการรับข้อมูลผ่าน ESP-NOW
void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  // แปลง byte array เป็นสตริง
  String jsonStringRecv = String((char *)incomingData);

  // สร้าง JSON document
  StaticJsonDocument<256> docRecv;

  // Parse JSON
  DeserializationError error = deserializeJson(docRecv, jsonStringRecv);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  // แสดงข้อมูลจาก JSON
  Serial.println("MAC UART: " + String(docRecv["mac-uart"].as<const char *>()));
  Serial.println("Message: " + String(docRecv["message"].as<const char *>()));

  if (strcmp("no-mac-uart", docLED["mac-uart"]) == 0) {
    docLED["mac-uart"] = docRecv["mac-uart"];
    digitalWrite(led_PIN_1, HIGH);
    digitalWrite(led_PIN_2, HIGH);
  }

  if (docLED["mac-uart"] != docRecv["mac-uart"]) {
    docLED["mac-uart"] = docRecv["mac-uart"];
    digitalWrite(led_PIN_1, LOW);
    digitalWrite(led_PIN_2, LOW);
  } else {
    digitalWrite(led_PIN_1, HIGH);
    digitalWrite(led_PIN_2, HIGH);
  }

  if (strcmp("Getdata", docRecv["message"]) == 0) {
    // อ่านค่า ADC
    updateADC();

    // สร้าง JSON document
    StaticJsonDocument<256> docSend;
    docSend["mac-slave"] = clientId;
    docSend["result"] = temperature;

    // แปลง JSON เป็นสตริง
    String jsonString;
    serializeJson(docSend, jsonString);

    // แปลงสตริงเป็น byte array
    int lenJsonString = jsonString.length() + 1;
    memcpy(dataToSendESPNOW.dataESPNOW, jsonString.c_str(), lenJsonString);

    uint8_t macAddressArray[6];

    for (int i = 0; i < 6; i++) {
      String byteString = docRecv["mac-uart"].as<String>().substring(i * 2, (i * 2) + 2);
      macAddressArray[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }

    Serial.print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", macAddressArray[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();

    // ตรวจสอบว่ามี Peer ที่เป็น Uart อยู่แล้วหรือไม่
    if (esp_now_is_peer_exist(macAddressArray)) {
      Serial.println("มีการจับคู่ ESPNOW นี้แล้ว");
    } else {

      // ถ้าไม่มี Peer ให้ทำการเพิ่ม Uart เป็น Peer
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, macAddressArray, 6);
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

    // ส่งข้อมูลด้วย ESP-NOW ไปยัง Uart
    esp_err_t result = esp_now_send(macAddressArray, (uint8_t *)&dataToSendESPNOW, sizeof(dataToSendESPNOW));
    if (result == ESP_OK) {
      Serial.println("ส่งข้อมูลด้วย ESP-NOW ไปยัง Uart สำเร็จ");
    } else {
      Serial.println("ส่งข้อมูลด้วย ESP-NOW ไปยัง Uart ไม่สำเร็จ");
    }
    Serial.println("ข้อมูลที่ส่งไปยัง Uart: " + jsonString);

    digitalWrite(led_PIN_1, HIGH);
    digitalWrite(led_PIN_2, HIGH);
  }
}

// ฟังก์ชั่นสำหรับการอ่านค่า ADC
void updateADC() {
  temperature = 0;
  int adcIntValue1 = analogRead(adc_PIN_1);
  Serial.println(adcIntValue1);

  float adcFloatValue;
  // adcFloatValue = adcIntValue1 * (3.0 / 1024.0);
  adcFloatValue = adcIntValue1 * (3.3 / 4096.0);
  // adcFloatValue = adcIntValue1 * (1.2 / 4096.0);
  Serial.println(adcFloatValue);
  adcFloatValue = adcFloatValue * 100;

  // ปัดค่า temperature ให้เป็นทศนิยม 2 ตำแหน่ง
  temperature = round(adcFloatValue * 100.0) / 100.0;

  // แสดงผลค่า temperature แบบทศนิยม 2 ตำแหน่ง
  Serial.print("อุณหภูมิที่วัดได้: ");
  Serial.print(temperature, 2);
  Serial.println(" °C\n");
}

// ฟังก์ชั่นการเชื่อมต่อ ESP-NOW
void setupESPNOW() {
  WiFi.mode(WIFI_STA);  // ตั้งค่า WiFi เป็นโหมดสถานี (STA)
  WiFi.STA.begin();

  // ดึง MAC Address ของ ESP32 และเก็บในตัวแปร clientId
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(clientId, 25, "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("Client ID: ");
  Serial.println(clientId);  // แสดง MAC Address ที่ใช้เป็น clientId
  if (esp_now_init() != ESP_OK) {
    Serial.println("เริ่มใช้งาน ESP-NOW ไม่สำเร็จ");
    ESP.restart();
  }
  esp_wifi_set_max_tx_power(84);
  esp_now_register_recv_cb(onDataRecv);  // ลงทะเบียน callback สำหรับรับข้อมูล
  Serial.println("เริ่มใช้งาน ESP-NOW ได้สำเร็จ");
}

void setup() {
  // เริ่มต้น Serial Monitor
  Serial.begin(115200);

  // เริ่มต้น ESP-NOW
  setupESPNOW();

  // ตั้งค่า ADC resolution (ความละเอียดของ ADC) เป็น 10 บิต
  analogReadResolution(12);

  // ตั้งค่า ADC Attenuation เป็น 11dB เพื่อให้ช่วงวัดได้ถึง 3.3V
  analogSetAttenuation(ADC_6db);

  // ตั้งค่าขา GPIO ให้เป็น OUTPUT เพื่อควบคุม LED
  pinMode(led_PIN_1, OUTPUT);
  pinMode(led_PIN_2, OUTPUT);

  // ปิดไฟ LED
  digitalWrite(led_PIN_1, LOW);  // ส่งสัญญาณ HIGH เพื่อปิดไฟ LED
  digitalWrite(led_PIN_2, LOW);  // ส่งสัญญาณ HIGH เพื่อปิดไฟ LED

  docLED["mac-uart"] = "no-mac-uart";
}

void loop() {
  // ไม่จำเป็นต้องทำอะไรใน loop หลักเพราะใช้ ESP-NOW ผ่าน callback
  // updateADC();
  // delay(1000);
}