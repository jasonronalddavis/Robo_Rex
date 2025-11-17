#pragma once
#include <Arduino.h>

// --------------------------------------------------------------------
// I2C pin configuration for ESP32‑S3 - IMU gets its own dedicated bus
// IMU uses Wire1 on GPIO 19/20 (separate from PCA9685 servos)
// --------------------------------------------------------------------
#ifndef IMU_SDA_PIN
#define IMU_SDA_PIN 8
#endif

#ifndef IMU_SCL_PIN
#define IMU_SCL_PIN 9
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
  uint32_t error_count = 0;  // Track read failures
  uint32_t success_count = 0;  // Track successful reads
};

// Map measured angles -> normalized servo corrections (if you use it)
struct ImuGains {
  float k_roll  = 0.010f;  // deg -> normalized correction
  float b_roll  = 0.50f;   // neutral
  float k_pitch = 0.010f;
  float b_pitch = 0.50f;
};

namespace IMU {
  // Init sensor + filter on dedicated Wire1 bus. Returns true if sensor found.
  bool begin();

  // Start background task to continuously read + filter.
  void startTask(uint32_t hz = 200);

  // Stop background task
  void stopTask();

  // Copy latest filtered state (thread‑safe).
  void get(ImuState& out);

  // Check if IMU is responding
  bool isHealthy();

  // Get basic sensor info for debugging
  void printStatus();

  // Optional trim offsets (deg) to zero the reference attitude.
  void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg);

  // Optional: simple gains container if you apply stabilization elsewhere.
  void setGains(const ImuGains& g);
  ImuGains getGains();

  // Enable/disable publishing (task still runs; you can gate usage).
  void enable(bool on);
  bool isEnabled();

  // Test basic I2C communication with MPU6050 on Wire1
  bool testConnection();
  
  // Wake up MPU6050 from sleep mode (if needed)
  bool wakeUpDevice();
  
  // Try initialization with both possible addresses
  bool beginWithAddressDetection();
}