#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Leg {

struct Map {
  // Left leg channels
  uint8_t L_hip;
  uint8_t L_knee;
  uint8_t L_ankle;
  uint8_t L_foot;
  uint8_t L_toe;

  // Right leg channels
  uint8_t R_hip;
  uint8_t R_knee;
  uint8_t R_ankle;
  uint8_t R_foot;
  uint8_t R_toe;
};

void begin(ServoBus* bus, const Map& map);

// High-level motions
void walkForward(float speed_hz);
void walkBackward(float speed_hz);
void turnLeft(float rate);
void turnRight(float rate);
void stop();

// Parameter adjustment
void setGait(float speed_hz, float stride_amp, float lift_amp, const String& mode);
void adjustSpeed(float delta_hz);
void setStride(float value);      // 0..1 suggested
void setPosture(float level01);   // overall height/stance trim 0..1

// Should be called in loop() if you want the gait engine to run continuously
void tick();

} // namespace Leg