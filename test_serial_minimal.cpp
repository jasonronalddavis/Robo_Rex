// Minimal test to verify serial output works
// Rename this to main.cpp temporarily to test

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize

  Serial.println("=== SERIAL TEST ===");
  Serial.println("If you see this, serial is working!");
  Serial.println("Board: ESP32-S3");
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  static uint32_t counter = 0;
  Serial.print("Counter: ");
  Serial.println(counter++);
  delay(1000);
}
