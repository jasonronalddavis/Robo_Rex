// Spine_Function.cpp
#include "Spine_Function.h"
static ServoBus* SB=nullptr; static Spine::Map MAP;

namespace Spine {
void begin(ServoBus* bus, const Map& map) {
  SB = bus; MAP = map;
  SB->attach(MAP.spinePitch, ServoLimits{500,2500,0,180});
}
void up()   { SB->writeDegrees(MAP.spinePitch, 120); }
void down() { SB->writeDegrees(MAP.spinePitch, 60); }
void set(float level01) {
  float v = constrain(level01, 0, 1);
  SB->writeDegrees(MAP.spinePitch, 60 + v*60);
}
}
