// src/main.cpp - 15-SERVO CONFIGURATION
// Robo Rex - Complete T-Rex control with ESP32-S3 GPIO
// ESP32-S3 Freenove WROOM board

#include <Arduino.h>
#include <NimBLEDevice.h>

// Servo control via ESP32 GPIO
#include "ServoBus.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Pelvis_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Leg_Function.h"

// ========== BLE UUIDs (Nordic UART Service) ==========
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

// ========== Global Variables ==========
static NimBLECharacteristic* g_txChar = nullptr;
static String g_lineBuf;
static ServoBus servoBus;  // ESP32 GPIO servo controller

// ========== Sweep Test Configuration ==========
// Set to true to enable continuous sweep test (bypasses normal servo control)
// Set to false for normal operation
#define ENABLE_SWEEP_TEST false

struct Sweeper {
  bool     enabled     = ENABLE_SWEEP_TEST;
  uint32_t lastMs      = 0;
  uint16_t intervalMs  = 15;     // update cadence (15ms = smooth sweep)
  float    minDeg      = 10.0f;  // minimum angle (avoid mechanical limits)
  float    maxDeg      = 170.0f; // maximum angle (avoid mechanical limits)
  float    stepDeg     = 1.0f;   // degrees per step
  float    posDeg      = 10.0f;  // current position
  int8_t   dir         = +1;     // direction: +1 or -1
} g_sweep;

// ========== Helper Functions ==========
static inline void txNotify(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
}

// All 16 servo channels (0-15)
static const uint8_t kAllCh[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// ========== Sweep Test Functions ==========
// Call this every loop (non-blocking)
static inline void sweepAllTick() {
  if (!g_sweep.enabled) return;

  const uint32_t now = millis();
  if (now - g_sweep.lastMs < g_sweep.intervalMs) return;
  g_sweep.lastMs = now;

  // Write the current angle to all channels
  for (uint8_t ch : kAllCh) {
    servoBus.writeDegrees(ch, g_sweep.posDeg);
  }

  // Print current position every 10 degrees for monitoring
  static float lastPrintPos = 0;
  if (abs(g_sweep.posDeg - lastPrintPos) >= 10.0f) {
    Serial.print(F("[SWEEP] Position: "));
    Serial.print(g_sweep.posDeg);
    Serial.println(F("°"));
    lastPrintPos = g_sweep.posDeg;
  }

  // Advance position and bounce at the edges
  g_sweep.posDeg += g_sweep.dir * g_sweep.stepDeg;
  if (g_sweep.posDeg >= g_sweep.maxDeg) { 
    g_sweep.posDeg = g_sweep.maxDeg; 
    g_sweep.dir = -1;
    Serial.println(F("[SWEEP] ⟲ Reversing direction (max reached)"));
  }
  if (g_sweep.posDeg <= g_sweep.minDeg) { 
    g_sweep.posDeg = g_sweep.minDeg; 
    g_sweep.dir = +1;
    Serial.println(F("[SWEEP] ⟳ Reversing direction (min reached)"));
  }
}

// ========== Command Parser ==========
static void handleCommand(const String& line) {
  if (!line.length()) return;
  
  Serial.print(F("[CMD] RX: "));
  Serial.println(line);
  
  // ===== Neck Commands =====
  if (line == "LOOK_LEFT") {
    Neck::lookLeft(1.0);
    txNotify("Looking left\n");
  }
  else if (line == "LOOK_RIGHT") {
    Neck::lookRight(1.0);
    txNotify("Looking right\n");
  }
  else if (line == "LOOK_CENTER") {
    Neck::center();
    txNotify("Looking center\n");
  }
  
  // ===== Head/Jaw Commands =====
  else if (line == "JAW_OPEN") {
    Head::mouthOpen();
    txNotify("Jaw opened\n");
  }
  else if (line == "JAW_CLOSE") {
    Head::mouthClose();
    txNotify("Jaw closed\n");
  }
  else if (line == "ROAR") {
    Head::roar();
    txNotify("ROAR!\n");
  }
  else if (line == "SNAP") {
    Head::snap();
    txNotify("Snap!\n");
  }
  else if (line == "HEAD_UP") {
    Head::lookUp(1.0);
    txNotify("Head up\n");
  }
  else if (line == "HEAD_DOWN") {
    Head::lookDown(1.0);
    txNotify("Head down\n");
  }
  
  // ===== Pelvis Commands =====
  else if (line == "PELVIS_LEFT") {
    Pelvis::setRoll01(0.0);
    txNotify("Pelvis left\n");
  }
  else if (line == "PELVIS_RIGHT") {
    Pelvis::setRoll01(1.0);
    txNotify("Pelvis right\n");
  }
  else if (line == "PELVIS_CENTER") {
    Pelvis::center();
    txNotify("Pelvis centered\n");
  }
  
  // ===== Spine Commands =====
  else if (line == "SPINE_LEFT") {
    Spine::left();
    txNotify("Spine twisted left\n");
  }
  else if (line == "SPINE_RIGHT") {
    Spine::right();
    txNotify("Spine twisted right\n");
  }
  else if (line == "SPINE_CENTER") {
    Spine::center();
    txNotify("Spine centered\n");
  }
  
  // ===== Tail Commands =====
  else if (line == "TAIL_WAG") {
    Tail::wag();
    txNotify("Tail wagging!\n");
  }
  else if (line == "TAIL_CENTER") {
    Tail::center();
    txNotify("Tail centered\n");
  }
  
  // ===== Leg/Walking Commands =====
  else if (line == "WALK_FORWARD") {
    Leg::walkForward(1.0);
    txNotify("Walking forward\n");
  }
  else if (line == "WALK_BACKWARD") {
    Leg::walkBackward(1.0);
    txNotify("Walking backward\n");
  }
  else if (line == "TURN_LEFT") {
    Leg::turnLeft(0.8);
    txNotify("Turning left\n");
  }
  else if (line == "TURN_RIGHT") {
    Leg::turnRight(0.8);
    txNotify("Turning right\n");
  }
  else if (line == "STOP") {
    Leg::stop();
    txNotify("Stopped\n");
  }
  
  // ===== System Commands =====
  else if (line == "CENTER_ALL") {
    Neck::center();
    Head::center();
    Pelvis::center();
    Spine::center();
    Tail::center();
    Leg::stop();
    txNotify("All centered\n");
  }
  else if (line == "ALL_OFF") {
    servoBus.setAllOff();
    txNotify("All off\n");
  }
  else if (line == "SWEEP_ON") {
    g_sweep.enabled = true;
    g_sweep.posDeg = g_sweep.minDeg;
    g_sweep.dir = +1;
    Serial.println(F("[CMD] ✓ Sweep test ENABLED"));
    txNotify("Sweep test ON\n");
  }
  else if (line == "SWEEP_OFF") {
    g_sweep.enabled = false;
    Serial.println(F("[CMD] ✓ Sweep test DISABLED"));
    txNotify("Sweep test OFF\n");
  }
  else if (line == "STATUS") {
    Serial.println(F("[CMD] System Status:"));
    Serial.print(F("  Sweep enabled: "));
    Serial.println(g_sweep.enabled ? "YES" : "NO");
    if (g_sweep.enabled) {
      Serial.print(F("  Sweep position: "));
      Serial.print(g_sweep.posDeg);
      Serial.println(F("°"));
    }
    Serial.print(F("  Leg mode: "));
    Serial.println(Leg::mode());
    Serial.print(F("  Speed: "));
    Serial.print(Leg::speedHz());
    Serial.println(F(" Hz"));
    txNotify("Status OK\n");
  }
  else if (line == "HELP") {
    Serial.println(F("\n[CMD] Available Commands:"));
    Serial.println(F("  Neck:   LOOK_LEFT, LOOK_RIGHT, LOOK_CENTER"));
    Serial.println(F("  Head:   JAW_OPEN, JAW_CLOSE, ROAR, SNAP"));
    Serial.println(F("          HEAD_UP, HEAD_DOWN"));
    Serial.println(F("  Pelvis: PELVIS_LEFT, PELVIS_RIGHT, PELVIS_CENTER"));
    Serial.println(F("  Spine:  SPINE_LEFT, SPINE_RIGHT, SPINE_CENTER"));
    Serial.println(F("  Tail:   TAIL_WAG, TAIL_CENTER"));
    Serial.println(F("  Legs:   WALK_FORWARD, WALK_BACKWARD"));
    Serial.println(F("          TURN_LEFT, TURN_RIGHT, STOP"));
    Serial.println(F("  System: CENTER_ALL, ALL_OFF, STATUS, HELP"));
    Serial.println(F("  Test:   SWEEP_ON, SWEEP_OFF"));
    txNotify("Help sent to serial\n");
  }
  
  // ===== Unknown Command =====
  else {
    Serial.print(F("[CMD] ✗ Unknown: "));
    Serial.println(line);
    txNotify("Unknown command\n");
  }
}

// ========== BLE Callbacks ==========
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    const std::string v = c->getValue();
    for (char ch : v) {
      if (ch == '\r') continue;
      if (ch != '\n') {
        g_lineBuf += ch;
      } else {
        String line = g_lineBuf;
        line.trim();
        g_lineBuf = "";
        if (line.length() > 0) {
          handleCommand(line);
        }
      }
    }
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override { 
    Serial.println(F("[BLE] ✓ Client connected")); 
    txNotify("Connected to Robo_Rex\n");
  }
  void onDisconnect(NimBLEServer*) override { 
    Serial.println(F("[BLE] ✗ Client disconnected")); 
    NimBLEDevice::startAdvertising(); 
  }
};

// ========== BLE Initialization ==========
static void setupBLE() {
  Serial.println(F("\n[BLE] Initializing Bluetooth..."));
  
  NimBLEDevice::init("Robo_Rex_GPIO");
  NimBLEDevice::setMTU(69);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = server->createService(NUS_SERVICE_UUID);
  
  g_txChar = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
  NimBLECharacteristic* rx = svc->createCharacteristic(
    NUS_RX_UUID, 
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  rx->setCallbacks(new RxCallbacks());

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  Serial.println(F("[BLE] ✓ Advertising as 'Robo_Rex_GPIO'"));
}

// ========== Arduino Setup ==========
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║                                            ║"));
  Serial.println(F("║    ROBO REX - 15 SERVO CONFIGURATION       ║"));
  Serial.println(F("║         ESP32-S3 GPIO Control Mode         ║"));
  Serial.println(F("║                                            ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialize GPIO servo system
  Serial.println(F("[GPIO] Initializing servo system..."));
  servoBus.begin();
  Serial.println(F("[GPIO] ✓ Ready"));
  
  // Initialize servo functions
  Serial.println(F("\n[Servos] Initializing 15 servos..."));
  
  // Neck (1 servo - channel 0)
  Neck::Map neckMap;
  neckMap.yaw = 0;
  Neck::begin(&servoBus, neckMap);
  
  // Head (2 servos - channels 1-2)
  Head::Map headMap;
  headMap.jaw = 1;
  headMap.pitch = 2;
  Head::begin(&servoBus, headMap);
  
  // Pelvis (1 servo - channel 3)
  Pelvis::Map pelvisMap;
  pelvisMap.roll = 3;
  Pelvis::begin(&servoBus, pelvisMap);
  
  // Spine (1 servo - channel 4)
  Spine::Map spineMap;
  spineMap.spineYaw = 4;
  Spine::begin(&servoBus, spineMap);
  
  // Tail (1 servo - channel 5)
  Tail::Map tailMap;
  tailMap.wag = 5;
  Tail::begin(&servoBus, tailMap);
  
  // Legs (10 servos - channels 6-15)
  Leg::Map legMap;
  // Right leg
  legMap.R_hipX  = 7;
  legMap.R_hipY  = 10;
  legMap.R_knee  = 11;
  legMap.R_ankle = 12;
  legMap.R_foot  = 13;
  // Left leg
  legMap.L_hipX  = 14;
  legMap.L_hipY  = 15;
  legMap.L_knee  = 16;
  legMap.L_ankle = 17;
  legMap.L_foot  = 18;
  Leg::begin(&servoBus, legMap);
  
  Serial.println(F("[Servos] ✓ All 15 servos initialized"));
  
  // Show sweep test status
  if (g_sweep.enabled) {
    Serial.println();
    Serial.println(F("╔════════════════════════════════════════════╗"));
    Serial.println(F("║  ⚠️  SWEEP TEST MODE ENABLED  ⚠️           ║"));
    Serial.println(F("║                                            ║"));
    Serial.println(F("║  All servos will sweep 10° - 170°         ║"));
    Serial.println(F("║  Send 'SWEEP_OFF' to disable              ║"));
    Serial.println(F("╚════════════════════════════════════════════╝"));
  }
  
  // Initialize BLE
  setupBLE();
  
  // Ready!
  Serial.println(F("\n╔════════════════════════════════════════════╗"));
  Serial.println(F("║           SYSTEM READY                     ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println(F("\nESP32-S3 GPIO control mode active"));
  Serial.println(F("15 servos on channels 0-15"));
  Serial.println(F("Type HELP for command list\n"));
  
  txNotify("Robo_Rex Ready!\n");
}

// ========== Arduino Loop ==========
void loop() {
  // Run sweep test if enabled (takes priority)
  if (g_sweep.enabled) {
    sweepAllTick();
  } else {
    // Normal operation - update walking gait
    Leg::tick();
  }
  
  delay(20);  // 50 Hz update rate
}