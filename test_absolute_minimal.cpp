// ABSOLUTE MINIMAL TEST - Nothing but Serial
// No libraries, no servos, no I2C, nothing
// If this doesn't work, it's a fundamental hardware/driver issue

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(3000);  // Long delay to ensure serial is ready

  // Infinite loop of messages
  while(true) {
    Serial.println("HELLO FROM ESP32-S3!");
    delay(500);
  }
}

void loop() {
  // Never reached - everything in setup()
}
