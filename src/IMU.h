#pragma once
#include <Arduino.h>
// #define IMU_SENSOR_ICM20948
#define IMU_SENSOR_MPU6050
#define IMU_SENSOR_ICM20948
// #define IMU_SENSOR_MPU6050


struct ImuState {
  float roll_deg  = 0.0f;  // right down is + roll
  float pitch_deg = 0.0f;  // nose up is + pitch
  float yaw_deg   = 0.0f;  // heading (not used for stabilization)
  uint32_t sample_us = 0;
  bool healthy = false;
};

struct ImuGains {
  // Map measured angles -> servo correction (normalized 0..1)
  // Example: pelvis_correction = clamp(k_roll * roll_deg + b_roll)
  float k_roll  = 0.01f;  // deg -> normalized
  float b_roll  = 0.50f;  // neutral (0..1) when level
  float k_pitch = 0.01f;  // deg -> normalized
  float b_pitch = 0.50f;  // neutral (0..1)
};

namespace IMU {
  // Compile-time select ONE:
  // #define IMU_SENSOR_ICM20948
  // #define IMU_SENSOR_MPU6050
  // If none defined here, IMU.cpp defaults to MPU6050.

  bool begin();                 // init sensor + filter
  void startTask(uint32_t hz);  // spawn FreeRTOS task to run IMU loop
  void get(ImuState& out);      // copy latest filtered state (thread-safe)
  void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg);
  void setGains(const ImuGains& g);
  ImuGains getGains();
  void enable(bool on);
  bool isEnabled();
}
