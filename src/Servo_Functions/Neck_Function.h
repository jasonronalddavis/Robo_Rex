#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Neck {

// ========== Pin Mapping Structure ==========
struct Map {
  uint8_t jaw = 0;   // PCA9685 channel 0 - neck left/right turn
};

// ========== Initialization ==========
// Initialize with ServoBus (PCA9685 control)
void begin(ServoBus* bus, const Map& map);

// ========== Primary Control Functions ==========
// Look left by specified amount (0.0 = neutral, 1.0 = full left)
void lookLeft(float amt01);

// Look right by specified amount (0.0 = neutral, 1.0 = full right)
void lookRight(float amt01);

// ========== Direct Position Control ==========
// Set yaw position directly (0.0 = full left, 1.0 = full right)
void setYaw01(float a01);

// ========== Relative Movement ==========
// Nudge yaw by relative angle in degrees (positive = right, negative = left)
void nudgeYawDeg(float deltaDegrees);

// ========== Utility Functions ==========
// Move neck to neutral/center position
void center();

} // namespace Neck