#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Spine {

// ========== Pin Mapping Structure ==========
struct Map {
  uint8_t spineYaw = 15;  // PCA9685 channel 15 - spine left/right twist
};

// ========== Initialization ==========
// Initialize with ServoBus (PCA9685 control)
void begin(ServoBus* bus, const Map& map);

// ========== Primary Control Functions ==========
// Quick preset positions
void left();                      // Twist spine fully left
void right();                     // Twist spine fully right

// Absolute positioning (0.0 = full left, 1.0 = full right)
void set(float level01);

// ========== Direct Position Control ==========
// Set spine yaw position (0.0 = full left, 1.0 = full right)
void setYaw01(float level01);

// Nudge spine by relative angle in degrees
// Positive = right, Negative = left
void nudgeYawDeg(float deltaDegrees);

// ========== Utility Functions ==========
// Move spine to neutral/center position
void center();

} // namespace Spine