#include "Spine_Function.h"

namespace Spine {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Servo Limits ==========
// Maps 0-180° to pulse width (µs) and sets safe angle bounds
// Tune these values to prevent mechanical binding
static const ServoLimits LIM_SPINE(500, 2500, 0.0f, 180.0f);

// ========== Mechanical Configuration ==========
// Neutral (centered) angle when spine is straight
static const float NEUTRAL_YAW_DEG = 90.0f;

// UI range for left/right movement
// These define the safe operational range for the spine twist
static const float UI_MIN_DEG = 60.0f;   // Full left position
static const float UI_MAX_DEG = 120.0f;  // Full right position

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
    Serial.println(F("[Spine] ERROR: ServoBus is nullptr!"));
    return;
  }
  
  // Attach servo through PCA9685
  SB->attach(CH.spineYaw, LIM_SPINE);
  
  // Move to neutral position
  SB->writeDegrees(CH.spineYaw, NEUTRAL_YAW_DEG);
  
  Serial.print(F("[Spine] Initialized on PCA9685 channel "));
  Serial.println(CH.spineYaw);
}

// ========== Primary Control Functions ==========

// Twist spine fully left
void left() { 
  if (!SB) return;
  SB->writeDegrees(CH.spineYaw, UI_MIN_DEG);
  Serial.println(F("[Spine] Twisted left"));
}

// Twist spine fully right
void right() { 
  if (!SB) return;
  SB->writeDegrees(CH.spineYaw, UI_MAX_DEG);
  Serial.println(F("[Spine] Twisted right"));
}

// Set spine position using normalized 0.0-1.0 input
// level01: 0.0 = full left, 0.5 = center, 1.0 = full right
void set(float level01) {
  if (!SB) return;
  
  // Clamp input to valid range
  const float a = clampf(level01, 0.0f, 1.0f);
  
  // Map 0.0-1.0 to angle range
  const float deg = UI_MIN_DEG + a * (UI_MAX_DEG - UI_MIN_DEG);
  
  SB->writeDegrees(CH.spineYaw, deg);
}

// ========== Direct Position Control ==========

// Set spine yaw position
// level01: 0.0 = full left, 1.0 = full right
void setYaw01(float level01) {
  set(level01);
}

// Nudge spine by relative angle in degrees
// delta: positive = right, negative = left
void nudgeYawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_YAW_DEG;
  
  // Apply delta
  last += delta;
  
  // Clamp to safe limits
  last = clampf(last, LIM_SPINE.minDeg, LIM_SPINE.maxDeg);
  
  // Also clamp to UI window for consistency
  last = clampf(last, UI_MIN_DEG, UI_MAX_DEG);
  
  SB->writeDegrees(CH.spineYaw, last);
}

// ========== Utility Functions ==========

// Move spine to neutral/center position
void center() {
  if (!SB) return;
  SB->writeDegrees(CH.spineYaw, NEUTRAL_YAW_DEG);
  Serial.println(F("[Spine] Centered"));
}

} // namespace Spine