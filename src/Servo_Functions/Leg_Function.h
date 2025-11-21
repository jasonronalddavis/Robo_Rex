#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Leg {

// ========== Servo Port Mapping Structure ==========
// All leg servos use PCA9685 (ports 6-15, direct mapping)
struct Map {
  // Right leg servos (5 servos) - PCA9685 ports 6-10
  uint8_t R_hipX   = 6;   // PCA9685 port 6  - Right hip forward/back
  uint8_t R_hipY   = 7;   // PCA9685 port 7  - Right hip up/down
  uint8_t R_knee   = 8;   // PCA9685 port 8  - Right knee bend
  uint8_t R_ankle  = 9;   // PCA9685 port 9  - Right ankle pivot
  uint8_t R_foot   = 10;  // PCA9685 port 10 - Right foot tilt

  // Left leg servos (5 servos) - PCA9685 ports 11-15
  uint8_t L_hipX   = 11;  // PCA9685 port 11 - Left hip forward/back
  uint8_t L_hipY   = 12;  // PCA9685 port 12 - Left hip up/down
  uint8_t L_knee   = 13;  // PCA9685 port 13 - Left knee bend
  uint8_t L_ankle  = 14;  // PCA9685 port 14 - Left ankle pivot
  uint8_t L_foot   = 15;  // PCA9685 port 15 - Left foot tilt
};

// ========== Locomotion Modes ==========
// High-level walking states
enum Mode : uint8_t { 
  IDLE = 0,      // Standing still, neutral stance
  WALK_FWD,      // Walking forward
  WALK_BWD,      // Walking backward
  TURN_L,        // Turning left (rotating in place)
  TURN_R         // Turning right (rotating in place)
};

// ========== Initialization ==========
// Initialize leg servos with ServoBus (GPIO control)
// Must be called before any other leg functions
void begin(ServoBus* bus, const Map& map);

// ========== High-Level Locomotion Commands ==========
void walkForward(float speed_hz);
void walkBackward(float speed_hz);
void turnLeft(float rate_hz);
void turnRight(float rate_hz);
void stop();
void emergencyStop();

// ========== Gait Parameter Tuning ==========
void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode = "walk");
void adjustSpeed(float delta_hz);
void setStride(float value01);
void setPosture(float level01);

// ========== Command Parsing Helper ==========
bool handleAction(const String& command, const String& phase);

// ========== Main Update Loop ==========
void tick();

// ========== State Query Functions ==========
Mode  mode();        // Get current locomotion mode
float speedHz();     // Current walking speed in Hz
float strideAmp();   // Current stride amplitude (0.0 - 1.0)
float liftAmp();     // Current foot lift amplitude (0.0 - 1.0)
float posture01();   // Current posture level (0.0 - 1.0)

} // namespace Leg