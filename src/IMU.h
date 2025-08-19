#pragma once
#include <Arduino.h>

// --------------------------------------------------------------------
// I2C pin defaults for ESP32‑S3 (safe, conflict‑free):
//   SDA -> GPIO 19
//   SCL -> GPIO 20
// You can still override at build time with -DIMU_SDA_PIN / -DIMU_SCL_PIN
// --------------------------------------------------------------------
#ifndef IMU_SDA_PIN
#define IMU_SDA_PIN 19
#endif

#ifndef IMU_SCL_PIN
#define IMU_SCL_PIN 20
#endif

// Enable extra logs by adding -DIMU_DEBUG to build_flags
// #define IMU_DEBUG

// Simple IMU state (degrees)
struct ImuState {
  float roll_deg  = 0.0f;  // + right down
  float pitch_deg = 0.0f;  // + nose up
  float yaw_deg   = 0.0f;  // heading estimate (mag disabled -> relative)
  uint32_t sample_us = 0;  // micros() capture time
  bool healthy = false;
};

// Map measured angles -> normalized servo corrections (if you use it)
struct ImuGains {
  float k_roll  = 0.010f;  // deg -> normalized correction
  float b_roll  = 0.50f;   // neutral
  float k_pitch = 0.010f;
  float b_pitch = 0.50f;
};

namespace IMU {
  // Init sensor + filter. Returns true if the sensor is found.
  bool begin();

  // Start background task to continuously read + filter.
  void startTask(uint32_t hz = 200);

  // Copy latest filtered state (thread‑safe).
  void get(ImuState& out);

  // Optional trim offsets (deg) to zero the reference attitude.
  void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg);

  // Optional: simple gains container if you apply stabilization elsewhere.
  void setGains(const ImuGains& g);
  ImuGains getGains();

  // Enable/disable publishing (task still runs; you can gate usage).
  void enable(bool on);
  bool isEnabled();
}
