#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Pelvis {

struct Map {
  uint8_t roll = 15;   // single DOF: pelvis roll / level
};

// Init + attach with safe limits; moves to neutral.
void begin(ServoBus* bus, const Map& map);

// ===== Existing API (kept for compatibility) =====
// level01 ∈ [0..1] → mapped to a safe roll range around neutral.
void set(float level01);

// Feedforward from IMU: pass normalized level for closed-loop leveling.
// (You’re already calling this from main.cpp.)
void stabilize(float rollLevel01);

// ===== Optional helpers (nice with new UI/CommandRouter) =====
void setLevel01(float level01);    // same as set()
void nudgeRollDeg(float delta);    // relative adjust in degrees
void center();                     // neutral pelvis

} // namespace Pelvis
