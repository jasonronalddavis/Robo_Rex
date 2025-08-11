#pragma once
#include <Arduino.h>
#include "../ServoBus.h"

namespace Head {

struct Map {
  uint8_t jaw        = 11; // adjust in main.cpp to match wiring
  uint8_t neckPitch  = 12; // pitch (up/down)
};

// Initialize and attach channels with safe defaults
void begin(ServoBus* bus, const Map& map);

// Existing API
void mouthOpen();
void mouthClose();
void roar();               // simple open->delay->close

// New helpers (optional but convenient)
void setPitch01(float level01);   // absolute [0..1] -> degrees within limits
void nudgePitchDeg(float delta);  // relative change in degrees
void center();                    // neutral jaw + neck

} // namespace Head
