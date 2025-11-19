#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

// ========== GPIO Pin Definitions ==========
// NOTE: GPIO 8 and 9 are reserved for IMU (Wire1)
// Using GPIO pins that don't conflict with IMU or system pins

// Single servo channels (0-5)
#define SERVO_CH0_PIN   1   // Neck Yaw
#define SERVO_CH1_PIN   2   // Jaw
#define SERVO_CH2_PIN   3   // Head Pitch
#define SERVO_CH3_PIN   4   // Pelvis Roll
#define SERVO_CH4_PIN   5   // Spine Yaw
#define SERVO_CH5_PIN   6   // Tail Wag

// Leg servo channels (6-15) - 10 servos total
// Using GPIO 7, 10-18 (avoiding 8-9 for IMU, and 33-37 which are hardware restricted)
#define SERVO_CH6_PIN   7    // Right Hip X
#define SERVO_CH7_PIN   10   // Right Hip Y
#define SERVO_CH8_PIN   11   // Right Knee
#define SERVO_CH9_PIN   12   // Right Ankle
#define SERVO_CH10_PIN  13   // Right Foot
#define SERVO_CH11_PIN  14   // Left Hip X
#define SERVO_CH12_PIN  15   // Left Hip Y
#define SERVO_CH13_PIN  16   // Left Knee
#define SERVO_CH14_PIN  17   // Left Ankle
#define SERVO_CH15_PIN  18   // Left Foot

// Total servo count
#define SERVO_COUNT 16

// ---------------- Servo limits & mapping ----------------
struct ServoLimits {
  uint16_t minPulse;   // microseconds (typical 500–700)
  uint16_t maxPulse;   // microseconds (typical 2300–2500)
  float    minDeg;     // degrees (mechanical min)
  float    maxDeg;     // degrees (mechanical max)

  // C++11-friendly defaults so ServoLimits{500,2500,0,180} works
  constexpr ServoLimits(uint16_t minP = 500,
                        uint16_t maxP = 2500,
                        float    minD = 0.0f,
                        float    maxD = 180.0f)
  : minPulse(minP), maxPulse(maxP), minDeg(minD), maxDeg(maxD) {}
};

// --------------------------- ServoBus ---------------------------
class ServoBus {
public:
  // Initialize ESP32 GPIO servo system
  // Parameters kept for API compatibility but ignored in GPIO mode
  bool begin(uint8_t i2c_addr = 0x40, float freq_hz = 50.0f);

  // Change servo output frequency (kept for compatibility)
  void setFrequency(float freq_hz);

  // Attach/detach logical servo channels with limits
  void attach(uint8_t channel, const ServoLimits& limits = ServoLimits());
  void detach(uint8_t channel);
  void setLimits(uint8_t channel, const ServoLimits& limits);

  // Writes
  void writeMicroseconds(uint8_t channel, uint16_t us);
  void writeDegrees(uint8_t channel, float deg);
  void writeNeutral(uint8_t channel);   // midpoint (average of min/max degrees)
  void setAllOff();                     // disable pulses on all channels

  // Queries
  inline bool  isAttached(uint8_t ch) const { 
    return (ch < SERVO_COUNT) && _attached[ch]; 
  }
  inline float frequency() const { return _freq; }

private:
  Servo       _servos[SERVO_COUNT];
  ServoLimits _limits[SERVO_COUNT];
  bool        _attached[SERVO_COUNT] = { false };
  uint8_t     _pins[SERVO_COUNT];
  float       _freq = 50.0f;

  // Helpers
  uint16_t _degToUs(uint8_t ch, float deg) const;
  uint8_t  _channelToPin(uint8_t channel) const;
  
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
