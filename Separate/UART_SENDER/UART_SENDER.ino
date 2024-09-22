#include <Arduino.h>
#include <string.h>
#include "driver/uart.h"

#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define BUF_SIZE (1024)

typedef struct structMessageSend {
    uint8_t superMasterMacAddress[6];
    uint8_t masterMacAddress[6];
    uint8_t slaveMacAddress[6];
    char stringMessage[9];
} structMessageSend;

void sendDataOverUart(structMessageSend *message) {
    uint8_t buffer[sizeof(structMessageSend)];
    memcpy(buffer, message, sizeof(structMessageSend));
    
    uart_write_bytes(UART_NUM_1, (const char *)buffer, sizeof(buffer));
}

void setup() {
    // Configure UART
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

    Serial.begin(115200);
}

void loop() {
    structMessageSend message = {
        .superMasterMacAddress = {0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x88},
        .masterMacAddress = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
        .slaveMacAddress = {0xcc, 0xdb, 0xa7, 0x32, 0xa7, 0x88},
        .stringMessage = "null"
    };

    sendDataOverUart(&message);
    Serial.println("Send UART");
    delay(5000);
}