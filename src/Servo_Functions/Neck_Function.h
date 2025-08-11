#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Neck {

struct Map {
  uint8_t yaw   = 13; // left/right
  uint8_t pitch = 12; // up/down (if you prefer to keep pitch in Head, leave map unused)
};

// Init + attach with safe limits; moves to neutral.
void begin(ServoBus* bus, const Map& map);

// ---- Existing API (kept for compatibility) ----
// amt01 is clamped to [0..1]
void lookLeft(float amt01);
void lookRight(float amt01);
void nod(float amt01); // positive -> nod “yes” (pitch up from neutral)

// ---- Optional helpers (nice with your new UI) ----
void setYaw01(float a01);        // absolute yaw  [0..1] within limits
void setPitch01(float a01);      // absolute pitch[0..1] within limits
void nudgeYawDeg(float delta);   // relative yaw adjust (deg)
void nudgePitchDeg(float delta); // relative pitch adjust (deg)
void center();                   // neutral yaw/pitch

} // namespace Neck
