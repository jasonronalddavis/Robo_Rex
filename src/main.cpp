// src/main.cpp - 15-SERVO CONFIGURATION
// Robo Rex - Complete T-Rex control with PCA9685
// ESP32-S3 Freenove WROOM board

#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>

// Servo control via PCA9685
#include "ServoBus.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Pelvis_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Leg_Function.h"

// ========== I2C Pin Configuration ==========
#define I2C_SDA 8   // GPIO 8 for SDA
#define I2C_SCL 9   // GPIO 9 for SCL

// ========== PCA9685 Configuration ==========
#define PCA9685_ADDR 0x40    // Default I2C address
#define PCA9685_FREQ 50.0f   // 50 Hz for servos

// ========== BLE UUIDs (Nordic UART Service) ==========
static const char* NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
static const char* NUS_RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

// ========== Global Variables ==========
static NimBLECharacteristic* g_txChar = nullptr;
static String g_lineBuf;
static ServoBus servoBus;  // PCA9685 servo controller

// ========== Helper Functions ==========
static inline void txNotify(const char* s) {
  if (!g_txChar || !s) return;
  g_txChar->setValue((uint8_t*)s, strlen(s));
  g_txChar->notify();
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
  else if (line == "STATUS") {
    Serial.println(F("[CMD] System Status:"));
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
  
  NimBLEDevice::init("Robo_Rex_15Servo");
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

  Serial.println(F("[BLE] ✓ Advertising as 'Robo_Rex_15Servo'"));
}

// ========== Arduino Setup ==========
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════════╗"));
  Serial.println(F("║                                            ║"));
  Serial.println(F("║    ROBO REX - 15 SERVO CONFIGURATION       ║"));
  Serial.println(F("║         PCA9685 Control Mode               ║"));
  Serial.println(F("║                                            ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialize I2C
  Serial.println(F("[I2C] Initializing..."));
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  Serial.println(F("[I2C] ✓ Ready at 100kHz"));
  
  // Initialize PCA9685
  Serial.println(F("\n[PCA9685] Initializing..."));
  servoBus.begin(PCA9685_ADDR, PCA9685_FREQ);
  Serial.println(F("[PCA9685] ✓ Ready"));
  
  // Initialize servo functions
  Serial.println(F("\n[Servos] Initializing 15 servos..."));
  
  // Neck (1 servo - channel 0)
  Neck::Map neckMap;
  neckMap.jaw = 0;
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
  legMap.R_hipX  = 6;
  legMap.R_hipY  = 7;
  legMap.R_knee  = 8;
  legMap.R_ankle = 9;
  legMap.R_foot  = 10;
  // Left leg
  legMap.L_hipX  = 11;
  legMap.L_hipY  = 12;
  legMap.L_knee  = 13;
  legMap.L_ankle = 14;
  legMap.L_foot  = 15;
  Leg::begin(&servoBus, legMap);
  
  Serial.println(F("[Servos] ✓ All 15 servos initialized"));
  
  // Initialize BLE
  setupBLE();
  
  // Ready!
  Serial.println(F("\n╔════════════════════════════════════════════╗"));
  Serial.println(F("║           SYSTEM READY                     ║"));
  Serial.println(F("╚════════════════════════════════════════════╝"));
  Serial.println(F("\n15 servos active on channels 0-15"));
  Serial.println(F("Type HELP for command list\n"));
  
  txNotify("Robo_Rex Ready!\n");
}

// ========== Arduino Loop ==========
void loop() {
  // Update walking gait (MUST be called every loop!)
  Leg::tick();
  
  delay(20);  // 50 Hz update rate
}