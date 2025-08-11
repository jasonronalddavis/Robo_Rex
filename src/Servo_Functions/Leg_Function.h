#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Leg {

// ----------------- Channel Map -----------------
// Assign these indexes to your PCA9685 channels in main.cpp.
struct Map {
  uint8_t L_hip   = 0;
  uint8_t L_knee  = 1;
  uint8_t L_ankle = 2;
  uint8_t L_foot  = 3;
  uint8_t L_toe   = 4;

  uint8_t R_hip   = 5;
  uint8_t R_knee  = 6;
  uint8_t R_ankle = 7;
  uint8_t R_foot  = 8;
  uint8_t R_toe   = 9;
};

// High‑level locomotion modes
enum Mode : uint8_t { IDLE = 0, WALK_FWD, WALK_BWD, TURN_L, TURN_R };

// --------------- Lifecycle / Setup ---------------
void begin(ServoBus* bus, const Map& map);

// --------------- High‑level control ---------------
void walkForward(float speed_hz);
void walkBackward(float speed_hz);
void turnLeft(float rate_hz);
void turnRight(float rate_hz);
void stop();               // gracefully return to neutral
void emergencyStop();      // immediate neutral (no smoothing)

// Gait tuning (safe ranges are enforced)
void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode = "walk");
void adjustSpeed(float delta_hz);
void setStride(float value01);
void setPosture(float level01);

// Optional: single entry‑point that maps `{command, phase}` to the above.
// Returns true if recognized.
bool handleAction(const String& command, const String& phase);

// Call regularly from loop()
void tick();

// --------------- State queries (optional) ---------------
Mode  mode();
float speedHz();
float strideAmp();
float liftAmp();
float posture01();

} // namespace Leg
