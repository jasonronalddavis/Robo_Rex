#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Leg {

// ========== PCA9685 Channel Mapping Structure ==========
// Assign these to your PCA9685 channels (6-15) in main.cpp
struct Map {
  // Right leg servos (5 servos) - Channels 6-10
  uint8_t R_hipX   = 7;    // PCA9685 channel 6  - Right hip forward/back
  uint8_t R_hipY   = 8;    // PCA9685 channel 7  - Right hip up/down
  uint8_t R_knee   = 9;    // PCA9685 channel 8  - Right knee bend
  uint8_t R_ankle  = 10;    // PCA9685 channel 9  - Right ankle pivot
  uint8_t R_foot   = 11;   // PCA9685 channel 10 - Right foot tilt
  
  // Left leg servos (5 servos) - Channels 11-15
  uint8_t L_hipX   = 12;   // PCA9685 channel 11 - Left hip forward/back
  uint8_t L_hipY   = 13;   // PCA9685 channel 12 - Left hip up/down
  uint8_t L_knee   = 14;   // PCA9685 channel 13 - Left knee bend
  uint8_t L_ankle  = 15;   // PCA9685 channel 14 - Left ankle pivot
  uint8_t L_foot   = 16;   // PCA9685 channel 15 - Left foot tilt
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
// Initialize leg servos with ServoBus (PCA9685 control)
// Must be called before any other leg functions
void begin(ServoBus* bus, const Map& map);

// ========== High-Level Locomotion Commands ==========
// Start walking forward at specified speed
// speed_hz: cycles per second (0.1 - 3.0 Hz, typical: 1.0 Hz)
void walkForward(float speed_hz);

// Start walking backward at specified speed
// speed_hz: cycles per second (0.1 - 3.0 Hz, typical: 1.0 Hz)
void walkBackward(float speed_hz);

// Start turning left (rotate counterclockwise)
// rate_hz: rotation speed in cycles/sec (0.1 - 3.0 Hz)
void turnLeft(float rate_hz);

// Start turning right (rotate clockwise)
// rate_hz: rotation speed in cycles/sec (0.1 - 3.0 Hz)
void turnRight(float rate_hz);

// Stop walking and return to neutral stance (smooth transition)
void stop();

// Emergency stop - immediate return to neutral (no smoothing)
void emergencyStop();

// ========== Gait Parameter Tuning ==========
// Configure walking gait parameters
// speed_hz: walking speed (0.05 - 4.0 Hz)
// stride_amp: stride amplitude 0.0-1.0 (0.0 = no movement, 1.0 = full stride)
// lift_amp: foot lift height 0.0-1.0 (0.0 = no lift, 1.0 = maximum lift)
// mode: "walk" (normal) or "run" (faster, less lift)
void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode = "walk");

// Adjust current walking speed by delta
// delta_hz: speed change in Hz (positive = faster, negative = slower)
void adjustSpeed(float delta_hz);

// Set stride amplitude (how far legs swing forward/back)
// value01: 0.0 = minimal stride, 1.0 = full stride
void setStride(float value01);

// Set overall posture/stance height
// level01: 0.0 = crouched, 0.5 = neutral, 1.0 = extended
void setPosture(float level01);

// ========== Command Parsing Helper ==========
// Maps string commands to locomotion functions
// command: "move_forward", "move_backward", "move_left", "move_right", "stop"
// phase: "start", "hold", "stop"
// Returns: true if command was recognized and executed
bool handleAction(const String& command, const String& phase);

// ========== Main Update Loop ==========
// Must be called regularly from loop() to generate walking motion
// Typical call rate: 20-50 Hz (every 20-50ms)
void tick();

// ========== State Query Functions ==========
// Get current locomotion mode
Mode mode();

// Get current walking speed in Hz
float speedHz();

// Get current stride amplitude (0.0 - 1.0)
float strideAmp();

// Get current foot lift amplitude (0.0 - 1.0)
float liftAmp();

// Get current posture level (0.0 - 1.0)
float posture01();

} // namespace Leg