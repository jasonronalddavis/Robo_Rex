#include "Neck_Function.h"

namespace Neck {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Servo Limits ==========
// Tune these values for your mechanical linkages
static const ServoLimits LIM_YAW(500, 2500, 30.0f, 150.0f); // left..right

// ========== Mechanical Configuration ==========
// Neutral pose (head straight ahead)
static const float NEUTRAL_YAW_DEG = 90.0f;

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
    Serial.println(F("[Neck] ERROR: ServoBus is nullptr!"));
    return;
  }
  
  // Attach servo via ServoBus (automatically routes to GPIO or PCA9685)
  SB->attach(CH.yaw, LIM_YAW);

  // Move to neutral position
  SB->writeDegrees(CH.yaw, NEUTRAL_YAW_DEG);

  Serial.print(F("[Neck] Initialized on ServoBus channel "));
  Serial.println(CH.yaw);
}

// ========== Primary Control Functions ==========

// Look left by specified amount
// amt01: 0.0 = neutral, 1.0 = full left
void lookLeft(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  
  // Map [0..1] into the left half of the yaw range
  const float deg = NEUTRAL_YAW_DEG - a * (NEUTRAL_YAW_DEG - LIM_YAW.minDeg);
  
  SB->writeDegrees(CH.yaw, deg);
}

// Look right by specified amount
// amt01: 0.0 = neutral, 1.0 = full right
void lookRight(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  
  // Map [0..1] into the right half of the yaw range
  const float deg = NEUTRAL_YAW_DEG + a * (LIM_YAW.maxDeg - NEUTRAL_YAW_DEG);
  
  SB->writeDegrees(CH.yaw,deg);
}

// ========== Direct Position Control ==========

// Set YAW position directly
// a01: 0.0 = full left, 1.0 = full right
void setYaw01(float a01) {
  if (!SB) return;
  
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_YAW.minDeg + a * (LIM_YAW.maxDeg - LIM_YAW.minDeg);
  
  SB->writeDegrees(CH.yaw, deg);
}

// ========== Relative Movement ==========

// Nudge jaw by relative angle
void nudgeyawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_YAW_DEG;
  last = clampf(last + delta, LIM_YAW.minDeg, LIM_YAW.maxDeg);
  
  SB->writeDegrees(CH.yaw, last);
}

// ========== Utility Functions ==========

// Move neck to neutral/center position
void center() {
  if (!SB) return;
  
  SB->writeDegrees(CH.yaw, NEUTRAL_YAW_DEG);
  
  Serial.println(F("[Neck] Centered"));
}

} // namespace Neck