#include "DHT.h"

#define DHTPIN 32      // กำหนดขาที่ใช้เชื่อมต่อกับเซ็นเซอร์ DHT22
#define DHTTYPE DHT22  // กำหนดชนิดของเซ็นเซอร์เป็น DHT22
#define NTC 33

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  // อ่านค่าอุณหภูมิและความชื้นจากเซ็นเซอร์
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // ตรวจสอบว่ามีการอ่านค่าล้มเหลวหรือไม่
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // แสดงผลอุณหภูมิและความชื้น
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  float tempC = 1 / (log(1 / (4096. / analogRead(NTC) - 1)) / 3950 + 1.0 / 298.15) - 273.15;
  tempC = tempC * 0.802 + 0.485;  // รวมค่า Offset ทั้งหมด
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println(" ℃");

  delay(1000);  // Delay for one second before reading again
}