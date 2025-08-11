#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Tail {

struct Map {
  uint8_t wag = 14;   // single DOF: tail yaw (left/right)
};

// Init + attach with safe limits; moves to neutral.
void begin(ServoBus* bus, const Map& map);

// ===== Existing API (kept for compatibility) =====
void set(float amt01);  // absolute [0..1] mapped around neutral (legacy)
void wag();             // quick 3x wag pattern

// ===== Optional helpers (nice with new UI/CommandRouter) =====
void setYaw01(float a01);       // absolute [0..1] → yaw within limits
void nudgeYawDeg(float delta);  // relative adjust (deg), great for “hold”
void center();                  // neutral tail

} // namespace Tail
