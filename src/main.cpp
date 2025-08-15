#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

#include "ServoBus.h"
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"
#include "CommandRouter.h"

// ── BLE UUIDs (Nordic UART) ──────────────────────────────────────────
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // write
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // notify

// ── Globals ───────────────────────────────────────────────────────────
static NimBLECharacteristic* g_txChar = nullptr;   // notify up to web
static String                g_lineBuf;            // accumulates until '\n'
static ServoBus              SERVO;

// Keep maps in scope (handy for future debug)
static Leg::Map g_legMap;

// ── Helpers ───────────────────────────────────────────────────────────
static inline void txNotify(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}
static inline void txNotify(const String& s) { txNotify(s.c_str()); }

static void handleLineFromWeb(const String& line) {
  if (line.length() == 0) return;

  // Log to USB for visibility
  Serial.print(F("[WEB ▶ MCU] "));
  Serial.println(line);

  // Optional echo back to the browser (handy in DevTools console)
  txNotify(String("RX OK: ") + line + "\n");

  // Route to your command router (supports new {target,part,command,phase}
  // and legacy {"cmd":"rex_*"} forms you already implemented)
  CommandRouter::handleLine(line);
}

// ── BLE Callbacks ─────────────────────────────────────────────────────
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    const std::string& v = c->getValue();
    if (v.empty()) return;

    // Accumulate until newline so partial packets don't break parsing
    for (char ch : v) {
      if (ch == '\r') continue;
      if (ch != '\n') g_lineBuf += ch;
      else { String line = g_lineBuf; g_lineBuf = ""; handleLineFromWeb(line); }
    }
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override { Serial.println(F("[BLE] Central connected")); }
  void onDisconnect(NimBLEServer*) override {
    Serial.println(F("[BLE] Central disconnected"));
    NimBLEDevice::startAdvertising();
  }
};

// ── Servo/I²C + module bring‑up ───────────────────────────────────────
static void setupServosAndModules() {
  Wire.begin();          // board defaults (ESP32‑S3)
  Wire.setClock(400000); // 400 kHz I²C for PCA9685

  if (!SERVO.begin(0x40, 50.0f)) {
    Serial.println(F("! SERVO.begin failed (PCA9685)"));
  } else {
    Serial.println(F("SERVO ready (PCA9685 @ 0x40, 50Hz)"));
  }

  // Channel maps (adjust to your wiring)
  g_legMap.L_hip   = 0;  g_legMap.L_knee  = 1;  g_legMap.L_ankle = 2;  g_legMap.L_foot = 3;  g_legMap.L_toe = 4;
  g_legMap.R_hip   = 5;  g_legMap.R_knee  = 6;  g_legMap.R_ankle = 7;  g_legMap.R_foot = 8;  g_legMap.R_toe = 9;

  Spine::Map  spineMap;  spineMap.spinePitch = 10;
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch     = 12;   // adjust if separate
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;

  // Initialize modules (attaches channels & writes neutral)
  Leg::begin(&SERVO, g_legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  Serial.println(F("Body modules initialized"));
}

// ── BLE setup ─────────────────────────────────────────────────────────
static void setupBLE() {
  NimBLEDevice::init("Robo_Rex_ESP32S3");
  NimBLEDevice::setMTU(69);  // conservative, reliable

  auto* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  auto* svc = server->createService(NUS_SERVICE_UUID);

  // TX: browser subscribes to this
  g_txChar = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  // RX: browser writes control lines here
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

// ── Arduino lifecycle ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Robo Rex (BLE command test) ==="));

  setupServosAndModules();
  setupBLE();

  txNotify("MCU online (ready for BLE commands)\n");
}

void loop() {
  // Keep leg engine ticking even if idle; router may switch modes
  Leg::tick();
  delay(5);
}
