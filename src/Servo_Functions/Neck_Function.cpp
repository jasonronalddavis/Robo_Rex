#include "Neck_Function.h"

namespace Neck {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Servo Limits ==========
// Tune these values for your mechanical linkages
// Format: ServoLimits(minPulse_µs, maxPulse_µs, minDeg, maxDeg)
static const ServoLimits LIM_YAW  (500, 2500,  30.0f, 150.0f); // left..right
static const ServoLimits LIM_PITCH(500, 2500,  30.0f, 150.0f); // down..up

// ========== Mechanical Configuration ==========
// Neutral pose (roughly straight ahead, level)
static const float NEUTRAL_YAW_DEG   = 90.0f;
static const float NEUTRAL_PITCH_DEG = 90.0f;

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
  
  // Attach servos to PCA9685 channels with limits
  SB->attach(CH.yaw,   LIM_YAW);
  
  // Move to neutral position
  SB->writeDegrees(CH.yaw,   NEUTRAL_YAW_DEG);
  
  Serial.print(F("[Neck] Initialized on PCA9685 channels "));
  Serial.print(CH.yaw);
  Serial.print(F(" (yaw) and "));
  Serial.println(F(" (pitch)"));
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
  
  SB->writeDegrees(CH.yaw, deg);
}

// Nod head up by specified amount
// amt01: 0.0 = neutral, 1.0 = full up
void nod(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  
  // Positive -> pitch up from neutral
  const float deg = NEUTRAL_PITCH_DEG + a * (LIM_PITCH.maxDeg - NEUTRAL_PITCH_DEG);
  
}

// ========== Direct Position Control ==========

// Set yaw position directly
// a01: 0.0 = full left, 1.0 = full right
void setYaw01(float a01) {
  if (!SB) return;
  
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_YAW.minDeg + a * (LIM_YAW.maxDeg - LIM_YAW.minDeg);
  
  SB->writeDegrees(CH.yaw, deg);
}

// Set pitch position directly
// a01: 0.0 = full down, 1.0 = full up
void setPitch01(float a01) {
  if (!SB) return;
  
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_PITCH.minDeg + a * (LIM_PITCH.maxDeg - LIM_PITCH.minDeg);
  
}

// ========== Relative Movement ==========

// Nudge yaw by relative angle
void nudgeYawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_YAW_DEG;
  last = clampf(last + delta, LIM_YAW.minDeg, LIM_YAW.maxDeg);
  
  SB->writeDegrees(CH.yaw, last);
}

// Nudge pitch by relative angle
void nudgePitchDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_PITCH_DEG;
  last = clampf(last + delta, LIM_PITCH.minDeg, LIM_PITCH.maxDeg);
  
}

// ========== Utility Functions ==========

// Move neck to neutral/center position
void center() {
  if (!SB) return;
  
  SB->writeDegrees(CH.yaw,   NEUTRAL_YAW_DEG);
  
  Serial.println(F("[Neck] Centered"));
}

} // namespace Neck