// MINIMAL ESP32-S3 SERIAL TEST
// Test ONLY serial output - no servos, no libraries, nothing else
//
// Instructions:
// 1. Disconnect ALL hardware (servos, PCA9685, everything)
// 2. Connect ESP32-S3 to PC via USB-C ONLY
// 3. Temporarily rename src/main.cpp to src/main.cpp.backup
// 4. Copy this file to src/main.cpp
// 5. Upload and check serial monitor at 115200 baud
// 6. You should see "Hello from ESP32-S3!" every second
//
// If this works: Your USB-CDC and board config are correct, issue is in main code
// If this fails: USB driver issue, wrong USB port, or board selection problem

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize

  Serial.println("========================================");
  Serial.println("ESP32-S3 MINIMAL SERIAL TEST");
  Serial.println("========================================");
  Serial.println("If you can read this, USB-CDC is working!");
  Serial.println();
}

void loop() {
  static unsigned long count = 0;

  Serial.print("Hello from ESP32-S3! Counter: ");
  Serial.println(count++);

  delay(1000);
}
