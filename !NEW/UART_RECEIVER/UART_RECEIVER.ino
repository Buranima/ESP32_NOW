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
    char stringMessage[9];  // ขยายขนาดเป็น 9 เพื่อรองรับ '\0'
} structMessageSend;

void receiveDataFromUart() {
    uint8_t buffer[BUF_SIZE];
    structMessageSend receivedMessage;

    int len = uart_read_bytes(UART_NUM_1, buffer, sizeof(receivedMessage), 20 / portTICK_RATE_MS);
    if (len > 0) {
        memcpy(&receivedMessage, buffer, sizeof(receivedMessage));

        // แสดงผลข้อมูลที่ได้รับ
        Serial.printf("Super Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            receivedMessage.superMasterMacAddress[0], receivedMessage.superMasterMacAddress[1], 
            receivedMessage.superMasterMacAddress[2], receivedMessage.superMasterMacAddress[3], 
            receivedMessage.superMasterMacAddress[4], receivedMessage.superMasterMacAddress[5]);

        Serial.printf("Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            receivedMessage.masterMacAddress[0], receivedMessage.masterMacAddress[1], 
            receivedMessage.masterMacAddress[2], receivedMessage.masterMacAddress[3], 
            receivedMessage.masterMacAddress[4], receivedMessage.masterMacAddress[5]);

        Serial.printf("Slave MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            receivedMessage.slaveMacAddress[0], receivedMessage.slaveMacAddress[1], 
            receivedMessage.slaveMacAddress[2], receivedMessage.slaveMacAddress[3], 
            receivedMessage.slaveMacAddress[4], receivedMessage.slaveMacAddress[5]);

        Serial.printf("String Message: %s\n", receivedMessage.stringMessage);
    }
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
    
    Serial.begin(115200);  // เปิด Serial Monitor
}

void loop() {
    receiveDataFromUart();  // รับข้อมูล
    // delay(1000);
}