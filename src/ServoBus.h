#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

struct ServoLimits {
  uint16_t minPulse;
  uint16_t maxPulse;
  float    minDeg;
  float    maxDeg;

  // Explicit ctor so ServoLimits{...} and default construction both work
  constexpr ServoLimits(uint16_t minP = 500,
                        uint16_t maxP = 2500,
                        float    minD = 0.0f,
                        float    maxD = 180.0f)
  : minPulse(minP), maxPulse(maxP), minDeg(minD), maxDeg(maxD) {}
};


class ServoBus {
public:
  // PCA9685 @ default addr 0x40, 50 Hz for servos (adjust to your hardware)
  bool begin(uint8_t i2c_addr = 0x40, float freq_hz = 50.0f);
  void attach(uint8_t channel, ServoLimits limits = {});
  void writeMicroseconds(uint8_t channel, uint16_t us);
  void writeDegrees(uint8_t channel, float deg);
  void setAllOff();

private:
  Adafruit_PWMServoDriver _pwm;
  ServoLimits _limits[16];
  bool _attached[16] = {false};
  uint16_t _usToTicks(uint16_t us) const;
  uint16_t _degToUs(uint8_t ch, float deg) const;
  float _freq = 50.0f;
};
