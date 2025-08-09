// Head_Function.cpp
#include "Head_Function.h"
static ServoBus* SB=nullptr; static Head::Map MAP;

namespace Head {
void begin(ServoBus* bus, const Map& map) {
  SB = bus; MAP = map;
  SB->attach(MAP.jaw, ServoLimits{500,2500,0,180});
  SB->attach(MAP.neckPitch, ServoLimits{500,2500,0,180});
}
void mouthOpen()  { SB->writeDegrees(MAP.jaw, 140); }
void mouthClose() { SB->writeDegrees(MAP.jaw, 70); }
void roar() {
  mouthOpen(); delay(250); mouthClose();
}
}

