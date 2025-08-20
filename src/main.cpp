// src/main.cpp
#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

#include "ServoBus.h"
#include "CommandRouter.h"

// Body modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// IMU (MPU6050 + Madgwick)
#include "IMU.h"

// ---------- Build-time I2C pin config ----------
// Servos (PCA9685) on Wire  bus: GPIO 8/9
// IMU (MPU6050)    on Wire1 bus: GPIO 19/20 (separate bus for stability)
#ifndef SERVO_SDA_PIN
#define SERVO_SDA_PIN 8
#endif
#ifndef SERVO_SCL_PIN
#define SERVO_SCL_PIN 9
#endif

// IMU pins defined in IMU.h (19/20 by default)

// ---------- BLE UUIDs (Nordic UART compatible) ----------
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // notify
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // write

// ---------- Globals ----------
static NimBLECharacteristic* g_txChar = nullptr;
static String                g_lineBuf;
static ServoBus              SERVO;

// ---------- Helpers ----------
static inline void txNotify(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}

static void handleLineFromWeb(const String& line) {
  if (!line.length()) return;
  Serial.print(F("[Router] RX: "));
  Serial.println(line);
  // Your router signature in this repo:
  CommandRouter::handleLine(line);
}

// ---------- BLE Callbacks ----------
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    const std::string v = c->getValue();
    for (char ch : v) {
      if (ch == '\r') continue;
      if (ch != '\n') g_lineBuf += ch;
      else {
        String line = g_lineBuf;
        g_lineBuf = "";
        handleLineFromWeb(line);
      }
    }
  }
};
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override    { Serial.println(F("[BLE] Client connected")); }
  void onDisconnect(NimBLEServer*) override { Serial.println(F("[BLE] Client disconnected")); NimBLEDevice::startAdvertising(); }
};

// ---------- I2C scanner for Wire bus (servos) ----------
static void scanServoI2C() {
  Serial.println(F("[I2C] Scanning Wire bus (servos)..."));
  byte err;
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  Found device at 0x%02X\n", addr);
      found++;
    }
  }
  if (!found) Serial.println(F("  No devices found on Wire bus."));
  else Serial.printf("  Found %d device(s) on Wire bus\n", found);
  Serial.println();
}

// ---------- I2C scanner for Wire1 bus (IMU) ----------
static void scanIMUI2C() {
  Serial.println(F("[I2C] Scanning Wire1 bus (IMU)..."));
  byte err;
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    err = Wire1.endTransmission();
    if (err == 0) {
      Serial.printf("  Found device at 0x%02X\n", addr);
      found++;
    }
  }
  if (!found) Serial.println(F("  No devices found on Wire1 bus."));
  else Serial.printf("  Found %d device(s) on Wire1 bus\n", found);
  Serial.println();
}

// ---------- Debug MPU6050 directly ----------
static void testMPU6050Directly() {
  Serial.println(F("\n[DEBUG] Testing MPU6050 directly..."));
  
  // Test basic I2C communication
  Wire1.beginTransmission(0x68);
  byte error = Wire1.endTransmission();
  Serial.printf("[DEBUG] I2C test result: %d (0=success)\n", error);
  
  // Try to read WHO_AM_I register (0x75)
  Wire1.beginTransmission(0x68);
  Wire1.write(0x75);  // WHO_AM_I register
  error = Wire1.endTransmission();
  
  if (error == 0) {
    Wire1.requestFrom(0x68, 1);
    if (Wire1.available()) {
      byte whoami = Wire1.read();
      Serial.printf("[DEBUG] WHO_AM_I: 0x%02X (expected: 0x68)\n", whoami);
    } else {
      Serial.println(F("[DEBUG] No data available from WHO_AM_I"));
    }
  } else {
    Serial.printf("[DEBUG] WHO_AM_I read failed: %d\n", error);
  }
  
  // Try different MPU6050 addresses
  Serial.println(F("[DEBUG] Trying different addresses..."));
  for (uint8_t addr = 0x68; addr <= 0x69; addr++) {
    Wire1.beginTransmission(addr);
    error = Wire1.endTransmission();
    Serial.printf("[DEBUG] Address 0x%02X: %s\n", addr, error == 0 ? "ACK" : "NACK");
  }
  
  // Try to read Power Management register (0x6B)
  Serial.println(F("[DEBUG] Reading Power Management register..."));
  Wire1.beginTransmission(0x68);
  Wire1.write(0x6B);  // PWR_MGMT_1 register
  error = Wire1.endTransmission();
  
  if (error == 0) {
    Wire1.requestFrom(0x68, 1);
    if (Wire1.available()) {
      byte pwr_mgmt = Wire1.read();
      Serial.printf("[DEBUG] PWR_MGMT_1: 0x%02X (0x40=sleep, 0x00=awake)\n", pwr_mgmt);
      
      // If in sleep mode, try to wake it up
      if (pwr_mgmt & 0x40) {
        Serial.println(F("[DEBUG] Device is in sleep mode, attempting wake-up..."));
        Wire1.beginTransmission(0x68);
        Wire1.write(0x6B);  // PWR_MGMT_1 register
        Wire1.write(0x00);  // Wake up device
        error = Wire1.endTransmission();
        Serial.printf("[DEBUG] Wake-up result: %d\n", error);
        delay(100);
        
        // Read again to confirm
        Wire1.beginTransmission(0x68);
        Wire1.write(0x6B);
        Wire1.endTransmission();
        Wire1.requestFrom(0x68, 1);
        if (Wire1.available()) {
          pwr_mgmt = Wire1.read();
          Serial.printf("[DEBUG] PWR_MGMT_1 after wake: 0x%02X\n", pwr_mgmt);
        }
      }
    } else {
      Serial.println(F("[DEBUG] No data available from PWR_MGMT_1"));
    }
  } else {
    Serial.printf("[DEBUG] PWR_MGMT_1 read failed: %d\n", error);
  }
  
  Serial.println(F("[DEBUG] MPU6050 direct test complete\n"));
}

// ---------- Servo + module bring-up ----------
static void setupServosAndModules() {
  // PCA9685 @0x40, 50Hz on Wire bus
  if (!SERVO.begin(0x40, 50.0f)) {
    Serial.println(F("[ServoBus] Init FAILED (PCA9685)"));
    return;
  } else {
    Serial.println(F("[ServoBus] Ready (PCA9685 @0x40, 50Hz on Wire)"));
  }

  // Channel maps — adjust to your wiring
  Leg::Map legMap;
  legMap.L_hip   = 0;  legMap.L_knee  = 1;  legMap.L_ankle = 2;  legMap.L_foot = 3;  legMap.L_toe = 4;
  legMap.R_hip   = 5;  legMap.R_knee  = 6;  legMap.R_ankle = 7;  legMap.R_foot = 8;  legMap.R_toe = 9;

  Spine::Map  spineMap;  spineMap.spinePitch = 10;
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch     = 12; // adjust if separate
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;

  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // Center all attached channels once on boot (no sweep)
  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (SERVO.isAttached(ch)) SERVO.writeNeutral(ch);
  }
  Serial.println(F("[ServoBus] All attached channels centered (90°)."));
}

// ---------- BLE bring-up ----------
static void setupBLE() {
  NimBLEDevice::init("Robo_Rex_ESP32S3");
  NimBLEDevice::setMTU(69);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = server->createService(NUS_SERVICE_UUID);
  g_txChar = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  NimBLECharacteristic* rx = svc->createCharacteristic(
    NUS_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  rx->setCallbacks(new RxCallbacks());

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  Serial.println(F("[BLE] Advertising as Robo_Rex_ESP32S3 (NUS)"));
}

// ---------- Arduino lifecycle ----------
void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println(F("=== Robo Rex (Freenove S3 | Dual I2C: Wire=Servos, Wire1=IMU) ==="));

  // 1) Initialize Wire bus for servos (GPIO 8/9) - slower, stable
  Serial.println(F("[I2C] Initializing Wire for servos..."));
  Wire.begin(SERVO_SDA_PIN, SERVO_SCL_PIN);
  Wire.setClock(100000);  // 100kHz - stable for servos
  delay(50);
  scanServoI2C();  // Should find 0x40 (PCA9685)

  // 2) Initialize IMU on separate Wire1 bus (GPIO 19/20) - faster, isolated
  //    Note: IMU::begin() handles Wire1 initialization internally
  Serial.println(F("[IMU] Initializing MPU6050 on dedicated Wire1 bus..."));
  if (IMU::begin()) {
    Serial.println(F("[IMU] MPU6050 initialized successfully on Wire1"));
    scanIMUI2C();  // Should find 0x68 (MPU6050)
    
    // Start the IMU background task
    IMU::startTask();
    Serial.println(F("[IMU] Background task started"));
  } else {
    Serial.println(F("[IMU] ERROR: MPU6050 initialization failed on Wire1!"));
    testMPU6050Directly();  // Debug the MPU6050 directly
    scanIMUI2C();  // Scan anyway to see what's on the bus
    // Continue anyway - servos might still work
  }

  // 3) Initialize servos on Wire bus (after IMU is stable on Wire1)
  Serial.println(F("[Servos] Initializing PCA9685 and servo modules on Wire..."));
  setupServosAndModules();

  // 4) BLE last
  Serial.println(F("[BLE] Initializing Bluetooth..."));
  setupBLE();

  Serial.println(F("[Setup] All systems initialized"));
  Serial.println(F("  Wire  (GPIO 8/9):  PCA9685 servos"));
  Serial.println(F("  Wire1 (GPIO 19/20): MPU6050 IMU"));
  
  txNotify("MCU online - Dual I2C buses ready\n");
}

void loop() {
  // Print IMU status every 5 seconds for debugging
  static uint32_t lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    IMU::printStatus();
    lastStatus = millis();
  }

  // IMU runs in its own task, leg animations run here
  Leg::tick();    // no motion unless commanded via BLE
  delay(10);
}