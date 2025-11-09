#include "Tail_Function.h"

namespace Tail {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Servo Limits ==========
// Maps 0-180° to pulse width (µs) and sets safe angle bounds
// Tune these values to prevent mechanical binding or tail hitting body
static const ServoLimits LIM_TAIL(500, 2500, 0.0f, 180.0f);

// ========== Mechanical Configuration ==========
// Neutral position (tail straight behind)
static const float NEUTRAL_YAW_DEG = 90.0f;

// Swing range for tail wagging
// UI_SWING_DEG = 30° means tail can swing ±30° from neutral
// This gives a total 60° range of motion
static const float UI_SWING_DEG = 30.0f;

// ========== Helper Functions ==========
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ========== Initialization ==========
void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;
  
  if (!SB) {
    Serial.println(F("[Tail] ERROR: ServoBus is nullptr!"));
    return;
  }
  
  // Attach servo through PCA9685
  SB->attach(CH.wag, LIM_TAIL);
  
  // Move to neutral position (straight behind)
  SB->writeDegrees(CH.wag, NEUTRAL_YAW_DEG);
  
  Serial.print(F("[Tail] Initialized on PCA9685 channel "));
  Serial.println(CH.wag);
}

// ========== Primary Control Functions ==========

// Legacy absolute positioning (backward compatibility)
// Maps 0.0-1.0 to neutral → (neutral + swing)
// Originally designed to only swing right from neutral
// amt01: 0.0 = neutral (90°), 1.0 = full right (120°)
void set(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  const float deg = NEUTRAL_YAW_DEG + a * UI_SWING_DEG;
  
  SB->writeDegrees(CH.wag, deg);
}

// Quick tail wag animation
// Performs 3 cycles of left-right wagging
// Great for showing excitement or friendliness
void wag() {
  if (!SB) return;
  
  Serial.println(F("[Tail] Wagging!"));
  
  for (int i = 0; i < 3; ++i) {
    // Swing right
    set(1.0f); 
    delay(150);
    
    // Swing left (using setYaw01 for symmetry)
    setYaw01(0.0f);
    delay(150);
  }
  
  // Return to neutral
  center();
}

// ========== Enhanced Position Control ==========

// Absolute positioning with full left/right symmetry
// a01: 0.0 = full left (60°), 0.5 = neutral (90°), 1.0 = full right (120°)
void setYaw01(float a01) {
  if (!SB) return;
  
  const float a = clampf(a01, 0.0f, 1.0f);
  
  // Map 0.0-1.0 to full left..right range
  const float deg = (NEUTRAL_YAW_DEG - UI_SWING_DEG) + a * (2.0f * UI_SWING_DEG);
  
  SB->writeDegrees(CH.wag, deg);
}

// Relative nudge in degrees
// Positive delta = wag right, negative delta = wag left
// Useful for smooth incremental adjustments
void nudgeYawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_YAW_DEG;
  
  // Apply delta
  last += delta;
  
  // Clamp to safe limits
  last = clampf(last, LIM_TAIL.minDeg, LIM_TAIL.maxDeg);
  
  SB->writeDegrees(CH.wag, last);
}

// ========== Utility Functions ==========

// Return tail to neutral position (straight behind)
void center() {
  if (!SB) return;
  SB->writeDegrees(CH.wag, NEUTRAL_YAW_DEG);
  Serial.println(F("[Tail] Centered"));
}

} // namespace Tail