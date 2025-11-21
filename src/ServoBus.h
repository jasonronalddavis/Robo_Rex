#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

// ========== PCA9685 Servo Control Configuration ==========
// Simple design: All 16 servos use one PCA9685 board
// Port numbers in code = Physical port numbers on hardware (0-15)

// I2C pins for PCA9685
#ifndef PCA9685_SDA_PIN
#define PCA9685_SDA_PIN 10
#endif

#ifndef PCA9685_SCL_PIN
#define PCA9685_SCL_PIN 11
#endif

#define PCA9685_I2C_ADDRESS 0x40
#define SERVO_COUNT 16  // PCA9685 has 16 ports (0-15)

// ---------------- Servo limits & mapping ----------------
struct ServoLimits {
  uint16_t minPulse;   // microseconds (typical 500–700)
  uint16_t maxPulse;   // microseconds (typical 2300–2500)
  float    minDeg;     // degrees (mechanical min)
  float    maxDeg;     // degrees (mechanical max)

  constexpr ServoLimits(uint16_t minP = 500,
                        uint16_t maxP = 2500,
                        float    minD = 0.0f,
                        float    maxD = 180.0f)
  : minPulse(minP), maxPulse(maxP), minDeg(minD), maxDeg(maxD) {}
};

// --------------------------- ServoBus ---------------------------
class ServoBus {
public:
  // Initialize PCA9685
  // i2c_addr: PCA9685 I2C address (default 0x40)
  // freq_hz: PWM frequency (default 50Hz for servos)
  bool begin(uint8_t i2c_addr = PCA9685_I2C_ADDRESS, float freq_hz = 50.0f);

  // Change PCA9685 servo output frequency
  void setFrequency(float freq_hz);

  // Attach/detach servo ports with limits
  // port: PCA9685 port number (0-15), matches physical hardware port
  void attach(uint8_t port, const ServoLimits& limits = ServoLimits());
  void detach(uint8_t port);
  void setLimits(uint8_t port, const ServoLimits& limits);

  // Writes
  void writeMicroseconds(uint8_t port, uint16_t us);
  void writeDegrees(uint8_t port, float deg);
  void writeNeutral(uint8_t port);   // midpoint (average of min/max degrees)
  void setAllOff();                  // disable pulses on all ports

  // Queries
  inline bool  isAttached(uint8_t port) const {
    return (port < SERVO_COUNT) && _attached[port];
  }
  inline float frequency() const { return _freq; }

private:
  Adafruit_PWMServoDriver _pca9685;

  ServoLimits _limits[SERVO_COUNT];
  bool        _attached[SERVO_COUNT] = { false };
  float       _freq = 50.0f;
  uint8_t     _i2cAddr = PCA9685_I2C_ADDRESS;

  // Helpers
  uint16_t _degToUs(uint8_t port, float deg) const;

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
