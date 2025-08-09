// Pelvis_Function.cpp
#include "Pelvis_Function.h"
static ServoBus* SB=nullptr; static Pelvis::Map MAP;

namespace Pelvis {
void begin(ServoBus* bus, const Map& map) {
  SB=bus; MAP=map;
  SB->attach(MAP.roll, ServoLimits{700,2400,0,180});
}
void set(float level01) {
  SB->writeDegrees(MAP.roll, 90 + 20*constrain(level01,0,1));
}
void stabilize(float rollLevel01) {
  set(rollLevel01); // replace with IMU feedback later
}
}
