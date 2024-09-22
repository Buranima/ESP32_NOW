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
}

void loop() {
    structMessageSend message = {
        .superMasterMacAddress = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
        .masterMacAddress = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66},
        .slaveMacAddress = {0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC},
        .stringMessage = "Hello"
    };

    sendDataOverUart(&message);  // ส่งข้อมูล
    delay(1000);
}