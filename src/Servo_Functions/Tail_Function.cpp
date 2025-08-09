// Tail_Function.cpp
#include "Tail_Function.h"

static ServoBus* SB=nullptr; static Tail::Map MAP;

namespace Tail {
void begin(ServoBus* bus, const Map& map) {
  SB=bus; MAP=map;
SB->attach(MAP.wag, ServoLimits{ (uint16_t)500, (uint16_t)2500, 0.0f, 180.0f });
}
void set(float amt01) {
  SB->writeDegrees(MAP.wag, 90 + 30*constrain(amt01,0,1));
}
void wag() {
  for (int i=0;i<3;i++){ set(1); delay(150); set(0); delay(150); }
}
}
