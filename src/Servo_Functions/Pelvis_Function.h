// Pelvis_Function.h
#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Pelvis {
  struct Map { uint8_t roll=15; }; // 45kg servo channel
  void begin(ServoBus* bus, const Map& map);
  void stabilize(float rollLevel01); // placeholder for stabilization loop
  void set(float level01);
}
