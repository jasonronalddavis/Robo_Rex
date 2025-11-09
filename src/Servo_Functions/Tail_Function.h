#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Tail {

// ========== Pin Mapping Structure ==========
struct Map {
  uint8_t wag = 5;   // PCA9685 channel 5 - tail yaw (left/right wag)
};

// ========== Initialization ==========
// Initialize with ServoBus (PCA9685 control)
void begin(ServoBus* bus, const Map& map);

// ========== Primary Control Functions ==========
// Set tail position using normalized 0.0-1.0 input (legacy behavior)
// 0.0 = neutral position, 1.0 = full right swing
// Note: This matches the original behavior for backward compatibility
void set(float amt01);

// Perform a quick tail wag animation (3 cycles)
// Wags tail left-right-left for friendly/excited behavior
void wag();

// ========== Enhanced Position Control ==========
// Set tail yaw position with full left/right range
// 0.0 = full left, 0.5 = neutral, 1.0 = full right
void setYaw01(float a01);

// Nudge tail by relative angle in degrees
// Positive = right, Negative = left
// Ideal for incremental adjustments or "hold" gestures
void nudgeYawDeg(float deltaDegrees);

// ========== Utility Functions ==========
// Move tail to neutral/center position (straight behind)
void center();

} // namespace Tail