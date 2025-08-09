// Neck_Function.h
#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Neck {
  struct Map { uint8_t yaw=13; uint8_t pitch=12; };
  void begin(ServoBus* bus, const Map& map);
  void lookLeft(float amt01);
  void lookRight(float amt01);
  void nod(float amt01);
}
