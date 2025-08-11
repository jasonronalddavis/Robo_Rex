#include "Tail_Function.h"

namespace Tail {

static ServoBus* SB = nullptr;
static Map       CH;

// Limits — tune to your linkage so you never bind the horn.
static const ServoLimits LIM_TAIL(500, 2500,  0.0f, 180.0f);

// Neutral pose and UI swing range.
// Legacy set() used 90 + 30*amt → keep that behavior as default.
static const float NEUTRAL_YAW_DEG = 90.0f;
static const float UI_SWING_DEG    = 30.0f;   // 0..1 → 90..120 (or 60..90 if you choose)

// Small helper
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;
  if (!SB) return;

  SB->attach(CH.wag, LIM_TAIL);
  SB->writeDegrees(CH.wag, NEUTRAL_YAW_DEG);
}

// ---------------- Existing API ----------------
// Legacy absolute: a ∈ [0..1] mapped to NEUTRAL..(NEUTRAL+UI_SWING_DEG).
// (Historically this only swept to the right; we keep it for back-compat.)
void set(float amt01) {
  if (!SB) return;
  const float a   = clampf(amt01, 0.0f, 1.0f);
  const float deg = NEUTRAL_YAW_DEG + a * UI_SWING_DEG;
  SB->writeDegrees(CH.wag, deg);
}

void wag() {
  if (!SB) return;
  for (int i = 0; i < 3; ++i) {
    set(1.0f); delay(150);
    set(0.0f); delay(150);
  }
}

// ---------------- Optional helpers ----------------
// Absolute left..right over the full symmetric UI window.
// a01=0.0 -> hard left (neutral-UI_SWING), 0.5 -> neutral, 1.0 -> hard right.
void setYaw01(float a01) {
  if (!SB) return;
  const float a   = clampf(a01, 0.0f, 1.0f);
  const float deg = (NEUTRAL_YAW_DEG - UI_SWING_DEG) + a * (2.0f * UI_SWING_DEG);
  SB->writeDegrees(CH.wag, deg);
}

// Relative nudges (ideal for “hold” ticks from the UI)
void nudgeYawDeg(float delta) {
  if (!SB) return;
  static float last = NEUTRAL_YAW_DEG;
  last = clampf(last + delta, LIM_TAIL.minDeg, LIM_TAIL.maxDeg);
  SB->writeDegrees(CH.wag, last);
}

void center() {
  if (!SB) return;
  SB->writeDegrees(CH.wag, NEUTRAL_YAW_DEG);
}

} // namespace Tail
