#include <esp_now.h>
#include <WiFi.h>

uint8_t mac_mother[6] = {0, 0, 0, 0, 0, 0};

float response = -99.9;

void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  Serial.println("Data received from master");
  if (len == sizeof(mac_mother)) {
    memcpy(mac_mother, recv_info->src_addr, 6);
    Serial.print("Master MAC Address received: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X:", mac_mother[i]);
    }
    Serial.println();
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_mother, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add master as peer. Attempting to remove previous peer and retry.");
      esp_now_del_peer(mac_mother);
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add master as peer after retrying");
        return;
      }
    }
    esp_err_t result = esp_now_send(mac_mother, (uint8_t *)&response, sizeof(response));
    if (result == ESP_OK) {
      Serial.println("Send success");
    } else {
      Serial.println("Send failed");
    }
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // ไม่มีการทำงานใน loop
}
