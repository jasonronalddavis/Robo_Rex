#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Pelvis {

// ========== Pin Mapping Structure ==========
struct Map {
  uint8_t roll = 2;   // PCA9685 channel 2 - pelvis roll/level control
};

// ========== Initialization ==========
// Initialize with ServoBus (PCA9685 control)
void begin(ServoBus* bus, const Map& map);

// ========== Primary Control Functions ==========
// Set pelvis roll level using normalized 0.0-1.0 input
// 0.0 = full left, 0.5 = center, 1.0 = full right
void set(float level01);

// Alternative name for set() - more explicit
void setLevel01(float level01);

// Alias for compatibility with main.cpp
void setRoll01(float level01);

// ========== IMU Stabilization ==========
// Feedforward from IMU for closed-loop leveling
// Pass normalized roll level (0.0-1.0) computed from IMU data
void stabilize(float rollLevel01);

// ========== Utility Functions ==========
// Move pelvis to neutral/center position
void center();

// Nudge pelvis by relative angle in degrees
// Positive = right, Negative = left
void nudgeRollDeg(float deltaDegrees);

// ========== Direct Angle Control (Advanced) ==========
// Set pelvis to specific angle in degrees (within safe limits)
void setAngleDeg(float degrees);

// Get current pelvis angle in degrees
float getCurrentAngleDeg();

} // namespace Pelvis