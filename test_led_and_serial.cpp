// MINIMAL TEST WITH LED - Verify code is actually running
// LED will blink even if serial doesn't work
// Freenove ESP32-S3 WROOM: GPIO 48 = onboard RGB LED

#include <Arduino.h>

#define LED 48  // Onboard LED on Freenove ESP32-S3

void setup() {
  // Setup LED first
  pinMode(LED, OUTPUT);

  // Blink rapidly 10 times on startup (shows code is running)
  for(int i = 0; i < 10; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }

  // Setup Serial
  Serial.begin(115200);
  delay(3000);  // Long delay

  // Send startup messages
  Serial.println();
  Serial.println("========================================");
  Serial.println("ESP32-S3 LED + SERIAL TEST");
  Serial.println("========================================");
  Serial.println("If LED is blinking: CODE IS RUNNING");
  Serial.println("If you see this: SERIAL IS WORKING");
  Serial.println("========================================");
  Serial.println();
}

void loop() {
  // Blink LED every 1 second
  digitalWrite(LED, HIGH);
  Serial.println("LED ON  - If you see this, serial works!");
  delay(1000);

  digitalWrite(LED, LOW);
  Serial.println("LED OFF - Still printing...");
  delay(1000);
}
