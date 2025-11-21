// I2C Scanner - Detects all I2C devices on the bus
// Upload this to diagnose PCA9685 communication issues

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== I2C SCANNER ===");
  Serial.println("Testing multiple I2C pin configurations...\n");

  // Test configuration 1: GPIO 4/5
  Serial.println("Testing GPIO 4 (SDA) / GPIO 5 (SCL)...");
  Wire.begin(4, 5);
  Wire.setClock(100000);
  scanI2C();

  // Test configuration 2: GPIO 10/11
  Serial.println("\nTesting GPIO 10 (SDA) / GPIO 11 (SCL)...");
  Wire.begin(10, 11);
  Wire.setClock(100000);
  scanI2C();

  // Test configuration 3: GPIO 21/22 (default)
  Serial.println("\nTesting GPIO 21 (SDA) / GPIO 22 (SCL)...");
  Wire.begin(21, 22);
  Wire.setClock(100000);
  scanI2C();

  Serial.println("\n=== SCAN COMPLETE ===");
  Serial.println("If PCA9685 not found:");
  Serial.println("1. Check VCC = 3.3V");
  Serial.println("2. Check V+ has external 5-6V power");
  Serial.println("3. Check SDA/SCL wiring");
  Serial.println("4. Check address jumpers (A0-A5)");
}

void scanI2C() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning addresses 0x00-0x7F...");

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  Device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);

      // Identify known devices
      if (address == 0x40 || address == 0x41) {
        Serial.print(" - PCA9685 Servo Driver");
      } else if (address == 0x68) {
        Serial.print(" - MPU6050 IMU");
      }
      Serial.println();
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("  Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("  ** NO I2C DEVICES FOUND **");
  } else {
    Serial.print("  Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)");
  }
}

void loop() {
  // Do nothing
}
