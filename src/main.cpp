#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include <Wire.h>
#include "ServoBus.h"
#include "CommandRouter.h"

// Body modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// IMU
#include "IMU.h"

// -------- BLE UUIDs (Nordic UART-like) --------
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // notify
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // write

// -------- Globals --------
static NimBLECharacteristic* g_txChar = nullptr;
static String                g_lineBuf;
static ServoBus              SERVO;

// -------- Helpers --------
static inline void txNotifyStr(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}

static void handleLineFromWeb(const String& line) {
  if (!line.length()) return;
  Serial.print(F("[WEB ▶ MCU] "));
  Serial.println(line);
  CommandRouter::handleLine(line);
}

// -------- BLE Callbacks --------
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    const std::string& v = c->getValue();
    if (v.empty()) return;
    for (char ch : v) {
      if (ch == '\r') continue;
      if (ch != '\n') {
        g_lineBuf += ch;
      } else {
        String line = g_lineBuf; g_lineBuf = "";
        handleLineFromWeb(line);
      }
    }
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

// -------- Servo + modules setup --------
static void setupServosAndModules() {
  // PCA9685 @0x40, 50 Hz
  if (!SERVO.begin(0x40, 50.0f)) {
    Serial.println(F("! SERVO.begin failed (PCA9685)"));
  } else {
    Serial.println(F("SERVO ready (PCA9685 @ 0x40, 50Hz)"));
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

  // Initialize modules (safe even if some channels are unconnected)
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // Center everything once
  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (SERVO.isAttached(ch)) SERVO.writeNeutral(ch);
  }
  Serial.println(F("Body modules initialized (neutralized)"));
}

// -------- BLE setup --------
static void setupBLE() {
  NimBLEDevice::init("Robo_Rex_ESP32S3");
  NimBLEDevice::setMTU(69);
  // NimBLEDevice::setPower(ESP_PWR_LVL_P9); // optional

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = server->createService(NUS_SERVICE_UUID);
  g_txChar = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  auto* rx = svc->createCharacteristic(
    NUS_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  rx->setCallbacks(new RxCallbacks());
  svc->start();

  auto* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  Serial.println(F("[BLE] Advertising as Robo_Rex_ESP32S3 (NUS)"));
}

// -------- Arduino lifecycle --------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Robo Rex (BLE + IMU) ==="));

  // 1) Bring up I2C ONCE on your chosen pins
  Wire.begin(19, 20);          // SDA=19, SCL=20
  Wire.setClock(400000);       // 400 kHz

  // 2) Now it's safe to init modules that use I2C
  SERVO.begin(0x40, 50.0f);    // NOTE: ServoBus.begin no longer calls Wire.begin()
  // ... attach servos / neutral as before ...

  // 3) IMU after I2C is up
  if (IMU::begin()) {
    Serial.println(F("[IMU] begin OK"));
    IMU::startTask(200);
  } else {
    Serial.println(F("[IMU] begin FAILED"));
  }

  // 4) BLE etc...
  setupBLE();
}

void loop() {
  Leg::tick();
  // read IMU periodically for debug
  static uint32_t last = 0;
  if (millis() - last > 500) {
    last = millis();
    ImuState s; IMU::get(s);
    Serial.printf("[IMU] roll=%.1f pitch=%.1f yaw=%.1f healthy=%d\n",
                  s.roll_deg, s.pitch_deg, s.yaw_deg, s.healthy);
  }
  delay(5);
}
