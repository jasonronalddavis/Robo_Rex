// Tail_Function.h
#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Tail {
  struct Map { uint8_t wag=14; };
  void begin(ServoBus* bus, const Map& map);
  void wag();
  void set(float amt01);
}
