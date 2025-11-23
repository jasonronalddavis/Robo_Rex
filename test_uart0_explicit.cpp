// EXPLICIT UART0 TEST - Force use of hardware UART0
// ESP32-S3 + CH340 should use UART0 on default pins
// This bypasses Arduino Serial abstraction

#include <Arduino.h>
#include <HardwareSerial.h>

// Force use of UART0 (what CH340 connects to)
HardwareSerial SerialPort(0);  // UART0

void setup() {
  // Initialize UART0 explicitly with default pins
  // For ESP32-S3: TX=43, RX=44 (typical defaults)
  // But we'll let Arduino pick the right ones for this board
  SerialPort.begin(115200, SERIAL_8N1);

  delay(3000);  // Long delay to ensure everything is ready

  // Send messages forever from setup
  for(int i = 0; i < 100; i++) {
    SerialPort.println("========================================");
    SerialPort.print("UART0 TEST - Message ");
    SerialPort.println(i);
    SerialPort.println("If you see this, UART0 is working!");
    SerialPort.println("ESP32-S3 Freenove WROOM");
    SerialPort.println("========================================");
    SerialPort.println();
    delay(500);
  }
}

void loop() {
  SerialPort.println("Loop running - UART0 active");
  delay(1000);
}
