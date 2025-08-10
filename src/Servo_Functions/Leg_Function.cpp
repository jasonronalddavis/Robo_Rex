#include "Leg_Function.h"
#include <math.h>
namespace Leg {

static ServoBus* SB = nullptr;
static Map       CH;

// ----------------- Tunables (match to your mechanics) -----------------
static float NEUTRAL_HIP   = 90.0f;   // deg
static float NEUTRAL_KNEE  = 90.0f;
static float NEUTRAL_ANKLE = 90.0f;
static float NEUTRAL_FOOT  = 90.0f;
static float NEUTRAL_TOE   = 90.0f;

// Per-joint amplitude scales (deg per unit amplitude)
static float HIP_STRIDE_SCALE   = 25.0f;  // swing fore/aft
static float KNEE_STRIDE_SCALE  = 18.0f;  // extend/flex
static float ANKLE_STRIDE_SCALE = 12.0f;
static float FOOT_LIFT_SCALE    = 10.0f;  // lift during swing
static float TOE_LIFT_SCALE     =  6.0f;

// Safety limits per joint (µs/deg). Adjust for your horns/geometry.
static ServoLimits LIM_HIP   (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_KNEE  (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_ANKLE (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_FOOT  (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_TOE   (500, 2500,  10.0f, 170.0f);

// ----------------- Internal gait state -----------------
enum Mode { IDLE, WALK_FWD, WALK_BWD, TURN_L, TURN_R };
static Mode  g_mode = IDLE;

static float g_speed_hz   = 0.7f;  // cycles per second
static float g_stride_amp = 0.6f;  // 0..1 (swing amplitude)
static float g_lift_amp   = 0.4f;  // 0..1 (foot/toe lift)
static float g_posture    = 0.5f;  // 0..1 overall stance (height trim)

static uint32_t g_t0_ms = 0;

// Helpers
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}
static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Convert normalized [-1..1] swing/lift signals into joint angles
static void writeLeftLeg(float swing, float lift, float posture01) {
  // swing: fore/aft (hips), lift: up during swing (knee/ankle/toe/foot)
  const float hip   = NEUTRAL_HIP   + swing * (HIP_STRIDE_SCALE   * g_stride_amp);
  const float knee  = NEUTRAL_KNEE  - lift  * (KNEE_STRIDE_SCALE  * g_lift_amp);
  const float ankle = NEUTRAL_ANKLE - lift  * (ANKLE_STRIDE_SCALE * g_lift_amp);
  const float foot  = NEUTRAL_FOOT  - lift  * (FOOT_LIFT_SCALE    * g_lift_amp);
  const float toe   = NEUTRAL_TOE   - lift  * (TOE_LIFT_SCALE     * g_lift_amp);

  // Posture trim (lower/raise body by biasing joints slightly)
  const float trim = (posture01 - 0.5f) * 10.0f; // +/- ~5 deg
  SB->writeDegrees(CH.L_hip,   hip   + trim);
  SB->writeDegrees(CH.L_knee,  knee  + trim);
  SB->writeDegrees(CH.L_ankle, ankle + trim);
  SB->writeDegrees(CH.L_foot,  foot  + trim);
  SB->writeDegrees(CH.L_toe,   toe   + trim);
}

static void writeRightLeg(float swing, float lift, float posture01) {
  // Mirror the left; adjust signs if your mechanics require
  const float hip   = NEUTRAL_HIP   + swing * (HIP_STRIDE_SCALE   * g_stride_amp);
  const float knee  = NEUTRAL_KNEE  - lift  * (KNEE_STRIDE_SCALE  * g_lift_amp);
  const float ankle = NEUTRAL_ANKLE - lift  * (ANKLE_STRIDE_SCALE * g_lift_amp);
  const float foot  = NEUTRAL_FOOT  - lift  * (FOOT_LIFT_SCALE    * g_lift_amp);
  const float toe   = NEUTRAL_TOE   - lift  * (TOE_LIFT_SCALE     * g_lift_amp);

  const float trim = (g_posture - 0.5f) * 10.0f;
  SB->writeDegrees(CH.R_hip,   hip   + trim);
  SB->writeDegrees(CH.R_knee,  knee  + trim);
  SB->writeDegrees(CH.R_ankle, ankle + trim);
  SB->writeDegrees(CH.R_foot,  foot  + trim);
  SB->writeDegrees(CH.R_toe,   toe   + trim);
}

// Generate swing (+/-) and lift (0..1) signals from phase
static void gaitWave(float phase01, float& swing_out, float& lift_out) {
  // phase 0..1 → swing is sine, lift peaks during swing forward
  const float theta = phase01 * TWO_PI; // TWO_PI from Arduino.h
  const float swing = sinf(theta);      // -1..1
  // Simple duty: lift = max(0, sin shifted up), tweak shape with pow
  float lift = 0.5f * (sinf(theta - PI/2) + 1.0f); // 0..1, peaks mid-swing
  lift = powf(lift, 1.2f); // a bit pointier

  swing_out = clampf(swing, -1.0f, 1.0f);
  lift_out  = clampf(lift,   0.0f, 1.0f);
}

// ----------------- Public API -----------------
void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;

  // Attach with limits (tweak per joint if needed)
  if (SB) {
    SB->attach(CH.L_hip,   LIM_HIP);
    SB->attach(CH.L_knee,  LIM_KNEE);
    SB->attach(CH.L_ankle, LIM_ANKLE);
    SB->attach(CH.L_foot,  LIM_FOOT);
    SB->attach(CH.L_toe,   LIM_TOE);

    SB->attach(CH.R_hip,   LIM_HIP);
    SB->attach(CH.R_knee,  LIM_KNEE);
    SB->attach(CH.R_ankle, LIM_ANKLE);
    SB->attach(CH.R_foot,  LIM_FOOT);
    SB->attach(CH.R_toe,   LIM_TOE);

    // Move to neutral stance
    SB->writeDegrees(CH.L_hip,   NEUTRAL_HIP);
    SB->writeDegrees(CH.L_knee,  NEUTRAL_KNEE);
    SB->writeDegrees(CH.L_ankle, NEUTRAL_ANKLE);
    SB->writeDegrees(CH.L_foot,  NEUTRAL_FOOT);
    SB->writeDegrees(CH.L_toe,   NEUTRAL_TOE);

    SB->writeDegrees(CH.R_hip,   NEUTRAL_HIP);
    SB->writeDegrees(CH.R_knee,  NEUTRAL_KNEE);
    SB->writeDegrees(CH.R_ankle, NEUTRAL_ANKLE);
    SB->writeDegrees(CH.R_foot,  NEUTRAL_FOOT);
    SB->writeDegrees(CH.R_toe,   NEUTRAL_TOE);
  }
  g_mode = IDLE;
  g_t0_ms = millis();
}

void walkForward(float speed_hz) {
  g_mode = WALK_FWD;
  g_speed_hz = clampf(speed_hz, 0.1f, 3.0f);
  g_t0_ms = millis();
}

void walkBackward(float speed_hz) {
  g_mode = WALK_BWD;
  g_speed_hz = clampf(speed_hz, 0.1f, 3.0f);
  g_t0_ms = millis();
}

void turnLeft(float rate) {
  g_mode = TURN_L;
  g_speed_hz = clampf(rate, 0.1f, 3.0f);
  g_t0_ms = millis();
}

void turnRight(float rate) {
  g_mode = TURN_R;
  g_speed_hz = clampf(rate, 0.1f, 3.0f);
  g_t0_ms = millis();
}

void stop() {
  g_mode = IDLE;
  // return to neutral gently
  writeLeftLeg(0.0f, 0.0f, g_posture);
  writeRightLeg(0.0f, 0.0f, g_posture);
}

void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode) {
  g_speed_hz   = clampf(speed_hz,   0.05f, 4.0f);
  g_stride_amp = clampf(stride_amp, 0.0f,  1.0f);
  g_lift_amp   = clampf(lift_amp,   0.0f,  1.0f);

  if (mode == "run") {
    // example: slightly higher frequency & lower lift for “run”
    g_speed_hz   = clampf(g_speed_hz * 1.3f, 0.1f, 5.0f);
    g_lift_amp   = clampf(g_lift_amp * 0.8f, 0.0f, 1.0f);
  }
  // keep current mode; caller can switch via walkForward/Backward/turn
}

void adjustSpeed(float delta_hz) {
  g_speed_hz = clampf(g_speed_hz + delta_hz, 0.05f, 4.0f);
}

void setStride(float value) {
  g_stride_amp = clampf(value, 0.0f, 1.0f);
}

void setPosture(float level01) {
  g_posture = clampf(level01, 0.0f, 1.0f);
}

// Call this regularly (e.g., each loop())
void tick() {
  if (!SB) return;

  const uint32_t now = millis();
  const float t = (now - g_t0_ms) / 1000.0f;  // seconds
  const float cycle = t * g_speed_hz;         // cycles since start
  float phase = cycle - floorf(cycle);        // 0..1

  // Default swings/lifts for left/right
  float swingL = 0, liftL = 0;
  float swingR = 0, liftR = 0;

  // Phase relationship: walking has legs out of phase by 180°
  float phaseR = phase + 0.5f;
  if (phaseR >= 1.0f) phaseR -= 1.0f;

  // Base gait
  gaitWave(phase,  swingL, liftL);
  gaitWave(phaseR, swingR, liftR);

  // Reverse for backward
  if (g_mode == WALK_BWD) {
    swingL = -swingL;
    swingR = -swingR;
  }

  // Turning tweaks: bias hip swing to steer
  if (g_mode == TURN_L) {
    // more forward swing on right leg, less on left
    swingL *= 0.6f;
    swingR *= 1.2f;
  } else if (g_mode == TURN_R) {
    swingL *= 1.2f;
    swingR *= 0.6f;
  }

  writeLeftLeg (swingL, liftL, g_posture);
  writeRightLeg(swingR, liftR, g_posture);
}

} // namespace Leg