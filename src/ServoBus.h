#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

struct ServoLimits {
  uint16_t minPulse = 500;   // µs
  uint16_t maxPulse = 2500;  // µs
  float minDeg = 0.0f;
  float maxDeg = 180.0f;
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
