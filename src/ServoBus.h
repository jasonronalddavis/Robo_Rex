#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Adafruit_PWMServoDriver.h>

// ========== Hybrid Servo Control Configuration ==========

// Channels 0-5:  ESP32 GPIO direct control (Neck, Head, Pelvis, Spine, Tail)

// Channels 6-15: PCA9685 I2C PWM driver (Legs - 10 servos)

 

// ========== GPIO Pin Definitions (Channels 0-5) ==========

// NOTE: GPIO 8 and 9 are reserved for IMU (Wire1)

// NOTE: GPIO 10 and 11 are reserved for PCA9685 (Wire)

 

// Body servo channels (0-5) - Direct GPIO control

#define SERVO_CH0_PIN   1   // Neck Yaw

#define SERVO_CH1_PIN   2   // Jaw

#define SERVO_CH2_PIN   3   // Head Pitch

#define SERVO_CH3_PIN   4   // Pelvis Roll

#define SERVO_CH4_PIN   5   // Spine Yaw

#define SERVO_CH5_PIN   6   // Tail Wag

 

// ========== PCA9685 Configuration (Channels 6-15) ==========

// Leg servo channels (6-15) - 10 servos via PCA9685

// Channel 6  -> PCA9685 port 0  (Right Hip X)

// Channel 7  -> PCA9685 port 1  (Right Hip Y)

// Channel 8  -> PCA9685 port 2  (Right Knee)

// Channel 9  -> PCA9685 port 3  (Right Ankle)

// Channel 10 -> PCA9685 port 4  (Right Foot)

// Channel 11 -> PCA9685 port 5  (Left Hip X)

// Channel 12 -> PCA9685 port 6  (Left Hip Y)

// Channel 13 -> PCA9685 port 7  (Left Knee)

// Channel 14 -> PCA9685 port 8  (Left Ankle)

// Channel 15 -> PCA9685 port 9  (Left Foot)

 

#define PCA9685_FIRST_CHANNEL 6  // First channel using PCA9685

#define PCA9685_I2C_ADDRESS   0x40

 

// I2C pins for PCA9685 (using default Wire bus)

#ifndef PCA9685_SDA_PIN

#define PCA9685_SDA_PIN 10

#endif

 

#ifndef PCA9685_SCL_PIN

#define PCA9685_SCL_PIN 11

#endif

 

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

  // Initialize hybrid servo system

  // i2c_addr: PCA9685 I2C address (default 0x40)

  // freq_hz: PCA9685 PWM frequency (default 50Hz)

  bool begin(uint8_t i2c_addr = PCA9685_I2C_ADDRESS, float freq_hz = 50.0f);

 

  // Change PCA9685 servo output frequency

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

  // GPIO servos (channels 0-5)

  Servo       _gpioServos[PCA9685_FIRST_CHANNEL];

  uint8_t     _gpioPins[PCA9685_FIRST_CHANNEL];

 

  // PCA9685 driver (channels 6-15)

  Adafruit_PWMServoDriver _pca9685;

 

  // Shared configuration

  ServoLimits _limits[SERVO_COUNT];
  bool        _attached[SERVO_COUNT] = { false };
  float       _freq = 50.0f;
  uint8_t     _i2cAddr = PCA9685_I2C_ADDRESS;

  // Helpers

  uint16_t _degToUs(uint8_t ch, float deg) const;

  uint8_t  _channelToGpioPin(uint8_t channel) const;

  uint8_t  _channelToPcaPort(uint8_t channel) const;

  bool     _isGpioChannel(uint8_t channel) const { return channel < PCA9685_FIRST_CHANNEL; }

 

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

 