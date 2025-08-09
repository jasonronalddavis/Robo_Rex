// Head_Function.h
#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Head {
  struct Map { uint8_t jaw=11; uint8_t neckPitch=12; };
  void begin(ServoBus* bus, const Map& map);
  void roar();
  void mouthOpen();
  void mouthClose();
}
