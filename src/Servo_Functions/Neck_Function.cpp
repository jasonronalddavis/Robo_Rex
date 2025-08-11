#include "Neck_Function.h"

namespace Neck {

static ServoBus* SB = nullptr;
static Map       CH;

// Limits — tune for your mechanics so you don’t hit horns/links
// (These protect against overtravel. Adjust min/maxDeg as needed.)
static const ServoLimits LIM_YAW  (500, 2500,  30.0f, 150.0f); // left..right
static const ServoLimits LIM_PITCH(500, 2500,  30.0f, 150.0f); // down..up

// Neutral pose (roughly straight ahead, level)
static const float NEUTRAL_YAW_DEG   = 90.0f;
static const float NEUTRAL_PITCH_DEG = 90.0f;

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;
  if (!SB) return;

  SB->attach(CH.yaw,   LIM_YAW);
  SB->attach(CH.pitch, LIM_PITCH);

  SB->writeDegrees(CH.yaw,   NEUTRAL_YAW_DEG);
  SB->writeDegrees(CH.pitch, NEUTRAL_PITCH_DEG);
}

// ---------------- Existing API ----------------
// amt01 ∈ [0..1]. We bias around neutral ±range for readability.
void lookLeft(float amt01) {
  if (!SB) return;
  const float a = clampf(amt01, 0.0f, 1.0f);
  // Map [0..1] into the left half of the yaw range
  const float deg = NEUTRAL_YAW_DEG - a * (NEUTRAL_YAW_DEG - LIM_YAW.minDeg);
  SB->writeDegrees(CH.yaw, deg);
}
void lookRight(float amt01) {
  if (!SB) return;
  const float a = clampf(amt01, 0.0f, 1.0f);
  // Map [0..1] into the right half of the yaw range
  const float deg = NEUTRAL_YAW_DEG + a * (LIM_YAW.maxDeg - NEUTRAL_YAW_DEG);
  SB->writeDegrees(CH.yaw, deg);
}
void nod(float amt01) {
  if (!SB) return;
  const float a = clampf(amt01, 0.0f, 1.0f);
  // Positive -> pitch up from neutral
  const float deg = NEUTRAL_PITCH_DEG + a * (LIM_PITCH.maxDeg - NEUTRAL_PITCH_DEG);
  SB->writeDegrees(CH.pitch, deg);
}

// ---------------- Optional helpers ----------------
void setYaw01(float a01) {
  if (!SB) return;
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_YAW.minDeg + a * (LIM_YAW.maxDeg - LIM_YAW.minDeg);
  SB->writeDegrees(CH.yaw, deg);
}
void setPitch01(float a01) {
  if (!SB) return;
  const float a = clampf(a01, 0.0f, 1.0f);
  const float deg = LIM_PITCH.minDeg + a * (LIM_PITCH.maxDeg - LIM_PITCH.minDeg);
  SB->writeDegrees(CH.pitch, deg);
}

void nudgeYawDeg(float delta) {
  if (!SB) return;
  static float last = NEUTRAL_YAW_DEG;
  last = clampf(last + delta, LIM_YAW.minDeg, LIM_YAW.maxDeg);
  SB->writeDegrees(CH.yaw, last);
}
void nudgePitchDeg(float delta) {
  if (!SB) return;
  static float last = NEUTRAL_PITCH_DEG;
  last = clampf(last + delta, LIM_PITCH.minDeg, LIM_PITCH.maxDeg);
  SB->writeDegrees(CH.pitch, last);
}

void center() {
  if (!SB) return;
  SB->writeDegrees(CH.yaw,   NEUTRAL_YAW_DEG);
  SB->writeDegrees(CH.pitch, NEUTRAL_PITCH_DEG);
}

} // namespace Neck
