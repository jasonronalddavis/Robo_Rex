#include "Head_Function.h"

namespace Head {

static ServoBus* SB = nullptr;
static Map       CH;

// Limits (tune for your hardware)
static const ServoLimits LIM_JAW       (500, 2500,   0.0f, 180.0f);
static const ServoLimits LIM_NECK_PITCH(500, 2500,  20.0f, 160.0f); // keep some headroom

// Neutral targets
static const float NEUTRAL_JAW_DEG   = 70.0f;  // mouth comfortably closed
static const float OPEN_JAW_DEG      = 140.0f; // a nice “roar” open
static const float NEUTRAL_PITCH_DEG = 90.0f;  // look straight ahead-ish

// Clamp helpers
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;
  if (!SB) return;

  SB->attach(CH.jaw,       LIM_JAW);
  SB->attach(CH.neckPitch, LIM_NECK_PITCH);

  // Move to neutral/closed
  SB->writeDegrees(CH.jaw,       NEUTRAL_JAW_DEG);
  SB->writeDegrees(CH.neckPitch, NEUTRAL_PITCH_DEG);
}

// ------------- Existing API -------------
void mouthOpen()  { if (SB) SB->writeDegrees(CH.jaw, OPEN_JAW_DEG); }
void mouthClose() { if (SB) SB->writeDegrees(CH.jaw, NEUTRAL_JAW_DEG); }

void roar() {
  mouthOpen();
  delay(250);
  mouthClose();
}

// ------------- Optional helpers -------------
void setPitch01(float level01) {
  if (!SB) return;
  const float a = clampf(level01, 0.0f, 1.0f);
  const float deg = LIM_NECK_PITCH.minDeg + a * (LIM_NECK_PITCH.maxDeg - LIM_NECK_PITCH.minDeg);
  SB->writeDegrees(CH.neckPitch, deg);
}

void nudgePitchDeg(float delta) {
  if (!SB) return;
  // We don't have a cached position; just read back our last command by keeping a static.
  static float lastDeg = NEUTRAL_PITCH_DEG;
  lastDeg = clampf(lastDeg + delta, LIM_NECK_PITCH.minDeg, LIM_NECK_PITCH.maxDeg);
  SB->writeDegrees(CH.neckPitch, lastDeg);
}

void center() {
  if (!SB) return;
  SB->writeDegrees(CH.jaw,       NEUTRAL_JAW_DEG);
  SB->writeDegrees(CH.neckPitch, NEUTRAL_PITCH_DEG);
}

} // namespace Head
