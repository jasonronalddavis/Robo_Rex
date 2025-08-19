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

// ---------- Build-time I2C pin config (shared by PCA9685 + MPU6050) ----------
#ifndef IMU_SDA_PIN
#define IMU_SDA_PIN 8
#endif
#ifndef IMU_SCL_PIN
#define IMU_SCL_PIN 9
#endif

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

// ---------- I2C scanner (debug aid) ----------
static void scanI2C() {
  Serial.println(F("[I2C] Scanning bus..."));
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
  if (!found) Serial.println(F("  No devices found."));
  Serial.println();
}

// ---------- Servo + module bring-up ----------
static void setupServosAndModules() {
  // PCA9685 @0x40, 50Hz
  if (!SERVO.begin(0x40, 50.0f)) {
    Serial.println(F("[ServoBus] Init FAILED (PCA9685)"));
  } else {
    Serial.println(F("[ServoBus] Ready (PCA9685 @0x40, 50Hz)"));
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
  Serial.println(F("=== Robo Rex (Freenove S3 | I2C on GPIO 8/9) ==="));

  // 1) I²C on pins 8/9 shared by PCA9685 + MPU6050
  Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
  Wire.setClock(400000);
  scanI2C();   // Expect 0x40 (PCA9685) and 0x68 (MPU6050)

  // 2) Servos
  setupServosAndModules();

  // 3) IMU (starts a background task; prints if IMU_DEBUG defined)
  IMU::begin();

  // 4) BLE
  setupBLE();

  txNotify("MCU online\n");
}

void loop() {
  // IMU runs in its own task after IMU::begin() and will print by itself if IMU_DEBUG
  Leg::tick();    // no motion unless commanded via BLE
  delay(10);
}
