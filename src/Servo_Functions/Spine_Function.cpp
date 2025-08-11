#include "Spine_Function.h"

namespace Spine {

static ServoBus* SB = nullptr;
static Map       CH;

// Limits — tune to your linkage so you never bind the horn.
// (microsecond range maps the servo; deg range bounds motion we’ll command)
static const ServoLimits LIM_SPINE(500, 2500,  0.0f, 180.0f);

// Choose a neutral and the UI range you want.
// Previous hard-coded mapping was 60..120°, so we’ll keep that as defaults.
static const float NEUTRAL_PITCH_DEG = 90.0f;
static const float UI_MIN_DEG        = 60.0f;
static const float UI_MAX_DEG        = 120.0f;

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void begin(ServoBus* bus, const Map& map) {
  SB = bus; 
  CH = map;
  if (!SB) return;

  SB->attach(CH.spinePitch, LIM_SPINE);
  SB->writeDegrees(CH.spinePitch, NEUTRAL_PITCH_DEG);
}

// ---------------- Existing API ----------------
void up()   { if (SB) SB->writeDegrees(CH.spinePitch, UI_MAX_DEG); }
void down() { if (SB) SB->writeDegrees(CH.spinePitch, UI_MIN_DEG); }

void set(float level01) {
  if (!SB) return;
  const float a   = clampf(level01, 0.0f, 1.0f);
  const float deg = UI_MIN_DEG + a * (UI_MAX_DEG - UI_MIN_DEG);
  SB->writeDegrees(CH.spinePitch, deg);
}

// ---------------- Optional helpers ----------------
void setPitch01(float level01) {
  set(level01);
}

void nudgePitchDeg(float delta) {
  if (!SB) return;
  static float last = NEUTRAL_PITCH_DEG;
  last = clampf(last + delta, LIM_SPINE.minDeg, LIM_SPINE.maxDeg);
  // Also clamp to UI window so UI/IMU expectations match
  last = clampf(last, UI_MIN_DEG, UI_MAX_DEG);
  SB->writeDegrees(CH.spinePitch, last);
}

void center() {
  if (!SB) return;
  SB->writeDegrees(CH.spinePitch, NEUTRAL_PITCH_DEG);
}

} // namespace Spine
