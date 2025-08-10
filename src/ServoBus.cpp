#include <Arduino.h>
#include "ServoBus.h"
#include <Wire.h>

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {
  Wire.begin();
  Wire.setClock(400000); // fast I2C is fine for PCA9685

  // (Re)construct the driver with the requested address
  _pwm = Adafruit_PWMServoDriver(i2c_addr);

  // Initialize and set frequency
  _pwm.begin();
  setFrequency(freq_hz);

  // Default all channels to unattached, conservative limits
  for (uint8_t ch = 0; ch < 16; ++ch) {
    _attached[ch] = false;
    _limits[ch]   = ServoLimits(); // 500..2500 µs, 0..180 deg
  }

  // Small delay to allow oscillator to stabilize
  delay(10);
  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  if (freq_hz < 40.0f)  freq_hz = 40.0f;   // datasheet guardrails
  if (freq_hz > 1000.0f) freq_hz = 1000.0f;
  _freq = freq_hz;
  _pwm.setPWMFreq(_freq);
  // Per Adafruit notes, brief delay after setPWMFreq helps
  delay(5);
}

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {
  if (channel >= 16) return;
  _limits[channel]   = limits;
  _attached[channel] = true;
}

uint16_t ServoBus::_usToTicks(uint16_t us) const {
  // PCA9685: 12-bit (0..4095) across one period. Period_us = 1e6 / freq
  const float period_us = 1000000.0f / _freq;
  float ticks_f = (static_cast<float>(us) / period_us) * 4096.0f;

  if (ticks_f < 0.0f)    ticks_f = 0.0f;
  if (ticks_f > 4095.0f) ticks_f = 4095.0f;

  return static_cast<uint16_t>(ticks_f + 0.5f); // round to nearest
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  const ServoLimits& L = _limits[ch];
  const float d = _clampF(deg, L.minDeg, L.maxDeg);

  // Linear map d in [minDeg, maxDeg] → [minPulse, maxPulse]
  const float spanDeg = (L.maxDeg - L.minDeg);
  if (spanDeg <= 0.0001f) return L.minPulse;

  const float t = (d - L.minDeg) / spanDeg; // 0..1
  const float us_f = static_cast<float>(L.minPulse) +
                     t * static_cast<float>(L.maxPulse - L.minPulse);

  // Clamp to safe microsecond range just in case
  return _clampU16(static_cast<int32_t>(us_f + 0.5f), 0, 4095); // 4095 is safe upper cap
}

void ServoBus::writeMicroseconds(uint8_t ch, uint16_t us) {
  if (ch >= 16 || !_attached[ch]) return;
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(ch, 0, ticks);
}

void ServoBus::writeDegrees(uint8_t ch, float deg) {
  if (ch >= 16 || !_attached[ch]) return;
  const uint16_t us = _degToUs(ch, deg);
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(ch, 0, ticks);
}

void ServoBus::setAllOff() {
  for (uint8_t ch = 0; ch < 16; ++ch) {
    _pwm.setPWM(ch, 0, 0);
  }
}