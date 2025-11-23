// LED + Serial Test for ESP32-S3 Freenove WROOM
// Tests BOTH serial output AND LED blinking
// If LED blinks but no serial: Serial port/monitoring issue
// If no LED and no serial: Code not running (power/upload issue)

#include <Arduino.h>

// Freenove ESP32-S3 WROOM onboard RGB LED
#define LED_PIN 48  // GPIO 48 = WS2812 RGB LED data pin

// Simple blink counter
unsigned long lastBlink = 0;
unsigned long lastSerial = 0;
bool ledState = false;
int counter = 0;

void setup() {
  // Setup LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Turn LED ON initially

  // Setup serial with longer delay
  Serial.begin(115200);
  delay(2000);  // Give serial extra time on ESP32-S3

  // Wait for serial to be ready (but timeout after 5 seconds)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 5000)) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Flash LED while waiting
    delay(100);
  }

  digitalWrite(LED_PIN, HIGH);  // LED on solid

  // Send startup banner
  Serial.println();
  Serial.println("========================================");
  Serial.println("ESP32-S3 LED + SERIAL TEST");
  Serial.println("========================================");
  Serial.println("Freenove ESP32-S3 WROOM");
  Serial.println("CH340 USB-UART Chip");
  Serial.println("========================================");
  Serial.println();
  Serial.print("Serial initialized at: ");
  Serial.print(115200);
  Serial.println(" baud");
  Serial.print("LED pin: GPIO ");
  Serial.println(LED_PIN);
  Serial.println();
  Serial.println("If you see this message:");
  Serial.println("  ✅ Serial communication is WORKING!");
  Serial.println("  ✅ Code is running");
  Serial.println("  ✅ CH340 driver is OK");
  Serial.println();
  Serial.println("LED should blink once per second.");
  Serial.println("Counter will increment every second.");
  Serial.println();
  Serial.println("Press RESET button to restart.");
  Serial.println("========================================");
  Serial.println();

  delay(1000);
}

void loop() {
  unsigned long now = millis();

  // Blink LED every 1000ms
  if (now - lastBlink >= 1000) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }

  // Print counter every 1000ms
  if (now - lastSerial >= 1000) {
    lastSerial = now;
    counter++;

    Serial.print("Counter: ");
    Serial.print(counter);
    Serial.print("  |  LED: ");
    Serial.print(ledState ? "ON " : "OFF");
    Serial.print("  |  Uptime: ");
    Serial.print(now / 1000);
    Serial.println(" seconds");
  }
}
