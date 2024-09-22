// ADC1: Channels 0 to 7 (GPIOs 32 to 39)
// ADC2: Channels 0 to 9 (GPIOs 0, 2, 4, 12-15, 25-27)

void setup() {
  Serial.begin(115200);
}

void loop() {
  int adcValue = analogRead(34);            // อ่านค่า ADC
  float value = (adcValue / 4095.0) * 3.3;  // แปลงค่าจาก ADC เป็นแรงดัน (0-3.3V)

  // สมมุติว่าเราจะใช้ค่าที่ได้ในการเก็บเป็นสตริง
  String valueString = String(value, 2);  // แปลงเป็นสตริง โดยมีทศนิยม 2 ตำแหน่ง

  Serial.println(valueString);  // แสดงค่าสตริง
  delay(1000);                  // หน่วงเวลา 1 วินาที
}