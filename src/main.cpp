// src/main.cpp
#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

// Servo bus + body modules (optional to connect later)
#include "ServoBus.h"
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// Command router (expects: CommandRouter::handleLine(const String&))
#include "CommandRouter.h"

// ---------------- BLE UUIDs (Nordic UART-like) ----------------
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // notify
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // write

// ---------------- Globals ----------------
static NimBLECharacteristic* g_txChar = nullptr;
static String                g_lineBuf;      // accumulate until '\n'
static ServoBus              SERVO;

// ---------------- Helpers ----------------
static inline void txNotify(const std::string& s) {
  if (!g_txChar) return;
  g_txChar->setValue((uint8_t*)s.data(), s.size());
  g_txChar->notify();
}

static inline void txNotifyStr(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}

static void handleLineFromWeb(const String& line) {
  if (line.length() == 0) return;

  // Log to USB serial for debugging
  Serial.print(F("[WEB ▶ MCU] "));
  Serial.println(line);

  // Route to your command router
  CommandRouter::handleLine(line);
}

// ---------------- BLE Callbacks ----------------
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    const std::string& v = c->getValue();
    if (v.empty()) return;

    // Append to line buffer; process on '\n'
    for (char ch : v) {
      if (ch == '\r') continue;
      if (ch != '\n') {
        g_lineBuf += ch;
      } else {
        String line = g_lineBuf;
        g_lineBuf = "";
        handleLineFromWeb(line);
      }
    }
    // No trailing newline? leave in buffer until next packet
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    Serial.println(F("[BLE] Central connected"));
  }
  void onDisconnect(NimBLEServer*) override {
    Serial.println(F("[BLE] Central disconnected"));
    NimBLEDevice::startAdvertising();
  }
};

// ---------------- Setup PCA9685 & body maps (optional) ----------------
static void setupServosAndModules() {
  // Bring up I2C on default board pins (DON'T force pins—prevents driver asserts)
  Wire.begin();            // use board defaults (safe on S3 boards)
  Wire.setClock(400000);   // fast I2C is fine for PCA9685

  // PCA9685 @ 0x40, 50 Hz for hobby servos
  if (!SERVO.begin(0x40, 50.0f)) {
    Serial.println(F("! SERVO.begin failed (PCA9685)"));
  } else {
    Serial.println(F("SERVO ready (PCA9685 @ 0x40, 50Hz)"));
  }

  // Example channel maps (adjust as your wiring evolves)
  Leg::Map legMap;
  legMap.L_hip   = 0;  legMap.L_knee  = 1;  legMap.L_ankle = 2;  legMap.L_foot = 3;  legMap.L_toe = 4;
  legMap.R_hip   = 5;  legMap.R_knee  = 6;  legMap.R_ankle = 7;  legMap.R_foot = 8;  legMap.R_toe = 9;

  Spine::Map  spineMap;  spineMap.spinePitch = 10;
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch     = 12;   // adjust if separate
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;

  // Initialize modules (safe even if no servos are plugged in)
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  Serial.println(F("Body modules initialized"));
}

// ---------------- Setup BLE (NimBLE) ----------------
static void setupBLE() {
  NimBLEDevice::init("Robo_Rex_ESP32S3");
  // Keep conservative while debugging; raise later if you like
  NimBLEDevice::setMTU(69);
  // Power call is optional; some SDK combos are picky. Safe to omit or use P9.
  // NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  auto* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  auto* svc = server->createService(NUS_SERVICE_UUID);
  g_txChar  = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  auto* rx = svc->createCharacteristic(
      NUS_RX_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  rx->setCallbacks(new RxCallbacks());

  svc->start();

  auto* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  Serial.println(F("[BLE] Advertising as Robo_Rex_ESP32S3 (NUS)"));
}

// ---------------- Arduino lifecycle ----------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Robo Rex (BLE bring-up, no IMU) ==="));

  // 1) Servos/I2C first (stable defaults to avoid driver asserts)
  setupServosAndModules();

  // 2) BLE last
  setupBLE();

  // Say hello to the web app (optional)
  txNotifyStr("MCU online\n");
}

void loop() {
  // Keep leg engine ticking even if idle (no-op until commanded)
  Leg::tick();

  // Keep the loop light
  delay(5);
}
