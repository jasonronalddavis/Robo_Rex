// src/main.cpp - 16-SERVO HYBRID CONFIGURATION
// Robo Rex - Complete T-Rex control with ESP32-S3
// Hybrid Mode: GPIO (channels 0-5) + PCA9685 (channels 6-15)
// ESP32-S3 Freenove WROOM board

#include <Arduino.h>

// Servo control via ESP32 GPIO
#include "ServoBus.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Pelvis_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Leg_Function.h"

// ========== Global Variables ==========
static ServoBus servoBus;  // ESP32 GPIO servo controller
static String g_cmdBuffer;   // Serial command buffer

// ========== Sweep Test Configuration ==========
#define ENABLE_SWEEP_TEST true

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

// All 16 servo channels (0-15)
static const uint8_t kAllCh[16] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// ========== Sweep Test Functions ==========
static inline void sweepAllTick() {
  if (!g_sweep.enabled) return;

  const uint32_t now = millis();
  if (now - g_sweep.lastMs < g_sweep.intervalMs) return;
  g_sweep.lastMs = now;

  for (uint8_t ch : kAllCh) {
    servoBus.writeDegrees(ch, g_sweep.posDeg);
  }

  static float lastPrintPos = 0;
  static bool firstRun = true;
  if (firstRun || abs(g_sweep.posDeg - lastPrintPos) >= 10.0f) {
    Serial.print(F("[SWEEP] Position: "));
    Serial.print(g_sweep.posDeg);
    Serial.println(F(" deg - Writing to all 16 channels"));
    lastPrintPos = g_sweep.posDeg;
    firstRun = false;
  }

  g_sweep.posDeg += g_sweep.dir * g_sweep.stepDeg;
  if (g_sweep.posDeg >= g_sweep.maxDeg) {
    g_sweep.posDeg = g_sweep.maxDeg;
    g_sweep.dir = -1;
    Serial.println(F("[SWEEP] Reversing direction (max reached)"));
  }
  if (g_sweep.posDeg <= g_sweep.minDeg) {
    g_sweep.posDeg = g_sweep.minDeg;
    g_sweep.dir = +1;
    Serial.println(F("[SWEEP] Reversing direction (min reached)"));
  }
}

// ========== Command Parser ==========
static void handleCommand(const String& line) {
  if (!line.length()) return;
  
  Serial.print(F("[CMD] RX: "));
  Serial.println(line);
  
  // Neck
  if (line == "LOOK_LEFT") {
    Neck::lookLeft(1.0);
  }
  else if (line == "LOOK_RIGHT") {
    Neck::lookRight(1.0);
  }
  else if (line == "LOOK_CENTER") {
    Neck::center();
  }
  
  // Head/Jaw
  else if (line == "JAW_OPEN") {
    Head::mouthOpen();
  }
  else if (line == "JAW_CLOSE") {
    Head::mouthClose();
  }
  else if (line == "ROAR") {
    Head::roar();
  }
  else if (line == "SNAP") {
    Head::snap();
  }
  else if (line == "HEAD_UP") {
    Head::lookUp(1.0);
  }
  else if (line == "HEAD_DOWN") {
    Head::lookDown(1.0);
  }
  
  // Pelvis
  else if (line == "PELVIS_LEFT") {
    Pelvis::setRoll01(0.0);
  }
  else if (line == "PELVIS_RIGHT") {
    Pelvis::setRoll01(1.0);
  }
  else if (line == "PELVIS_CENTER") {
    Pelvis::center();
  }
  
  // Spine
  else if (line == "SPINE_LEFT") {
    Spine::left();
  }
  else if (line == "SPINE_RIGHT") {
    Spine::right();
  }
  else if (line == "SPINE_CENTER") {
    Spine::center();
  }
  
  // Tail
  else if (line == "TAIL_WAG") {
    Tail::wag();
  }
  else if (line == "TAIL_CENTER") {
    Tail::center();
  }
  
  // Legs
  else if (line == "WALK_FORWARD") {
    Leg::walkForward(1.0);
  }
  else if (line == "WALK_BACKWARD") {
    Leg::walkBackward(1.0);
  }
  else if (line == "TURN_LEFT") {
    Leg::turnLeft(0.8);
  }
  else if (line == "TURN_RIGHT") {
    Leg::turnRight(0.8);
  }
  else if (line == "STOP") {
    Leg::stop();
  }
  
  // System
  else if (line == "CENTER_ALL") {
    Neck::center();
    Head::center();
    Pelvis::center();
    Spine::center();
    Tail::center();
    Leg::stop();
  }
  else if (line == "ALL_OFF") {
    servoBus.setAllOff();
  }
  else if (line == "SWEEP_ON") {
    g_sweep.enabled = true;
    g_sweep.posDeg = g_sweep.minDeg;
    g_sweep.dir = +1;
    Serial.println(F("[CMD] Sweep test ENABLED"));
  }
  else if (line == "SWEEP_OFF") {
    g_sweep.enabled = false;
    Serial.println(F("[CMD] Sweep test DISABLED"));
  }
  else if (line == "STATUS") {
    Serial.println(F("[CMD] System Status:"));
    Serial.print(F("  Sweep enabled: "));
    Serial.println(g_sweep.enabled ? "YES" : "NO");
    if (g_sweep.enabled) {
      Serial.print(F("  Sweep position: "));
      Serial.print(g_sweep.posDeg);
      Serial.println(F(" deg"));
    }
    Serial.print(F("  Leg mode: "));
    Serial.println(Leg::mode());
    Serial.print(F("  Speed: "));
    Serial.print(Leg::speedHz());
    Serial.println(F(" Hz"));
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
  }
  else {
    Serial.print(F("[CMD] Unknown: "));
    Serial.println(line);
  }
}

// ========== Arduino Setup ==========
void setup() {
  // Setup onboard LED for visual feedback (GPIO 48 on Freenove S3)
  pinMode(48, OUTPUT);
  digitalWrite(48, HIGH);  // LED ON to show power

  // Initialize serial with longer delay for ESP32-S3 + CH340
  Serial.begin(115200);
  delay(2000);  // Increased delay for CH340 stability

  // Wait for serial connection (with timeout)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) {
    delay(10);
  }

  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("    ROBO REX - 16 SERVO HYBRID MODE"));
  Serial.println(F("   GPIO (0-5) + PCA9685 (6-15)"));
  Serial.println(F("============================================"));
  Serial.println();

  // Initialize hybrid servo system (GPIO + PCA9685)
  Serial.println(F("[Hybrid] Initializing servo system..."));
  servoBus.begin();
  Serial.println(F("[Hybrid] Ready"));

  // Initialize servo functions
  Serial.println(F("\n[Servos] Initializing 16 servos..."));
  Serial.println(F("  Channels 0-5:  GPIO direct control"));
  Serial.println(F("  Channels 6-15: PCA9685 I2C driver"));

  // Neck (1 servo - channel 0 -> GPIO 1)
  Neck::Map neckMap;
  neckMap.yaw = 0;
  Neck::begin(&servoBus, neckMap);

  // Head (2 servos - channels 1-2 -> GPIO 2,3)
  Head::Map headMap;
  headMap.jaw   = 1;
  headMap.pitch = 2;
  Head::begin(&servoBus, headMap);

  // Pelvis (1 servo - channel 3 -> GPIO 4)
  Pelvis::Map pelvisMap;
  pelvisMap.roll = 3;
  Pelvis::begin(&servoBus, pelvisMap);

  // Spine (1 servo - channel 4 -> GPIO 5)
  Spine::Map spineMap;
  spineMap.spineYaw = 4;
  Spine::begin(&servoBus, spineMap);

  // Tail (1 servo - channel 5 -> GPIO 6)
  Tail::Map tailMap;
  tailMap.wag = 5;
  Tail::begin(&servoBus, tailMap);

  // Legs (10 servos - channels 6-15 -> PCA9685 ports 0-9)
  Leg::Map legMap;
  // Right leg
  legMap.R_hipX  = 6;   // channel 6  -> PCA9685 port 0
  legMap.R_hipY  = 7;   // channel 7  -> PCA9685 port 1
  legMap.R_knee  = 8;   // channel 8  -> PCA9685 port 2
  legMap.R_ankle = 9;   // channel 9  -> PCA9685 port 3
  legMap.R_foot  = 10;  // channel 10 -> PCA9685 port 4
  // Left leg
  legMap.L_hipX  = 11;  // channel 11 -> PCA9685 port 5
  legMap.L_hipY  = 12;  // channel 12 -> PCA9685 port 6
  legMap.L_knee  = 13;  // channel 13 -> PCA9685 port 7
  legMap.L_ankle = 14;  // channel 14 -> PCA9685 port 8
  legMap.L_foot  = 15;  // channel 15 -> PCA9685 port 9
  Leg::begin(&servoBus, legMap);

  Serial.println(F("[Servos] All 16 servos initialized"));

  // Explicitly attach all servos for sweep test
  Serial.println(F("\n[Attach] Attaching all 16 servo channels..."));
  for (uint8_t ch = 0; ch < 16; ch++) {
    servoBus.attach(ch);  // Attach with default limits
  }
  Serial.println(F("[Attach] All channels attached"));

  if (g_sweep.enabled) {
    Serial.println();
    Serial.println(F("============================================"));
    Serial.println(F("  WARNING: SWEEP TEST MODE ENABLED"));
    Serial.println(F("  All servos will sweep 10-170 degrees"));
    Serial.println(F("  Send 'SWEEP_OFF' to disable"));
    Serial.println(F("============================================"));
  }

  // Ready!
  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("           SYSTEM READY"));
  Serial.println(F("============================================"));
  Serial.println(F("Hybrid servo control mode active"));
  Serial.println(F("  GPIO: 6 servos (channels 0-5)"));
  Serial.println(F("  PCA9685: 10 servos (channels 6-15)"));
  Serial.println(F("Type HELP for command list"));
  Serial.println();
}

// ========== Arduino Loop ==========
void loop() {
  // Handle serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;  // Ignore carriage return
    if (c != '\n') {
      g_cmdBuffer += c;
    } else {
      // Process complete line
      String line = g_cmdBuffer;
      line.trim();
      g_cmdBuffer = "";
      if (line.length() > 0) {
        handleCommand(line);
      }
    }
  }

  // Run sweep test or leg control
  if (g_sweep.enabled) {
    sweepAllTick();
  } else {
    Leg::tick();
  }

  delay(20);  // 50 Hz update rate
}