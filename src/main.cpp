// src/main.cpp
#include <Arduino.h>

#include "ServoBus.h"
#include "CommandRouter.h"
#include "BLEServerHandler.h"

// Body modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"
// IMU (Madgwick + MPU6050 or ICM-20948; select in IMU.h)
#include "IMU.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLEServer* pServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;
// ----------------------- Config -----------------------
static const uint8_t PCA9685_ADDR   = 0x40;   // Adafruit PCA9685 default
static const float   SERVO_FREQ_HZ  = 50.0f;  // 50 Hz for hobby servos
#define PRINT_IMU_TELEMETRY 0                // set 1 to stream r/p/y

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
  Head::Map   headMap;   headMap.jaw = 11;   headMap.neckPitch = 12;
  Neck::Map   neckMap;   neckMap.yaw = 13;   neckMap.pitch     = 12;   // adjust if separate
  Tail::Map   tailMap;   tailMap.wag = 14;
  Pelvis::Map pelvisMap; pelvisMap.roll = 15;

  // --- Initialize body modules ---
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // --- BLE (your BLEServerHandler should route to CommandRouter::handle) ---
  BLEServerHandler::begin("Robo_Rex_ESP32S3");
  Serial.println(F("BLE ready — awaiting commands"));

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

      // Signs usually negative so correction fights the tilt.
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

  // Keep loop light; BLE callbacks handle inbound messages
  delay(5);
}
