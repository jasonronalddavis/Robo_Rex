#include "ServoBus.h"

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {
  // I²C (Wire) must already be initialized in main.cpp with the desired pins/clock.
  _pwm = Adafruit_PWMServoDriver(i2c_addr);

  // Initialize PCA9685
  _pwm.begin();
  setFrequency(freq_hz);

  // Defaults for all channels
  for (uint8_t ch = 0; ch < 16; ++ch) {
    _attached[ch] = false;
    _limits[ch]   = ServoLimits(); // 500..2500 µs, 0..180°
  }

  // Brief settle
  delay(5);
  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  // PCA9685 supports ~24–1526 Hz; clamp to a safe hobby-servo range
  if (freq_hz < 40.0f)   freq_hz = 40.0f;
  if (freq_hz > 1000.0f) freq_hz = 1000.0f;
  _freq = freq_hz;
  _pwm.setPWMFreq(_freq);
  delay(2);
}

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {
  if (channel >= 16) return;
  _limits[channel]   = limits;
  _attached[channel] = true;
}

void ServoBus::detach(uint8_t channel) {
  if (channel >= 16) return;
  _attached[channel] = false;
  _pwm.setPWM(channel, 0, 0); // turn off pulse
}

void ServoBus::setLimits(uint8_t channel, const ServoLimits& limits) {
  if (channel >= 16) return;
  _limits[channel] = limits;
}

uint16_t ServoBus::_usToTicks(uint16_t us) const {
  // PCA9685: 12-bit (0..4095) across one period; Period_us = 1e6 / freq
  const float period_us = 1000000.0f / _freq;
  float ticks_f = (static_cast<float>(us) / period_us) * 4096.0f;

  if (ticks_f < 0.0f)    ticks_f = 0.0f;
  if (ticks_f > 4095.0f) ticks_f = 4095.0f;

  return static_cast<uint16_t>(ticks_f + 0.5f); // round to nearest
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  const ServoLimits& L = _limits[ch];
  const float d = _clampF(deg, L.minDeg, L.maxDeg);

  const float spanDeg = (L.maxDeg - L.minDeg);
  if (spanDeg <= 1e-6f) return L.minPulse;

  const float t = (d - L.minDeg) / spanDeg; // 0..1
  const float us_f = static_cast<float>(L.minPulse) +
                     t * static_cast<float>(L.maxPulse - L.minPulse);

  // Keep pulses in a sane servo window
  return _clampU16(static_cast<int32_t>(us_f + 0.5f), 400, 2800);
}

void ServoBus::writeMicroseconds(uint8_t channel, uint16_t us) {
  if (channel >= 16 || !_attached[channel]) return;
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(channel, 0, ticks);
}

void ServoBus::writeDegrees(uint8_t channel, float deg) {
  if (channel >= 16 || !_attached[channel]) return;
  const uint16_t us    = _degToUs(channel, deg);
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(channel, 0, ticks);
}

void ServoBus::writeNeutral(uint8_t channel) {
  if (channel >= 16 || !_attached[channel]) return;
  const ServoLimits& L = _limits[channel];
  const float midDeg = (L.minDeg + L.maxDeg) * 0.5f;
  writeDegrees(channel, midDeg);
}

void ServoBus::setAllOff() {
  for (uint8_t ch = 0; ch < 16; ++ch) {
    _pwm.setPWM(ch, 0, 0);
  }
}