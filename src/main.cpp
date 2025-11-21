// MINIMAL SERVO TEST - NO BLE, NO IMU
// ESP32-S3 Hybrid servo control only

#include <Arduino.h>
#include "ServoBus.h"

static ServoBus servoBus;

void setup() {
  // Initialize serial with delay for USB-CDC
  Serial.begin(115200);
  delay(2000);  // Give USB serial time to initialize

  Serial.println();
  Serial.println("========================================");
  Serial.println("  ROBO REX - MINIMAL SERVO TEST");
  Serial.println("  NO BLE | NO IMU | SERVOS ONLY");
  Serial.println("========================================");
  Serial.println();

  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();

  // Initialize servo system
  Serial.println("[MAIN] Initializing servo system...");
  if (!servoBus.begin()) {
    Serial.println("[MAIN] ERROR: ServoBus initialization FAILED!");
    Serial.println("[MAIN] Check I2C wiring and PCA9685 power");
    while(1) {
      delay(1000);
      Serial.println("[MAIN] Halted - fix servo bus and reset");
    }
  }

  Serial.println("[MAIN] ServoBus initialized successfully!");
  Serial.println();

  // Attach all 16 servos
  Serial.println("[MAIN] Attaching 16 servos...");
  for (uint8_t ch = 0; ch < 16; ch++) {
    servoBus.attach(ch);
  }

  Serial.println();
  Serial.println("[MAIN] ✓ Setup complete!");
  Serial.println("[MAIN] Starting sweep test in 2 seconds...");
  Serial.println();
  delay(2000);
}

void loop() {
  static uint32_t lastUpdate = 0;
  static float position = 90.0;
  static int8_t direction = 1;

  // Update every 50ms
  if (millis() - lastUpdate < 50) return;
  lastUpdate = millis();

  // Sweep all servos together
  for (uint8_t ch = 0; ch < 16; ch++) {
    servoBus.writeDegrees(ch, position);
  }

  // Print position every 30 degrees
  static float lastPrint = 0;
  if (abs(position - lastPrint) >= 30.0) {
    Serial.print("[SWEEP] Position: ");
    Serial.print(position);
    Serial.println("°");
    lastPrint = position;
  }

  // Update position
  position += direction * 2.0;

  // Reverse at limits
  if (position >= 170.0) {
    position = 170.0;
    direction = -1;
    Serial.println("[SWEEP] ⟲ Reversing (max)");
  } else if (position <= 10.0) {
    position = 10.0;
    direction = 1;
    Serial.println("[SWEEP] ⟳ Reversing (min)");
  }
}
