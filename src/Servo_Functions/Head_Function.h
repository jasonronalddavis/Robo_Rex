#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Head {

// ========== Pin Mapping Structure ==========
struct Map {
  uint8_t jaw = 3;           // PCA9685 channel 3 - jaw open/close
  uint8_t neckPitch = 4;     // PCA9685 channel 4 - head up/down
};

// ========== Initialization ==========
// Initialize with ServoBus (PCA9685 control)
void begin(ServoBus* bus, const Map& map);

// ========== Jaw Control ==========
// Open mouth fully
void mouthOpen();

// Close mouth fully
void mouthClose();

// Set jaw position (0.0 = closed, 1.0 = open)
void setJaw01(float amt01);

// ========== Head Pitch Control ==========
// Look up by specified amount (0.0 = neutral, 1.0 = full up)
void lookUp(float amt01);

// Look down by specified amount (0.0 = neutral, 1.0 = full down)
void lookDown(float amt01);

// Set head pitch directly (0.0 = full down, 1.0 = full up)
void setPitch01(float amt01);

// ========== Animation Sequences ==========
// Roar animation (jaw opens and closes with head movement)
void roar();

// Snap animation (quick jaw open and close)
void snap();

// ========== Utility Functions ==========
// Move head to neutral/center position
void center();

// Nudge jaw by relative angle
void nudgeJawDeg(float deltaDegrees);

// Nudge pitch by relative angle
void nudgePitchDeg(float deltaDegrees);

} // namespace Head