#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];
  uint8_t masterMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[16];
} structMessageSend;
structMessageSend myDatacallbackESPNOW;
structMessageSend myDatasendESPNOWMessage;

uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void callbackESPNOW(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  memcpy(&myDatacallbackESPNOW, incomingData, sizeof(myDatacallbackESPNOW));
  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDatacallbackESPNOW.superMasterMacAddress[0], myDatacallbackESPNOW.superMasterMacAddress[1],
                myDatacallbackESPNOW.superMasterMacAddress[2], myDatacallbackESPNOW.superMasterMacAddress[3],
                myDatacallbackESPNOW.superMasterMacAddress[4], myDatacallbackESPNOW.superMasterMacAddress[5]);
  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDatacallbackESPNOW.masterMacAddress[0], myDatacallbackESPNOW.masterMacAddress[1],
                myDatacallbackESPNOW.masterMacAddress[2], myDatacallbackESPNOW.masterMacAddress[3],
                myDatacallbackESPNOW.masterMacAddress[4], myDatacallbackESPNOW.masterMacAddress[5]);
  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDatacallbackESPNOW.slaveMacAddress[0], myDatacallbackESPNOW.slaveMacAddress[1],
                myDatacallbackESPNOW.slaveMacAddress[2], myDatacallbackESPNOW.slaveMacAddress[3],
                myDatacallbackESPNOW.slaveMacAddress[4], myDatacallbackESPNOW.slaveMacAddress[5]);
  Serial.printf("String Message: %s\n", myDatacallbackESPNOW.stringMessage);
  sendESPNOWMessage();
}

void sendESPNOWMessage() {
  memcpy(&myDatasendESPNOWMessage, &myDatacallbackESPNOW, sizeof(myDatacallbackESPNOW));
  getADC();
  if (memcmp(noMaster, myDatasendESPNOWMessage.masterMacAddress, 6) == 0) {
    if (esp_now_is_peer_exist(myDatasendESPNOWMessage.superMasterMacAddress)) {
      Serial.println("Peer already exists, no need to add.");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, myDatasendESPNOWMessage.superMasterMacAddress, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
      } else {
        Serial.println("Peer added successfully.");
      }
    }
    esp_err_t result = esp_now_send(myDatasendESPNOWMessage.superMasterMacAddress, (uint8_t*)&myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
    if (result == ESP_OK) {
      Serial.println("Message sent successfully");
    } else {
      Serial.printf("Error sending message: %d\n", result);
    }
  } else {
    if (esp_now_is_peer_exist(myDatasendESPNOWMessage.masterMacAddress)) {
      Serial.println("Peer already exists, no need to add.");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, myDatasendESPNOWMessage.masterMacAddress, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      peerInfo.ifidx = WIFI_IF_STA;
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
      } else {
        Serial.println("Peer added successfully.");
      }
    }
    esp_err_t result = esp_now_send(myDatasendESPNOWMessage.masterMacAddress, (uint8_t*)&myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
    if (result == ESP_OK) {
      Serial.println("Message sent successfully");
    } else {
      Serial.printf("Error sending message: %d\n", result);
    }
  }
}

void getADC() {
  int valueADCInt = analogRead(34);
  float valueADCFloat = (valueADCInt / 4095.0) * 3.3;
  String valueADCString = String(valueADCFloat, 2);
  strcpy(myDatasendESPNOWMessage.stringMessage, valueADCString.c_str());
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ไม่สามารถเริ่ม ESP-NOW ได้");
    return;
  }
  esp_wifi_set_max_tx_power(84);
  esp_now_register_recv_cb(callbackESPNOW);
}

void loop() {
}