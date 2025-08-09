// Spine_Function.h
#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Spine {
  struct Map { uint8_t spinePitch = 10; /* edit channel */ };
  void begin(ServoBus* bus, const Map& map);
  void up();
  void down();
  void set(float level01); // -1..+1 or 0..1 as you prefer, here 0..1
}
