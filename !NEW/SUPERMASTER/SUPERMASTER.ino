#include <Arduino.h>
#include <string.h>
#include <driver/uart.h>

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
    // sendMQTTMessage();
  }
}

void setup() {
  Serial.begin(115200);
  setupUART();
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

void loop() {
  callbackUART();
}