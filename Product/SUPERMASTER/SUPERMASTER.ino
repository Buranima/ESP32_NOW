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
structMessageSend myDataMessageSend;

uint8_t noMaster[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void callbackUART() {
  uint8_t bufferUART[sizeof(myDataMessageSend)];
  int lenUART = uart_read_bytes(UART_NUM_1, bufferUART, sizeof(myDataMessageSend), 20 / portTICK_RATE_MS);
  if (lenUART > 0) {
    memcpy(&myDataMessageSend, bufferUART, sizeof(myDataMessageSend));
    // แสดงผลข้อมูลที่ได้รับ
    Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDataMessageSend.superMasterMacAddress[0], myDataMessageSend.superMasterMacAddress[1],
                  myDataMessageSend.superMasterMacAddress[2], myDataMessageSend.superMasterMacAddress[3],
                  myDataMessageSend.superMasterMacAddress[4], myDataMessageSend.superMasterMacAddress[5]);

    Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDataMessageSend.masterMacAddress[0], myDataMessageSend.masterMacAddress[1],
                  myDataMessageSend.masterMacAddress[2], myDataMessageSend.masterMacAddress[3],
                  myDataMessageSend.masterMacAddress[4], myDataMessageSend.masterMacAddress[5]);

    Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  myDataMessageSend.slaveMacAddress[0], myDataMessageSend.slaveMacAddress[1],
                  myDataMessageSend.slaveMacAddress[2], myDataMessageSend.slaveMacAddress[3],
                  myDataMessageSend.slaveMacAddress[4], myDataMessageSend.slaveMacAddress[5]);

    Serial.printf("String Message: %s\n", myDataMessageSend.stringMessage);
    sendESPNOWMessage();
  }
}

void callbackESPNOW(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  memcpy(&myDataMessageSend, incomingData, sizeof(myDataMessageSend));
  Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDataMessageSend.superMasterMacAddress[0], myDataMessageSend.superMasterMacAddress[1],
                myDataMessageSend.superMasterMacAddress[2], myDataMessageSend.superMasterMacAddress[3],
                myDataMessageSend.superMasterMacAddress[4], myDataMessageSend.superMasterMacAddress[5]);

  Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDataMessageSend.masterMacAddress[0], myDataMessageSend.masterMacAddress[1],
                myDataMessageSend.masterMacAddress[2], myDataMessageSend.masterMacAddress[3],
                myDataMessageSend.masterMacAddress[4], myDataMessageSend.masterMacAddress[5]);

  Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myDataMessageSend.slaveMacAddress[0], myDataMessageSend.slaveMacAddress[1],
                myDataMessageSend.slaveMacAddress[2], myDataMessageSend.slaveMacAddress[3],
                myDataMessageSend.slaveMacAddress[4], myDataMessageSend.slaveMacAddress[5]);

  Serial.printf("String Message: %s\n", myDataMessageSend.stringMessage);
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
  esp_now_register_recv_cb(callbackESPNOW);
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
  if (memcmp(noMaster, myDataMessageSend.masterMacAddress, 6) == 0) {
    // No master available, communicate with the slave directly
    if (esp_now_is_peer_exist(myDataMessageSend.slaveMacAddress)) {
      Serial.println("Peer already exists, no need to add.");
    } else {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));  // Initialize peerInfo with zeros
      memcpy(peerInfo.peer_addr, myDataMessageSend.slaveMacAddress, 6);
      peerInfo.channel = 0;          // Set the communication channel
      peerInfo.encrypt = false;      // Set encryption (false in this case)
      peerInfo.ifidx = WIFI_IF_STA;  // Set the interface to STA mode

      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;  // Exit if peer addition fails
      } else {
        Serial.println("Peer added successfully.");
      }
    }

    // Send ESP-NOW message
    esp_err_t result = esp_now_send(myDataMessageSend.slaveMacAddress, (uint8_t*)&myDataMessageSend, sizeof(myDataMessageSend));
    if (result == ESP_OK) {
      Serial.println("Message sent successfully");
    } else {
      Serial.printf("Error sending message: %d\n", result);
    }
  } else {
    // Handle when there is a Master MAC Address present
    // Your logic here to communicate with master
  }
}

void sendUARTMessage() {
  uint8_t bufferUART[sizeof(myDataMessageSend)];
  memcpy(bufferUART, &myDataMessageSend, sizeof(structMessageSend));
  uart_write_bytes(UART_NUM_1, (const char*)bufferUART, sizeof(bufferUART));
}

void loop() {
  callbackUART();
}