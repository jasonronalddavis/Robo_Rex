#include "Leg_Function.h"

static ServoBus* SB = nullptr;
static Leg::Map MAP;
static float g_speed = 0.6f;
static float g_stride = 0.6f;
static float g_lift = 0.4f;

namespace Leg {

void begin(ServoBus* bus, const Map& map) {
  SB = bus; MAP = map;
  // Attach with safe limits (tune per joint!)
  ServoLimits lim{500, 2500, 0, 180};
  SB->attach(MAP.L_hip, lim);   SB->attach(MAP.L_knee, lim);
  SB->attach(MAP.L_ankle, lim); SB->attach(MAP.L_foot, lim);
  SB->attach(MAP.L_toe, lim);
  SB->attach(MAP.R_hip, lim);   SB->attach(MAP.R_knee, lim);
  SB->attach(MAP.R_ankle, lim); SB->attach(MAP.R_foot, lim);
  SB->attach(MAP.R_toe, lim);
}

void stop() {
  // Simple: freeze current positions (do nothing) or set neutral pose
  // Example neutral:
  SB->writeDegrees(MAP.L_hip, 90); SB->writeDegrees(MAP.R_hip, 90);
}

void walkForward(float speed01) {
  g_speed = constrain(speed01, 0, 1);
  // SUPER SIMPLE placeholder gait: swing hips opposite phase
  // Replace with your full IK/trajectory later
  for (int i=0; i<20; ++i) {
    float a = 90 + 20 * g_speed;   // hip forward
    float b = 90 - 20 * g_speed;   // hip backward
    SB->writeDegrees(MAP.L_hip, a);
    SB->writeDegrees(MAP.R_hip, b);
    delay(40);
    SB->writeDegrees(MAP.L_hip, b);
    SB->writeDegrees(MAP.R_hip, a);
    delay(40);
  }
}

void walkBackward(float speed01) {
  walkForward(speed01); // replace with backward phasing
}

void turnLeft(float rate01) {
  float r = constrain(rate01, 0, 1);
  SB->writeDegrees(MAP.L_hip, 90 - 15*r);
  SB->writeDegrees(MAP.R_hip, 90 + 15*r);
}

void turnRight(float rate01) {
  float r = constrain(rate01, 0, 1);
  SB->writeDegrees(MAP.L_hip, 90 + 15*r);
  SB->writeDegrees(MAP.R_hip, 90 - 15*r);
}

void setGait(float speed01, float stride01, float lift01, const String& mode) {
  g_speed = constrain(speed01, 0, 1);
  g_stride = constrain(stride01, 0, 1);
  g_lift = constrain(lift01, 0, 1);
  (void)mode;
}

void setStride(float val01) { g_stride = constrain(val01, 0, 1); }
void adjustSpeed(float delta) { g_speed = constrain(g_speed + delta, 0, 1); }
void setPosture(float level01) {
  float v = constrain(level01, 0, 1);
  SB->writeDegrees(MAP.L_knee, 90 + 20*(1-v));
  SB->writeDegrees(MAP.R_knee, 90 + 20*(1-v));
}

} // namespace Leg
