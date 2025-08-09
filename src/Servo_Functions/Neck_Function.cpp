// Neck_Function.cpp
#include "Neck_Function.h"
static ServoBus* SB=nullptr; static Neck::Map MAP;

namespace Neck {
void begin(ServoBus* bus, const Map& map) {
  SB=bus; MAP=map;
  SB->attach(MAP.yaw, ServoLimits{500,2500,0,180});
  SB->attach(MAP.pitch, ServoLimits{500,2500,0,180});
}
void lookLeft(float amt01)  { SB->writeDegrees(MAP.yaw, 90 - 40*constrain(amt01,0,1)); }
void lookRight(float amt01) { SB->writeDegrees(MAP.yaw, 90 + 40*constrain(amt01,0,1)); }
void nod(float amt01)       { SB->writeDegrees(MAP.pitch, 90 + 20*constrain(amt01,0,1)); }
}
