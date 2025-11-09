#include "Pelvis_Function.h"

namespace Pelvis {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map CH;                    // Channel mapping

// ========== Servo Limits ==========
// Maps 0-180° to pulse width (µs) and sets safe angle bounds
// Tune these values to prevent mechanical binding
static const ServoLimits LIM_ROLL(700, 2400, 0.0f, 180.0f);

// ========== Mechanical Configuration ==========
// Neutral (level) angle when pelvis is centered
static const float NEUTRAL_ROLL_DEG = 90.0f;

// Maximum swing from neutral for UI input (0.0-1.0)
// Example: UI_SWING_DEG = 20° means:
//   - 0.0 → 70° (left)
//   - 0.5 → 90° (center)  
//   - 1.0 → 110° (right)
static const float UI_SWING_DEG = 20.0f;

// ========== State Tracking ==========
static float currentAngleDeg = NEUTRAL_ROLL_DEG;  // Current angle for tracking

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
    Serial.println(F("[Pelvis] ERROR: ServoBus is nullptr!"));
    return;
  }
  
  // Attach servo through PCA9685
  SB->attach(CH.roll, LIM_ROLL);
  
  // Move to neutral position
  currentAngleDeg = NEUTRAL_ROLL_DEG;
  SB->writeDegrees(CH.roll, currentAngleDeg);
  
  Serial.print(F("[Pelvis] Initialized on PCA9685 channel "));
  Serial.println(CH.roll);
}

// ========== Primary Control Functions ==========

// Set pelvis roll using normalized 0.0-1.0 input
void set(float level01) {
  if (!SB) return;
  
  // Clamp input to valid range
  const float normalized = clampf(level01, 0.0f, 1.0f);
  
  // Map 0.0-1.0 to angle range around neutral
  // 0.0 = neutral - swing
  // 0.5 = neutral
  // 1.0 = neutral + swing
  const float angle = NEUTRAL_ROLL_DEG + (normalized - 0.5f) * (2.0f * UI_SWING_DEG);
  
  // Clamp to safe limits
  currentAngleDeg = clampf(angle, LIM_ROLL.minDeg, LIM_ROLL.maxDeg);
  
  SB->writeDegrees(CH.roll, currentAngleDeg);
}

// Alternative name for set() - more explicit
void setLevel01(float level01) {
  set(level01);
}

// Alias for compatibility with main.cpp
void setRoll01(float level01) {
  set(level01);
}

// ========== IMU Stabilization ==========

// Feedforward from IMU for closed-loop leveling
void stabilize(float rollLevel01) {
  // In your IMU loop, you compute rollLevel01 as:
  //   clamp(k * roll_deg + b, 0.0, 1.0)
  // This provides continuous correction based on IMU feedback
  set(rollLevel01);
}

// ========== Utility Functions ==========

// Move pelvis to neutral/center position
void center() {
  if (!SB) return;
  
  currentAngleDeg = NEUTRAL_ROLL_DEG;
  SB->writeDegrees(CH.roll, currentAngleDeg);
  
  Serial.println(F("[Pelvis] Centered"));
}

// Nudge pelvis by relative angle in degrees
void nudgeRollDeg(float deltaDegrees) {
  if (!SB) return;
  
  currentAngleDeg = clampf(currentAngleDeg + deltaDegrees, LIM_ROLL.minDeg, LIM_ROLL.maxDeg);
  SB->writeDegrees(CH.roll, currentAngleDeg);
}

// ========== Direct Angle Control ==========

// Set pelvis to specific angle in degrees (within safe limits)
void setAngleDeg(float degrees) {
  if (!SB) return;
  
  currentAngleDeg = clampf(degrees, LIM_ROLL.minDeg, LIM_ROLL.maxDeg);
  SB->writeDegrees(CH.roll, currentAngleDeg);
}

// Get current pelvis angle in degrees
float getCurrentAngleDeg() {
  return currentAngleDeg;
}

} // namespace Pelvis