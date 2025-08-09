#include <Arduino.h>
#include "ServoBus.h"
#include <Wire.h>


bool ServoBus::begin(uint8_t addr, float freq_hz) {
  Wire.begin();
  _pwm = Adafruit_PWMServoDriver(addr);
  _pwm.begin();
  _freq = freq_hz;
  _pwm.setPWMFreq(_freq);
  delay(10);
  return true;
}

void ServoBus::attach(uint8_t channel, ServoLimits limits) {
  if (channel < 16) {
    _attached[channel] = true;
    _limits[channel] = limits;
  }
}

uint16_t ServoBus::_usToTicks(uint16_t us) const {
  // PCA9685: 4096 steps per cycle
  float period_us = 1e6f / _freq;
  float ticks = (us / period_us) * 4096.0f;
  if (ticks < 0) ticks = 0;
  if (ticks > 4095) ticks = 4095;
  return (uint16_t)ticks;
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  const auto& L = _limits[ch];
  if (deg < L.minDeg) deg = L.minDeg;
  if (deg > L.maxDeg) deg = L.maxDeg;
  float t = (deg - L.minDeg) / (L.maxDeg - L.minDeg + 1e-6f);
  return (uint16_t)(L.minPulse + t * (L.maxPulse - L.minPulse));
}

void ServoBus::writeMicroseconds(uint8_t ch, uint16_t us) {
  if (ch >= 16 || !_attached[ch]) return;
  uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(ch, 0, ticks);
}

void ServoBus::writeDegrees(uint8_t ch, float deg) {
  if (ch >= 16 || !_attached[ch]) return;
  writeMicroseconds(ch, _degToUs(ch, deg));
}

void ServoBus::setAllOff() {
  for (int ch=0; ch<16; ++ch) {
    _pwm.setPWM(ch, 0, 0);
  }
}
