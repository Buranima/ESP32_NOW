// กำหนดขาที่เชื่อมต่อกับเซนเซอร์ LM35
const int lm35Pin = 32;

void setup() {
  Serial.begin(115200); // เริ่มการสื่อสาร Serial

  analogReadResolution(12);
  
  // ปรับระดับการลดทอนแรงดันของ ADC (ใช้การลดทอนค่าเริ่มต้น 11dB เพื่ออ่านค่า 0-3.3V)
  analogSetAttenuation(ADC_11db); 
}

void loop() {
  // อ่านค่าจาก LM35 ที่ขา 32
  int analogValue = analogRead(lm35Pin);

  // Serial.println(analogValue);

  // แปลงค่า ADC เป็นอุณหภูมิในเซลเซียส
  float voltage = analogValue * (2.2 / 1024.0);  // คำนวณแรงดันไฟฟ้า
  Serial.println(voltage);
  float temperatureC = voltage * 100;  // LM35 ให้เอาต์พุต 10mV ต่อองศาเซลเซียส

  // แสดงค่าอุณหภูมิ
  Serial.print("Temperature (C): ");
  Serial.println(temperatureC);

  delay(1000); // รอ 1 วินาที
}