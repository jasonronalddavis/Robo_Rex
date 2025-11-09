#pragma once
#include <Arduino.h>

namespace CommandRouter {

// Initialize internal state and set neutral positions (call from setup()).
void begin();

// Handle a single newline-delimited message (JSON or legacy string).
// New JSON contract (preferred):
//   {"target":"legsPelvis","part":"legs","command":"move_forward","phase":"start"}
// Legacy JSON:
//   {"cmd":"rex_walk_forward","speed":0.7}
// Legacy raw string:
//   rex_roar
void handleLine(const String& line);

// Optional periodic work (not required, here for future smoothing).
void tick();

} // namespace CommandRouter