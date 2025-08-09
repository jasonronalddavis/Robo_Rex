#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Leg {
  // Example channel mapping (edit to your wiring)
  // Left leg: hip=0, knee=1, ankle=2, foot=3, toe=4
  // Right leg: hip=5, knee=6, ankle=7, foot=8, toe=9
  struct Map {
    uint8_t L_hip=0, L_knee=1, L_ankle=2, L_foot=3, L_toe=4;
    uint8_t R_hip=5, R_knee=6, R_ankle=7, R_foot=8, R_toe=9;
  };

  void begin(ServoBus* bus, const Map& map);
  void stop();
  void walkForward(float speed01);
  void walkBackward(float speed01);
  void turnLeft(float rate01);
  void turnRight(float rate01);
  void setGait(float speed01, float stride01, float lift01, const String& mode);
  void setStride(float val01);
  void adjustSpeed(float delta);
  void setPosture(float level01);
}
