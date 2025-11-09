#include "Head_Function.h"

namespace Head {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map CH;                    // Channel mapping

// ========== Servo Limits ==========
// Tune these values for your mechanical linkages
static const ServoLimits LIM_JAW  (500, 2500,  30.0f, 150.0f); // closed..open
static const ServoLimits LIM_PITCH(500, 2500,  30.0f, 150.0f); // down..up

// ========== Mechanical Configuration ==========
// Neutral positions
static const float NEUTRAL_JAW_DEG   = 60.0f;  // Mouth mostly closed
static const float NEUTRAL_PITCH_DEG = 90.0f;  // Head level

// Jaw positions for common actions
static const float JAW_CLOSED_DEG = 50.0f;   // Fully closed
static const float JAW_OPEN_DEG   = 120.0f;  // Fully open

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
    Serial.println(F("[Head] ERROR: ServoBus is nullptr!"));
    return;
  }
  
  // Attach servos to PCA9685 channels with limits
  SB->attach(CH.jaw,       LIM_JAW);
  SB->attach(CH.neckPitch, LIM_PITCH);
  
  // Move to neutral position
  SB->writeDegrees(CH.jaw,       NEUTRAL_JAW_DEG);
  SB->writeDegrees(CH.neckPitch, NEUTRAL_PITCH_DEG);
  
  Serial.print(F("[Head] Initialized on PCA9685 channels "));
  Serial.print(CH.jaw);
  Serial.print(F(" (jaw) and "));
  Serial.print(CH.neckPitch);
  Serial.println(F(" (pitch)"));
}

// ========== Jaw Control ==========

// Open mouth fully
void mouthOpen() {
  if (!SB) return;
  SB->writeDegrees(CH.jaw, JAW_OPEN_DEG);
}

// Close mouth fully
void mouthClose() {
  if (!SB) return;
  SB->writeDegrees(CH.jaw, JAW_CLOSED_DEG);
}

// Set jaw position
// amt01: 0.0 = closed, 1.0 = open
void setJaw01(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  const float deg = JAW_CLOSED_DEG + a * (JAW_OPEN_DEG - JAW_CLOSED_DEG);
  
  SB->writeDegrees(CH.jaw, deg);
}

// ========== Head Pitch Control ==========

// Look up by specified amount
// amt01: 0.0 = neutral, 1.0 = full up
void lookUp(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  const float deg = NEUTRAL_PITCH_DEG + a * (LIM_PITCH.maxDeg - NEUTRAL_PITCH_DEG);
  
  SB->writeDegrees(CH.neckPitch, deg);
}

// Look down by specified amount
// amt01: 0.0 = neutral, 1.0 = full down
void lookDown(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  const float deg = NEUTRAL_PITCH_DEG - a * (NEUTRAL_PITCH_DEG - LIM_PITCH.minDeg);
  
  SB->writeDegrees(CH.neckPitch, deg);
}

// Set head pitch directly
// amt01: 0.0 = full down, 1.0 = full up
void setPitch01(float amt01) {
  if (!SB) return;
  
  const float a = clampf(amt01, 0.0f, 1.0f);
  const float deg = LIM_PITCH.minDeg + a * (LIM_PITCH.maxDeg - LIM_PITCH.minDeg);
  
  SB->writeDegrees(CH.neckPitch, deg);
}

// ========== Animation Sequences ==========

// Roar animation
void roar() {
  if (!SB) return;
  
  Serial.println(F("[Head] ROAR!"));
  
  // Look up and open mouth
  SB->writeDegrees(CH.neckPitch, LIM_PITCH.maxDeg);
  SB->writeDegrees(CH.jaw, JAW_OPEN_DEG);
  delay(300);
  
  // Quick jaw movements
  for (int i = 0; i < 3; i++) {
    SB->writeDegrees(CH.jaw, JAW_CLOSED_DEG);
    delay(100);
    SB->writeDegrees(CH.jaw, JAW_OPEN_DEG);
    delay(100);
  }
  
  // Return to neutral
  delay(200);
  SB->writeDegrees(CH.jaw, NEUTRAL_JAW_DEG);
  SB->writeDegrees(CH.neckPitch, NEUTRAL_PITCH_DEG);
}

// Snap animation
void snap() {
  if (!SB) return;
  
  Serial.println(F("[Head] Snap!"));
  
  // Quick jaw open and close
  SB->writeDegrees(CH.jaw, JAW_OPEN_DEG);
  delay(100);
  SB->writeDegrees(CH.jaw, JAW_CLOSED_DEG);
  delay(100);
  SB->writeDegrees(CH.jaw, NEUTRAL_JAW_DEG);
}

// ========== Utility Functions ==========

// Move head to neutral/center position
void center() {
  if (!SB) return;
  
  SB->writeDegrees(CH.jaw,       NEUTRAL_JAW_DEG);
  SB->writeDegrees(CH.neckPitch, NEUTRAL_PITCH_DEG);
  
  Serial.println(F("[Head] Centered"));
}

// Nudge jaw by relative angle
void nudgeJawDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_JAW_DEG;
  last = clampf(last + delta, LIM_JAW.minDeg, LIM_JAW.maxDeg);
  
  SB->writeDegrees(CH.jaw, last);
}

// Nudge pitch by relative angle
void nudgePitchDeg(float delta) {
  if (!SB) return;
  
  static float last = NEUTRAL_PITCH_DEG;
  last = clampf(last + delta, LIM_PITCH.minDeg, LIM_PITCH.maxDeg);
  
  SB->writeDegrees(CH.neckPitch, last);
}

} // namespace Head