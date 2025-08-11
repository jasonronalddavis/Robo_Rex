#include "Pelvis_Function.h"

namespace Pelvis {

static ServoBus* SB = nullptr;
static Map       CH;

// Limits (tune to your linkage so you never bind the horn)
// These map 0..180 deg to pulse µs, and also bound the usable angle range.
static const ServoLimits LIM_ROLL(700, 2400,  0.0f, 180.0f);

// Choose a neutral (level) angle and how much total swing you want from UI.
// Example: ±20° about neutral when UI sends 0..1.
static const float NEUTRAL_ROLL_DEG = 90.0f;
static const float UI_SWING_DEG     = 20.0f;   // 0..1 → 90±20°

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;
  if (!SB) return;

  SB->attach(CH.roll, LIM_ROLL);
  SB->writeDegrees(CH.roll, NEUTRAL_ROLL_DEG);
}

// -------- Existing API --------
void set(float level01) {
  if (!SB) return;
  const float a   = clampf(level01, 0.0f, 1.0f);
  const float deg = NEUTRAL_ROLL_DEG + (a - 0.5f) * (2.0f * UI_SWING_DEG);
  SB->writeDegrees(CH.roll, deg);
}

void stabilize(float rollLevel01) {
  // For now, this is a thin wrapper around set(). In your IMU loop,
  // you already compute rollLevel01 via: clamp(k*roll_deg + b).
  set(rollLevel01);
}

// -------- Optional helpers --------
void setLevel01(float level01) {
  set(level01);
}

void nudgeRollDeg(float delta) {
  if (!SB) return;
  // Keep a simple cached last command; start from neutral.
  static float lastDeg = NEUTRAL_ROLL_DEG;
  lastDeg = clampf(lastDeg + delta, LIM_ROLL.minDeg, LIM_ROLL.maxDeg);
  SB->writeDegrees(CH.roll, lastDeg);
}

void center() {
  if (!SB) return;
  SB->writeDegrees(CH.roll, NEUTRAL_ROLL_DEG);
}

} // namespace Pelvis
