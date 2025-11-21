#include "Leg_Function.h"
#include <math.h>
//yaw
namespace Leg {

// ========== Internal State ==========
static ServoBus* SB = nullptr;    // Pointer to ServoBus (PCA9685)
static Map       CH;              // Channel mapping

// ========== Mechanical Configuration ==========
// Neutral positions for each joint (degrees)
// Adjust these to match your T-Rex's mechanical zero positions
static float NEUTRAL_HIP_X  = 90.0f;   // Hip X neutral (forward/back)
static float NEUTRAL_HIP_Y  = 90.0f;   // Hip Y neutral (up/down)
static float NEUTRAL_KNEE   = 90.0f;   // Knee neutral (bend)
static float NEUTRAL_ANKLE  = 90.0f;   // Ankle neutral (pivot)
static float NEUTRAL_FOOT   = 90.0f;   // Foot neutral (tilt)

// ========== Gait Parameters ==========
// Per-joint amplitude scales (degrees per unit amplitude)
// These control how much each joint moves during walking
static float HIPX_STRIDE_SCALE  = 50.0f;  // Hip X swing range (forward/back)
static float HIPY_LIFT_SCALE    = 35.0f;  // Hip Y lift range (up/down)
static float KNEE_STRIDE_SCALE  = 35.0f;  // Knee bend range
static float ANKLE_STRIDE_SCALE = 25.0f;  // Ankle rotation range
static float FOOT_LIFT_SCALE    = 20.0f;  // Foot lift height

// ========== Servo Limits ==========
// Safety limits per joint (µs pulse width / degrees)
// Adjust these to prevent mechanical binding or servo overtravel
static ServoLimits LIM_HIP_X (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_HIP_Y (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_KNEE  (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_ANKLE (500, 2500,  10.0f, 170.0f);
static ServoLimits LIM_FOOT  (500, 2500,  10.0f, 170.0f);

// ========== Gait State ==========
static Mode  g_mode = IDLE;          // Current locomotion mode

// Gait parameters with assertive defaults
static float g_speed_hz   = 1.0f;    // Walking speed (cycles per second)
static float g_stride_amp = 1.0f;    // Stride amplitude (0..1)
static float g_lift_amp   = 0.8f;    // Foot lift amount (0..1)
static float g_posture    = 0.5f;    // Overall stance height (0..1)

static uint32_t g_t0_ms = 0;         // Gait start timestamp

// ========== Helper Functions ==========

// Clamp float value to range
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Linear interpolation
static inline float lerp(float a, float b, float t) { 
  return a + (b - a) * t; 
}

// ========== Leg Control Functions ==========

// Write angles to right leg servos
// swing: -1.0 (back) to +1.0 (forward)
// lift: 0.0 (on ground) to 1.0 (lifted)
// posture01: 0.0 (crouch) to 1.0 (extend)
static void writeRightLeg(float swing, float lift, float posture01) {
  if (!SB) return;
  
  // Calculate joint angles based on gait parameters
  const float hipX  = NEUTRAL_HIP_X  + swing * (HIPX_STRIDE_SCALE  * g_stride_amp);
  const float hipY  = NEUTRAL_HIP_Y  - lift  * (HIPY_LIFT_SCALE    * g_lift_amp);
  const float knee  = NEUTRAL_KNEE   - lift  * (KNEE_STRIDE_SCALE  * g_lift_amp);
  const float ankle = NEUTRAL_ANKLE  - lift  * (ANKLE_STRIDE_SCALE * g_lift_amp);
  const float foot  = NEUTRAL_FOOT   - lift  * (FOOT_LIFT_SCALE    * g_lift_amp);

  // Apply posture trim (adjust overall height)
  const float trim = (posture01 - 0.5f) * 10.0f; // +/- ~5 degrees
  
  // Write to PCA9685 channels
  SB->writeDegrees(CH.R_hipX,  hipX  + trim);
  SB->writeDegrees(CH.R_hipY,  hipY  + trim);
  SB->writeDegrees(CH.R_knee,  knee  + trim);
  SB->writeDegrees(CH.R_ankle, ankle + trim);
  SB->writeDegrees(CH.R_foot,  foot  + trim);
}

// Write angles to left leg servos
// swing: -1.0 (back) to +1.0 (forward)
// lift: 0.0 (on ground) to 1.0 (lifted)
// posture01: 0.0 (crouch) to 1.0 (extend)
static void writeLeftLeg(float swing, float lift, float posture01) {
  if (!SB) return;
  
  // Calculate joint angles based on gait parameters
  const float hipX  = NEUTRAL_HIP_X  + swing * (HIPX_STRIDE_SCALE  * g_stride_amp);
  const float hipY  = NEUTRAL_HIP_Y  - lift  * (HIPY_LIFT_SCALE    * g_lift_amp);
  const float knee  = NEUTRAL_KNEE   - lift  * (KNEE_STRIDE_SCALE  * g_lift_amp);
  const float ankle = NEUTRAL_ANKLE  - lift  * (ANKLE_STRIDE_SCALE * g_lift_amp);
  const float foot  = NEUTRAL_FOOT   - lift  * (FOOT_LIFT_SCALE    * g_lift_amp);

  // Apply posture trim
  const float trim = (posture01 - 0.5f) * 10.0f;
  
  // Write to PCA9685 channels
  SB->writeDegrees(CH.L_hipX,  hipX  + trim);
  SB->writeDegrees(CH.L_hipY,  hipY  + trim);
  SB->writeDegrees(CH.L_knee,  knee  + trim);
  SB->writeDegrees(CH.L_ankle, ankle + trim);
  SB->writeDegrees(CH.L_foot,  foot  + trim);
}

// ========== Gait Wave Generator ==========
// Generate smooth walking motion using sine waves
// phase01: 0.0 to 1.0 (one complete gait cycle)
// swing_out: -1.0 to +1.0 (forward/back motion)
// lift_out: 0.0 to 1.0 (foot lift height)
static void gaitWave(float phase01, float& swing_out, float& lift_out) {
  const float theta = phase01 * TWO_PI; // TWO_PI from Arduino.h
  
  // Swing: sinusoidal forward/back motion
  const float swing = sinf(theta);      // -1..1
  
  // Lift: peaks at mid-swing for natural foot clearance
  float lift = 0.5f * (sinf(theta - PI/2) + 1.0f); // 0..1, peaks mid-swing
  lift = powf(lift, 1.2f); // Sharper peak for more ground contact time

  swing_out = clampf(swing, -1.0f, 1.0f);
  lift_out  = clampf(lift,   0.0f, 1.0f);
}

// ========== Public API Implementation ==========

// Initialize leg control system
void begin(ServoBus* bus, const Map& map) {
  SB = bus;
  CH = map;

  if (!SB) {
    Serial.println(F("[Leg] ERROR: ServoBus is nullptr!"));
    return;
  }

  // Attach all leg servos to PCA9685 with limits
  Serial.println(F("[Leg] Attaching servos..."));
  SB->attach(CH.R_hipX,  LIM_HIP_X);
  SB->attach(CH.R_hipY,  LIM_HIP_Y);
  SB->attach(CH.R_knee,  LIM_KNEE);
  SB->attach(CH.R_ankle, LIM_ANKLE);
  SB->attach(CH.R_foot,  LIM_FOOT);

  SB->attach(CH.L_hipX,  LIM_HIP_X);
  SB->attach(CH.L_hipY,  LIM_HIP_Y);
  SB->attach(CH.L_knee,  LIM_KNEE);
  SB->attach(CH.L_ankle, LIM_ANKLE);
  SB->attach(CH.L_foot,  LIM_FOOT);
  Serial.println(F("[Leg] All servos attached"));

  // Move to neutral stance
  Serial.println(F("[Leg] Moving to neutral stance..."));
  SB->writeDegrees(CH.R_hipX,  NEUTRAL_HIP_X);
  SB->writeDegrees(CH.R_hipY,  NEUTRAL_HIP_Y);
  SB->writeDegrees(CH.R_knee,  NEUTRAL_KNEE);
  SB->writeDegrees(CH.R_ankle, NEUTRAL_ANKLE);
  SB->writeDegrees(CH.R_foot,  NEUTRAL_FOOT);

  SB->writeDegrees(CH.L_hipX,  NEUTRAL_HIP_X);
  SB->writeDegrees(CH.L_hipY,  NEUTRAL_HIP_Y);
  SB->writeDegrees(CH.L_knee,  NEUTRAL_KNEE);
  SB->writeDegrees(CH.L_ankle, NEUTRAL_ANKLE);
  SB->writeDegrees(CH.L_foot,  NEUTRAL_FOOT);
  Serial.println(F("[Leg] Neutral stance complete"));

  // Initialize gait state
  g_mode  = IDLE;
  g_t0_ms = millis();

  Serial.println(F("[Leg] Initialized on PCA9685"));
  Serial.print(F("  Right leg: Ports "));
  Serial.print(CH.R_hipX); Serial.print(F(","));
  Serial.print(CH.R_hipY); Serial.print(F(","));
  Serial.print(CH.R_knee); Serial.print(F(","));
  Serial.print(CH.R_ankle); Serial.print(F(","));
  Serial.println(CH.R_foot);
  Serial.print(F("  Left leg:  Ports "));
  Serial.print(CH.L_hipX); Serial.print(F(","));
  Serial.print(CH.L_hipY); Serial.print(F(","));
  Serial.print(CH.L_knee); Serial.print(F(","));
  Serial.print(CH.L_ankle); Serial.print(F(","));
  Serial.println(CH.L_foot);
}

// ========== Locomotion Commands ==========

void walkForward(float speed_hz) { 
  g_mode = WALK_FWD; 
  g_speed_hz = clampf(speed_hz, 0.1f, 3.0f); 
  g_t0_ms = millis();
  Serial.print(F("[Leg] Walking forward at "));
  Serial.print(g_speed_hz);
  Serial.println(F(" Hz"));
}

void walkBackward(float speed_hz) { 
  g_mode = WALK_BWD; 
  g_speed_hz = clampf(speed_hz, 0.1f, 3.0f); 
  g_t0_ms = millis();
  Serial.print(F("[Leg] Walking backward at "));
  Serial.print(g_speed_hz);
  Serial.println(F(" Hz"));
}

void turnLeft(float rate_hz) { 
  g_mode = TURN_L; 
  g_speed_hz = clampf(rate_hz, 0.1f, 3.0f); 
  g_t0_ms = millis();
  Serial.print(F("[Leg] Turning left at "));
  Serial.print(g_speed_hz);
  Serial.println(F(" Hz"));
}

void turnRight(float rate_hz) { 
  g_mode = TURN_R; 
  g_speed_hz = clampf(rate_hz, 0.1f, 3.0f); 
  g_t0_ms = millis();
  Serial.print(F("[Leg] Turning right at "));
  Serial.print(g_speed_hz);
  Serial.println(F(" Hz"));
}

void stop() {
  g_mode = IDLE;
  writeRightLeg(0.0f, 0.0f, g_posture);
  writeLeftLeg(0.0f, 0.0f, g_posture);
  Serial.println(F("[Leg] Stopped - neutral stance"));
}

void emergencyStop() {
  g_mode = IDLE;
  if (!SB) return;
  writeRightLeg(0.0f, 0.0f, 0.5f);
  writeLeftLeg(0.0f, 0.0f, 0.5f);
  Serial.println(F("[Leg] EMERGENCY STOP"));
}

// ========== Gait Parameter Control ==========

void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode) {
  g_speed_hz   = clampf(speed_hz,   0.05f, 4.0f);
  g_stride_amp = clampf(stride_amp, 0.0f,  1.0f);
  g_lift_amp   = clampf(lift_amp,   0.0f,  1.0f);
  
  // Modify parameters for "run" mode
  if (mode == "run") {
    g_speed_hz = clampf(g_speed_hz * 1.3f, 0.1f, 5.0f); // 30% faster
    g_lift_amp = clampf(g_lift_amp * 0.8f, 0.0f, 1.0f); // 20% less lift
  }
  
  Serial.print(F("[Leg] Gait: "));
  Serial.print(g_speed_hz); Serial.print(F("Hz, stride="));
  Serial.print(g_stride_amp); Serial.print(F(", lift="));
  Serial.println(g_lift_amp);
}

void adjustSpeed(float d) { 
  g_speed_hz = clampf(g_speed_hz + d, 0.05f, 4.0f);
  Serial.print(F("[Leg] Speed adjusted to "));
  Serial.print(g_speed_hz);
  Serial.println(F(" Hz"));
}

void setStride(float v) { 
  g_stride_amp = clampf(v, 0.0f, 1.0f);
}

void setPosture(float v) { 
  g_posture = clampf(v, 0.0f, 1.0f);
}

// ========== Command Parser ==========

bool handleAction(const String& command, const String& phase) {
  // Stop command
  if (phase == "stop") { 
    stop(); 
    return true; 
  }
  
  // Start/hold phase - set default gait parameters
  if (phase == "start" || phase == "hold") {
    g_stride_amp = 1.0f;   // Full stride
    g_lift_amp   = 0.8f;   // 80% lift
    g_speed_hz   = 1.0f;   // 1 Hz walking speed
  }
  
  // Execute movement commands
  if (command == "move_forward")  { walkForward(g_speed_hz);  return true; }
  if (command == "move_backward") { walkBackward(g_speed_hz); return true; }
  if (command == "move_left")     { turnLeft(g_speed_hz);     return true; }
  if (command == "move_right")    { turnRight(g_speed_hz);    return true; }
  if (command == "stop")          { stop();                   return true; }
  
  // Unknown command
  return false;
}

// ========== Main Update Loop ==========

void tick() {
  if (!SB) return;

  // If idle, maintain neutral stance
  if (g_mode == IDLE) {
    writeRightLeg(0.0f, 0.0f, g_posture);
    writeLeftLeg(0.0f, 0.0f, g_posture);
    return;
  }

  // Calculate current phase in gait cycle
  const uint32_t now = millis();
  const float t = (now - g_t0_ms) / 1000.0f;  // Time in seconds
  const float cycle = t * g_speed_hz;         // Number of cycles completed
  float phase = cycle - floorf(cycle);        // Current phase (0.0 - 1.0)

  // Generate gait signals for each leg
  float swingR = 0, liftR = 0;
  float swingL = 0, liftL = 0;

  // Left leg is 180° out of phase with right leg
  float phaseL = phase + 0.5f;
  if (phaseL >= 1.0f) phaseL -= 1.0f;

  gaitWave(phase,  swingR, liftR);
  gaitWave(phaseL, swingL, liftL);

  // Modify gait based on current mode
  if (g_mode == WALK_BWD) {
    // Reverse swing direction for backward walking
    swingR = -swingR;
    swingL = -swingL;
  }
  else if (g_mode == TURN_L) {
    // Reduce left leg swing, increase right leg swing
    swingL *= 0.6f;
    swingR *= 1.2f;
  } 
  else if (g_mode == TURN_R) {
    // Increase left leg swing, reduce right leg swing
    swingL *= 1.2f;
    swingR *= 0.6f;
  }

  // Write calculated positions to servos
  writeRightLeg(swingR, liftR, g_posture);
  writeLeftLeg (swingL, liftL, g_posture);
}

// ========== State Query Functions ==========

Mode  mode()       { return g_mode; }
float speedHz()    { return g_speed_hz; }
float strideAmp()  { return g_stride_amp; }
float liftAmp()    { return g_lift_amp; }
float posture01()  { return g_posture; }

} // namespace Leg