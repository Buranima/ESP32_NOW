#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <driver/uart.h>
#include <esp_wifi.h>
#include <string.h>

// กำหนดการเชื่อมต่อ WiFi
const char* ssid = "989ait";
const char* password = "1234567890";

// กำหนดการเชื่อมต่อ MQTT
const char* mqttServer = "vr.carotail.com";
const int mqttPort = 28813;
const char* mqttUser = "techlabs";
const char* mqttPassword = "ASDzxc!@#QwE123";
const char* topicServer = "CPS485/server";
const char* topicClient = "CPS485/client";

// กำหนดพอร์ต UART
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)
#define UART_TIMEOUT_MS 1000

// กำหนดขา LED
const int led_PIN_1 = 4;  // GPIO 32
const int led_PIN_2 = 5;  // GPIO 33

// สร้าง client สำหรับ MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// ตัวแปรเพื่อเก็บ MAC Address
char clientId[25];
uint8_t uartBuffer[BUF_SIZE];

// สร้าง JSON document
StaticJsonDocument<256> docPage;

// ฟังก์ชั่น callback เมื่อได้รับข้อมูลจาก MQTT
void callback(char* topicServer, byte* payload, unsigned int length) {
  Serial.print("Message received in topicServer: ");
  Serial.println(topicServer);

  // อ่านข้อมูลที่รับจาก MQTT
  Serial.print("Message: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // สร้าง JSON document ขนาดที่เหมาะสมกับข้อมูลที่เราจะ parse
  StaticJsonDocument<1024> doc;

  // Parse ข้อความ JSON จาก MQTT payload
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // แสดงผลข้อมูลที่ดึงมาจาก JSON
  Serial.println("MAC Master Json: " + String(doc["mac-master"]));

  // แสดง MAC Address ที่ใช้เป็น clientId
  Serial.println("Client ID: " + String(clientId));

  // เปรียบเทียบ clientId กับ mac-master
  if (strcmp(clientId, doc["mac-master"]) == 0) {
    Serial.println("Mac Master ตรงกับอุปกรณ์นี้");

    if (strcmp("request-data-normal", doc["mode"]) == 0) {
      Serial.println("Mode: " + String(doc["mode"]));

      // อ่านค่า JSON จาก document
      int lastindex = doc["mac-slave"].size() - 1;
      String macSlaveLastIndex = doc["mac-slave"][lastindex];
      Serial.println("หน้าปัจจุบันคือ: " + String(doc["per-page"]));
      Serial.println("จำนวนหน้าสูงสุดคือ: " + String(doc["max-page"]));
      Serial.println("Mac Slave สุดท้ายของหน้านี้คือ: " + macSlaveLastIndex);

      docPage["mac-slave"] = doc["mac-slave"][lastindex];
      docPage["per-page"] = doc["per-page"];
      docPage["max-page"] = doc["max-page"];

      if (int(docPage["per-page"]) <= int(docPage["max-page"])) {
        // สร้าง JSON Document ใหม่
        StaticJsonDocument<1024> newDoc;
        JsonArray macSlaveArray = newDoc.createNestedArray("mac-slave");

        // คัดลอกข้อมูล mac-slave จาก doc ไปยัง newDoc
        for (JsonVariant value : doc["mac-slave"].as<JsonArray>()) {
          macSlaveArray.add(value.as<String>());  // เพิ่มแต่ละค่าใน array
        }

        // แปลง JSON document ใหม่เป็นสตริง
        String jsonString;
        serializeJson(newDoc, jsonString);  // แปลง JSON document เป็นสตริง

        // ส่งข้อมูลผ่าน UART
        Serial.println("ข้อมูลที่ส่งไปยัง Uart: " + jsonString);
        uart_write_bytes(UART_NUM_1, jsonString.c_str(), jsonString.length());
        Serial.println("");
      }

    } else if (strcmp("request-data-middel", doc["mode"]) == 0) {
      Serial.println("Mode: " + String(doc["mode"]));

      // // อ่านค่า JSON จาก document
      // maxPage = doc["max-page"];
      // int lastindex = doc["mac-slave"].size() - 1;
      // String macSlaveLastIndex = doc["mac-slave"][lastindex];
      // Serial.println(maxPage);
      // Serial.println(macSlaveLastIndex);

      // // ส่งข้อมูลที่ได้รับจาก MQTT ไปยัง UART
      // // uart_write_bytes(UART_NUM_1, message.c_str(), message.length());
      // Serial.println("Data sent to UART.");
    }
  } else {
    Serial.println("Mac Master ไม่ตรงกับอุปกรณ์นี้");
  }
}

// ฟังก์ชั่นการเชื่อมต่อกับ WiFi
void setupWiFi() {
  WiFi.disconnect();  // ตัดการเชื่อมต่อก่อน เพื่อป้องกันปัญหา
  // WiFi.setTxPower(WIFI_POWER_20dBm);
  esp_wifi_set_max_tx_power(84);
  Serial.println("กำลังเชื่อมต่อ WiFi...");
  WiFi.begin(ssid, password);

  int wifiRetryCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < 20) {  // ลองเชื่อมต่อสูงสุด 20 ครั้ง
    delay(500);
    Serial.print(".");
    wifiRetryCount++;

    // ปิดไฟ LED
    digitalWrite(led_PIN_1, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
    digitalWrite(led_PIN_2, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
  }

  if (WiFi.status() == WL_CONNECTED) {
    // เปิดไฟ LED
    digitalWrite(led_PIN_1, HIGH);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
    digitalWrite(led_PIN_2, HIGH);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
    Serial.println("\nเชื่อมต่อ WiFi สำเร็จ");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());

    // ดึง MAC Address ของ ESP32 และเก็บในตัวแปร clientId
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(clientId, 25, "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    Serial.print("Client ID: ");
    Serial.println(clientId);  // แสดง MAC Address ที่ใช้เป็น clientId
  } else {
    Serial.println("\เชื่อมต่อ WiFi ไม่สำเร็จ");
    ESP.restart();
  }
}

// ฟังก์ชั่น reconnect สำหรับเชื่อมต่อกับ MQTT หากการเชื่อมต่อขาด
void reconnect() {
  while (!client.connected()) {
    Serial.print("กำลังเชื่อมต่อ MQTT...");
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("เชื่อมต่อสำเร็จ");
      if (client.publish(topicClient, "ESP ได้เชื่อมต่อกับ MQTT แล้ว")) {
        Serial.println("ส่งข้อมูลไปยัง MQTT สำเร็จ");
      } else {
        Serial.println("ส่งข้อมูลไปยัง MQTT ไม่สำเร็จ");
      }
      client.subscribe(topicServer);
    } else {
      Serial.print("ไม่สามารถเชื่อมต่อ MQTT ได้เนื่องจาก, rc=");
      Serial.print(client.state());
      Serial.println("รอ 1 วินาทีก่อนลองใหม่");
      delay(1000);
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("ไม่ได้เชื่อมต่อ WiFi กำลังทำการเชื่อมต่อใหม่");

      // ปิดไฟ LED
      digitalWrite(led_PIN_1, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
      digitalWrite(led_PIN_2, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED

      ESP.restart();
    }
  }
}

// ฟังก์ชั่น setupUART สำหรับตั้งค่า UART
void setupUART() {
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };

  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM_1, BUF_SIZE, 0, 0, NULL, 0);
}

// ฟังก์ชั่นเพื่อรับข้อมูล JSON จาก UART และส่งไปยัง MQTT
void readUARTAndSendToMQTT() {
  int len = uart_read_bytes(UART_NUM_1, uartBuffer, BUF_SIZE, UART_TIMEOUT_MS / portTICK_RATE_MS);

  if (len > 0) {

    // แปลงข้อมูลที่รับจาก UART เป็น String
    String uartMessage = String((char*)uartBuffer);
    Serial.print("ข้อมูลที่รับมาจาก Uart: ");
    Serial.println(uartMessage);

    // แยกข้อความ JSON ออกมาเป็นแต่ละชุด
    int start = 0;
    while (start < uartMessage.length()) {
      int end = uartMessage.indexOf('}', start);
      if (end == -1) break;

      String singleMessage = uartMessage.substring(start, end + 1);
      Serial.println("ข้อมูลที่ส่งไปยัง Server: " + singleMessage);

      // สร้าง JSON document
      StaticJsonDocument<256> docResult;

      DeserializationError error = deserializeJson(docResult, singleMessage);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // ส่งข้อมูลไปยัง MQTT
      sendMQTTMessage(singleMessage);

      if (docResult["mac-slave"] == docPage["mac-slave"]) {
        if (int(docPage["per-page"]) < int(docPage["max-page"])) {
          // สร้าง JSON document
          StaticJsonDocument<256> docnextpage;
          docnextpage["mode"] = "request-next-data";
          docnextpage["mac-master"] = clientId;
          docnextpage["nextpage"] = int(docPage["per-page"]) + 1;

          serializeJson(docnextpage, singleMessage);  // แปลง JSON document เป็นสตริง

          Serial.println("ข้อมูลที่ส่งไปยัง Server: " + singleMessage);
          sendMQTTMessage(singleMessage);
        }
      }

      start = end + 1;
    }
  }
}

void sendMQTTMessage(String message) {
  if (client.publish(topicClient, message.c_str())) {
    Serial.println("ส่งข้อมูลไปยัง MQTT สำเร็จ");
  } else {
    Serial.println("ส่งข้อมูลไปยัง MQTT ไม่สำเร็จ");
  }
  // delayMicroseconds(100000);
}

void setup() {
  Serial.begin(115200);

  // ตั้งค่าขา GPIO ให้เป็น OUTPUT เพื่อควบคุม LED
  pinMode(led_PIN_1, OUTPUT);
  pinMode(led_PIN_2, OUTPUT);

  // ปิดไฟ LED
  digitalWrite(led_PIN_1, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
  digitalWrite(led_PIN_2, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED

  setupWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  setupUART();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ไม่ได้เชื่อมต่อ WiFi กำลังทำการเชื่อมต่อใหม่");

    // ปิดไฟ LED
    digitalWrite(led_PIN_1, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED
    digitalWrite(led_PIN_2, LOW);  // ส่งสัญญาณ HIGH เพื่อเปิดไฟ LED

    ESP.restart();
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  readUARTAndSendToMQTT();
}