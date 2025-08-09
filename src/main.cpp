// src/main.cpp
#include <Arduino.h>

#include "ServoBus.h"
#include "BLEServerHandler.h"
#include "CommandRouter.h"

// Servo modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// IMU (Madgwick + ICM-20948 or MPU6050)
#include "IMU.h"

// ---------------- Config ----------------

// PCA9685 I2C address & frequency (most Adafruit boards default to 0x40)
static const uint8_t PCA9685_ADDR = 0x40;
static const float   SERVO_FREQ_HZ = 50.0f;

// OPTIONAL: print IMU telemetry to Serial (comment to disable)
#define PRINT_IMU_TELEMETRY 0

// ------------- Globals / helpers -------------
ServoBus SERVO;

// Simple clamps & mapping helpers
static inline float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static inline float mapNorm(float angle_deg, float k, float b) {
  // normalized_out = clamp01(k * angle_deg + b)
  return clamp01(k * angle_deg + b);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println(F("=== Robo Rex boot ==="));

  // --- Servo bus (PCA9685) ---
  if (!SERVO.begin(PCA9685_ADDR, SERVO_FREQ_HZ)) {
    Serial.println(F("PCA9685 init FAILED"));
  } else {
    Serial.println(F("PCA9685 ready"));
  }

  // --- Channel maps (EDIT THESE TO YOUR WIRING) ---
  // Leg map shown with example channels for 5 DOF per leg.
  Leg::Map legMap;
  legMap.L_hip   = 0;  legMap.L_knee  = 1;  legMap.L_ankle = 2;  legMap.L_foot = 3;  legMap.L_toe = 4;
  legMap.R_hip   = 5;  legMap.R_knee  = 6;  legMap.R_ankle = 7;  legMap.R_foot = 8;  legMap.R_toe = 9;

  Spine::Map  spineMap;  spineMap.spinePitch = 10;
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch = 12;     // shares with headPitch if same linkage
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;  // heavy-duty pelvis servo

  // --- Initialize modules ---
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // --- BLE (NUS-style) ---
  BLEServerHandler::begin("Robo_Rex_ESP32S3");
  Serial.println(F("BLE ready — awaiting commands"));

  // --- IMU init ---
  if (!IMU::begin()) {
    Serial.println(F("IMU init FAILED"));
  } else {
    // Zero offsets (level on the floor), then tune gains.
    IMU::setOffsets(0.0f, 0.0f, 0.0f);

    // Gains: convert degrees -> normalized [0..1]
    // b_* defines neutral servo level when level (0 deg).
    // Signs on k_* often need to be negative so the correction pushes against tilt.
    ImuGains gains;
    gains.k_roll  = 0.012f;  // deg -> normalized
    gains.b_roll  = 0.50f;   // neutral pelvis
    gains.k_pitch = 0.010f;  // deg -> normalized
    gains.b_pitch = 0.50f;   // neutral spine
    IMU::setGains(gains);

    IMU::enable(true);
    IMU::startTask(200);     // 200 Hz filter loop on a FreeRTOS task
    Serial.println(F("IMU ready (stabilization enabled)"));
  }

  Serial.println(F("=== Setup complete ==="));
}

// ---------------- Main loop ----------------
void loop() {
  // IMU → stabilization
  if (IMU::isEnabled()) {
    ImuState s; 
    IMU::get(s);
    if (s.healthy) {
      // Get current gains (so you can tweak live via BLE rex_stab_gains)
      ImuGains g = IMU::getGains();

      // Map angles to normalized actuator commands.
      // Note: signs chosen so servo correction fights the tilt.
      const float pelvisLevel = mapNorm(s.roll_deg,  -g.k_roll,  g.b_roll);
      const float spineLevel  = mapNorm(s.pitch_deg, -g.k_pitch, g.b_pitch);

      Pelvis::stabilize(pelvisLevel);
      Spine::set(spineLevel);

    #if PRINT_IMU_TELEMETRY
      static uint32_t t0 = 0;
      const uint32_t now = millis();
      if (now - t0 > 200) {
        t0 = now;
        Serial.print(F("IMU r/p/y: "));
        Serial.print(s.roll_deg, 1);  Serial.print(' ');
        Serial.print(s.pitch_deg, 1); Serial.print(' ');
        Serial.print(s.yaw_deg, 1);
        Serial.print(F(" | pelvis=")); Serial.print(pelvisLevel, 2);
        Serial.print(F(" spine="));    Serial.println(spineLevel, 2);
      }
    #endif
    }
  }

  // Housekeeping — BLE runs via callbacks; main loop can stay light.
  delay(5);
}
