#include "Neck_Function.h"

namespace Neck {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Servo Limits ==========
// Tune these values for your mechanical linkages
static const ServoLimits LIM_JAW(500, 2500, 30.0f, 150.0f); // left..right

// ========== Mechanical Configuration ==========
// Neutral pose (head straight ahead)
static const float NEUTRAL_JAW_DEG = 90.0f;

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
  
  // Attach servo to PCA9685 channel with limits
  SB->attach(CH.jaw, LIM_JAW);
  
  // Move to neutral position
  SB->writeDegrees(CH.jaw, NEUTRAL_JAW_DEG);
  
  Serial.print(F("[Neck] Initialized on PCA9685 channel "));
  Serial.println(CH.jaw);
}

// ========== Primary Control Functions ==========

// Look left by specified amount
// amt01: 0.0 = neutral, 1.0 = full left
void lookLeft(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  
  // Map [0..1] into the left half of the jaw range
  const float deg = NEUTRAL_JAW_DEG - a * (NEUTRAL_JAW_DEG - LIM_JAW.minDeg);
  
  SB->writeDegrees(CH.jaw, deg);
}

// Look right by specified amount
// amt01: 0.0 = neutral, 1.0 = full right
void lookRight(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  
  // Map [0..1] into the right half of the jaw range
  const float deg = NEUTRAL_JAW_DEG + a * (LIM_JAW.maxDeg - NEUTRAL_JAW_DEG);
  
  SB->writeDegrees(CH.jaw, deg);
}

// ========== Direct Position Control ==========

// Set jaw position directly
// a01: 0.0 = full left, 1.0 = full right
void setJaw01(float a01) {
  if (!SB) return;
  
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_JAW.minDeg + a * (LIM_JAW.maxDeg - LIM_JAW.minDeg);
  
  SB->writeDegrees(CH.jaw, deg);
}

// ========== Relative Movement ==========

// Nudge jaw by relative angle
void nudgeJawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_JAW_DEG;
  last = clampf(last + delta, LIM_JAW.minDeg, LIM_JAW.maxDeg);
  
  SB->writeDegrees(CH.jaw, last);
}

// ========== Utility Functions ==========

// Move neck to neutral/center position
void center() {
  if (!SB) return;
  
  SB->writeDegrees(CH.jaw, NEUTRAL_JAW_DEG);
  
  Serial.println(F("[Neck] Centered"));
}

} // namespace Neck