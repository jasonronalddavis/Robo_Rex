#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Spine {

struct Map {
  uint8_t spinePitch = 10;  // adjust in main.cpp to your wiring
};

// Init + attach with safe limits; moves to neutral.
void begin(ServoBus* bus, const Map& map);

// ===== Existing API (kept for compatibility) =====
void up();                         // quick preset “up” pose
void down();                       // quick preset “down” pose
void set(float level01);           // absolute [0..1] → mapped to safe range

// ===== Optional helpers (nice with new UI) =====
void setPitch01(float level01);    // alias of set()
void nudgePitchDeg(float delta);   // relative adjust (deg)
void center();                     // neutral spine

} // namespace Spine
