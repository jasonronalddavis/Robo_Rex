// Simple Serial Test for ESP32-S3
// This will help diagnose serial communication issues

void setup() {
  // Try both USB Serial and UART Serial
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize

  // Blink onboard LED to show code is running
  pinMode(LED_BUILTIN, OUTPUT);

  // Print to serial every second
  for (int i = 0; i < 10; i++) {
    Serial.println("===== ESP32-S3 SERIAL TEST =====");
    Serial.print("Test message #");
    Serial.println(i);
    Serial.print("Millis: ");
    Serial.println(millis());
    Serial.println("If you see this, serial is working!");
    Serial.println();

    // Blink LED
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

void loop() {
  Serial.println("Loop running...");
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  delay(1000);
}
