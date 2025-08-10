#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

// Per-channel motion limits + mapping (µs and degrees)
struct ServoLimits {
  uint16_t minPulse;   // microseconds (typical 500–700)
  uint16_t maxPulse;   // microseconds (typical 2300–2500)
  float    minDeg;     // degrees (mechanical min)
  float    maxDeg;     // degrees (mechanical max)

  // C++11-friendly ctor so `ServoLimits{500,2500,0,180}` works
  constexpr ServoLimits(uint16_t minP = 500,
                        uint16_t maxP = 2500,
                        float    minD = 0.0f,
                        float    maxD = 180.0f)
  : minPulse(minP), maxPulse(maxP), minDeg(minD), maxDeg(maxD) {}
};

class ServoBus {
public:
  // Initialize the PCA9685 and set the servo frequency (Hz). Returns true if OK.
  bool begin(uint8_t i2c_addr = 0x40, float freq_hz = 50.0f);

  // Attach a logical "servo" to a PCA9685 channel with limits.
  void attach(uint8_t channel, const ServoLimits& limits = ServoLimits());

  // Write directly in microseconds (mapped to PCA9685 ticks).
  void writeMicroseconds(uint8_t channel, uint16_t us);

  // Write in degrees (mapped through per-channel limits to microseconds).
  void writeDegrees(uint8_t channel, float deg);

  // Turn all channels off (no pulses).
  void setAllOff();

  // Optionally adjust frequency later (e.g., 50 Hz for analog servos).
  void setFrequency(float freq_hz);

  // Quick query helpers
  inline bool isAttached(uint8_t ch) const { return (ch < 16) && _attached[ch]; }
  inline float frequency() const { return _freq; }

private:
  Adafruit_PWMServoDriver _pwm = Adafruit_PWMServoDriver(0x40);

  ServoLimits _limits[16];
  bool       _attached[16] = { false };
  float      _freq         = 50.0f;

  // Conversions
  uint16_t _usToTicks(uint16_t us) const;
  uint16_t _degToUs(uint8_t ch, float deg) const;

  // Clamp helpers
  static inline uint16_t _clampU16(int32_t v, uint16_t lo, uint16_t hi) {
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (uint16_t)v;
  }
  static inline float _clampF(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  }
};