#include <Arduino.h>
#include <string.h>
#include <driver/uart.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)

typedef struct structMessageSend {
  uint8_t superMasterMacAddress[6];
  uint8_t masterMacAddress[6];
  uint8_t slaveMacAddress[6];
  char stringMessage[16];
} structMessageSend;
structMessageSend myDatacallbackESPNOW;
structMessageSend myDatacallbackUART;
structMessageSend myDatasendESPNOWMessage;
structMessageSend myDatasendUARTMessage;

uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int retries = 0;
const int maxRetries = 3;
String valueADCString;
unsigned long previousMillis = 0;
const long interval = 30;

void callbackUART() {
  uint8_t bufferUART[sizeof(myDatacallbackUART)];
  int lenUART = uart_read_bytes(UART_NUM_1, bufferUART, sizeof(myDatacallbackUART), 20 / portTICK_RATE_MS);
  if (lenUART > 0) {
    memcpy(&myDatacallbackUART, bufferUART, sizeof(myDatacallbackUART));
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.superMasterMacAddress[0], myDatacallbackUART.superMasterMacAddress[1],
                  myDatacallbackUART.superMasterMacAddress[2], myDatacallbackUART.superMasterMacAddress[3],
                  myDatacallbackUART.superMasterMacAddress[4], myDatacallbackUART.superMasterMacAddress[5]);
    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.masterMacAddress[0], myDatacallbackUART.masterMacAddress[1],
                  myDatacallbackUART.masterMacAddress[2], myDatacallbackUART.masterMacAddress[3],
                  myDatacallbackUART.masterMacAddress[4], myDatacallbackUART.masterMacAddress[5]);
    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDatacallbackUART.slaveMacAddress[0], myDatacallbackUART.slaveMacAddress[1],
                  myDatacallbackUART.slaveMacAddress[2], myDatacallbackUART.slaveMacAddress[3],
                  myDatacallbackUART.slaveMacAddress[4], myDatacallbackUART.slaveMacAddress[5]);
    Serial.printf("String Message: %s\n", myDatacallbackUART.stringMessage);
    sendESPNOWMessage();
  }
}

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
  memcpy(&myDatasendUARTMessage, &myDatacallbackESPNOW, sizeof(myDatacallbackESPNOW));
  sendUARTMessage();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  setupUART();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  esp_wifi_set_max_tx_power(84);
  esp_now_register_recv_cb(callbackESPNOW);
  esp_now_register_send_cb(callbackSendESPNOW);
}

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

void sendESPNOWMessage() {
  memcpy(&myDatasendESPNOWMessage, &myDatacallbackUART, sizeof(myDatacallbackUART));
  if (memcmp(noMaster, myDatasendESPNOWMessage.masterMacAddress, 6) == 0) {
    if (esp_now_is_peer_exist(myDatasendESPNOWMessage.slaveMacAddress)) {
      Serial.println("Peer already exists, no need to add.");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
      memcpy(peerInfo.peer_addr, myDatasendESPNOWMessage.slaveMacAddress, 6);
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
    retries = 0;
    while (retries < maxRetries) {
      unsigned long currentMillis = millis();
      if ((currentMillis - previousMillis >= interval) || (previousMillis > currentMillis)) {
        previousMillis = currentMillis;
        esp_err_t result = esp_now_send(myDatasendESPNOWMessage.slaveMacAddress, (uint8_t*)&myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
        if (result == ESP_OK) {
          Serial.println("Message sent successfully");
        } else {
          Serial.printf("Error sending message: %d\n", result);
        }
      }
    }
    retries++;
    if (retries > maxRetries) {
      valueADCString = "null";
      strcpy(myDatasendESPNOWMessage.stringMessage, valueADCString.c_str());
      memcpy(&myDatasendUARTMessage, &myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
      sendUARTMessage();
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
    retries = 0;
    while (retries < maxRetries) {
      unsigned long currentMillis = millis();
      if ((currentMillis - previousMillis >= interval) || (previousMillis > currentMillis)) {
        previousMillis = currentMillis;
        esp_err_t result = esp_now_send(myDatasendESPNOWMessage.masterMacAddress, (uint8_t*)&myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
        if (result == ESP_OK) {
          Serial.println("Message sent successfully");
        } else {
          Serial.printf("Error sending message: %d\n", result);
        }
      }
    }
    retries++;
    if (retries > maxRetries) {
      valueADCString = "null";
      strcpy(myDatasendESPNOWMessage.stringMessage, valueADCString.c_str());
      memcpy(&myDatasendUARTMessage, &myDatasendESPNOWMessage, sizeof(myDatasendESPNOWMessage));
      sendUARTMessage();
    }
  }
}

void sendUARTMessage() {
  uint8_t bufferUART[sizeof(myDatasendUARTMessage)];
  memcpy(bufferUART, &myDatasendUARTMessage, sizeof(myDatasendUARTMessage));
  uart_write_bytes(UART_NUM_1, (const char*)bufferUART, sizeof(bufferUART));
}

void callbackSendESPNOW(const uint8_t* macAddresscallbackSendESPNOW, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("ส่งข้อมูลสำเร็จ!");
    retries = 0;
  } else {
    Serial.println("การส่งข้อมูลล้มเหลว!");
    retries++;
  }
}

void loop() {
  callbackUART();
}