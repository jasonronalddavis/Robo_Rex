// ULTRA VERBOSE SERIAL TEST
// Prints constantly from both setup() and loop()
// Tests if ANYTHING is coming through serial

#include <Arduino.h>

void setup() {
  // Try Serial0 explicitly (hardware UART)
  Serial.begin(115200);

  // Wait and send messages during wait
  for(int i = 0; i < 50; i++) {
    Serial.println("SETUP: Initializing...");
    delay(100);
  }

  Serial.println("");
  Serial.println("========================================");
  Serial.println("SETUP COMPLETE - ESP32-S3 IS RUNNING!");
  Serial.println("========================================");
  Serial.println("");
}

void loop() {
  static int counter = 0;

  Serial.print("LOOP ");
  Serial.print(counter++);
  Serial.print(" - Millis: ");
  Serial.println(millis());

  delay(500);  // Print twice per second
}
