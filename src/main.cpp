// src/main.cpp
#include <Arduino.h>

#include "ServoBus.h"
#include "BLEServerHandler.h"
#include "CommandRouter.h"

// Body modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// IMU (Madgwick + MPU6050 or ICM-20948; select in IMU.h)
#include "IMU.h"

// ----------------------- Config -----------------------
static const uint8_t PCA9685_ADDR  = 0x40;   // Adafruit PCA9685 default
static const float   SERVO_FREQ_HZ = 50.0f;  // 50 Hz for hobby servos

// Enable to stream r/p/y + levels at 5 Hz
#define PRINT_IMU_TELEMETRY 0

// -------------------- Globals -------------------------
ServoBus SERVO;

// Helpers
static inline float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static inline float mapNorm(float angle_deg, float k, float b) {
  // normalized_out = clamp01(k * angle_deg + b)
  return clamp01(k * angle_deg + b);
}

// --------------------- Setup --------------------------
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

  // --- Channel maps  (EDIT THESE TO MATCH YOUR WIRING) ---
  Leg::Map legMap;
  legMap.L_hip   = 0;  legMap.L_knee  = 1;  legMap.L_ankle = 2;  legMap.L_foot = 3;  legMap.L_toe = 4;
  legMap.R_hip   = 5;  legMap.R_knee  = 6;  legMap.R_ankle = 7;  legMap.R_foot = 8;  legMap.R_toe = 9;

  Spine::Map  spineMap;  spineMap.spinePitch = 10;
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;  // adjust to your head joints
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch     = 12;  // if separate, set properly
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;

  // --- Initialize body modules ---
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // --- BLE (Nordic UART) ---
  // Advertised name should match what the web app scans for.
  BLEServerHandler::begin("Robo_Rex_ESP32S3");
  // Optional: echo every received line on Serial for debugging
  // BLEServerHandler::setEcho(true);

  // --- Command router (JSON: {target, part, command, phase}) ---
  CommandRouter::begin();

  // --- IMU (stabilization) ---
  if (!IMU::begin()) {
    Serial.println(F("IMU init FAILED"));
  } else {
    // Level on flat ground; you can overwrite via BLE rex_stab_cal
    IMU::setOffsets(0.0f, 0.0f, 0.0f);

    ImuGains gains;
    gains.k_roll  = 0.012f;  // deg -> normalized
    gains.b_roll  = 0.50f;   // neutral pelvis level
    gains.k_pitch = 0.010f;  // deg -> normalized
    gains.b_pitch = 0.50f;   // neutral spine level
    IMU::setGains(gains);

    IMU::enable(true);
    IMU::startTask(200);     // 200 Hz filter loop on its own core/task
    Serial.println(F("IMU ready (stabilization enabled)"));
  }

  Serial.println(F("=== Setup complete ==="));
}

// ---------------------- Loop --------------------------
void loop() {
  // Run gait engine continuously
  Leg::tick();

  // IMU → Pelvis/Spine stabilization
  if (IMU::isEnabled()) {
    ImuState s;
    IMU::get(s);
    if (s.healthy) {
      const ImuGains g = IMU::getGains();

      // Signs are usually negative so correction fights the tilt.
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

  // Router/Server background (no-op placeholders if not needed)
  CommandRouter::tick();
  BLEServerHandler::tick();

  // Keep loop light; BLE callbacks handle inbound messages
  delay(5);
}
