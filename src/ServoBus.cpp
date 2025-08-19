#include "ServoBus.h"

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {
  _pwm = Adafruit_PWMServoDriver(i2c_addr);
  _pwm.begin();
  setFrequency(freq_hz);
  // defaults...
  delay(10);
  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  // PCA9685 datasheet usable range is wider, but we clamp to a safe window
  if (freq_hz < 40.0f)   freq_hz = 40.0f;
  if (freq_hz > 1000.0f) freq_hz = 1000.0f;
  _freq = freq_hz;
  _pwm.setPWMFreq(_freq);
  delay(3);
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] setPWMFreq=")); Serial.println(_freq);
#endif
}

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {
  if (channel >= 16) return;
  _limits[channel]   = limits;
  _attached[channel] = true;
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] attach ch=")); Serial.println(channel);
#endif
}

void ServoBus::detach(uint8_t channel) {
  if (channel >= 16) return;
  _attached[channel] = false;
  _pwm.setPWM(channel, 0, 0); // off
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] detach ch=")); Serial.println(channel);
#endif
}

void ServoBus::setLimits(uint8_t channel, const ServoLimits& limits) {
  if (channel >= 16) return;
  _limits[channel] = limits;
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] setLimits ch=")); Serial.println(channel);
#endif
}

uint16_t ServoBus::_usToTicks(uint16_t us) const {
  // PCA9685 has 12-bit resolution over one period: 0..4095 are the 4096 steps.
  // Period_us = 1e6 / freq; ticks = round(us / Period_us * 4096)
  const float period_us = 1000000.0f / _freq;
  float ticks_f = (static_cast<float>(us) / period_us) * 4096.0f;

  // Clamp to valid tick range for setPWM off-count
  if (ticks_f < 0.0f)    ticks_f = 0.0f;
  if (ticks_f > 4095.0f) ticks_f = 4095.0f;

  return static_cast<uint16_t>(ticks_f + 0.5f); // nearest
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  const ServoLimits& L = _limits[ch];
  const float d = _clampF(deg, L.minDeg, L.maxDeg);

  // Linear map d in [minDeg, maxDeg] → [minPulse, maxPulse] (µs)
  const float spanDeg = (L.maxDeg - L.minDeg);
  if (spanDeg <= 1e-6f) return L.minPulse;

  const float t = (d - L.minDeg) / spanDeg; // 0..1
  const float us_f = static_cast<float>(L.minPulse) +
                     t * static_cast<float>(L.maxPulse - L.minPulse);

  // Clamp to a sane servo pulse window (in microseconds)
  const uint16_t us = _clampU16(static_cast<int32_t>(us_f + 0.5f), 400, 2800);
  return us;
}

void ServoBus::writeMicroseconds(uint8_t ch, uint16_t us) {
  if (ch >= 16 || !_attached[ch]) return;
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(ch, 0, ticks);
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] ch=")); Serial.print(ch);
  Serial.print(F(" us=")); Serial.print(us);
  Serial.print(F(" ticks=")); Serial.println(ticks);
#endif
}

void ServoBus::writeDegrees(uint8_t ch, float deg) {
  if (ch >= 16 || !_attached[ch]) return;
  const uint16_t us    = _degToUs(ch, deg);
  const uint16_t ticks = _usToTicks(us);
  _pwm.setPWM(ch, 0, ticks);
#ifdef SERVO_BUS_DEBUG
  Serial.print(F("[ServoBus] ch=")); Serial.print(ch);
  Serial.print(F(" deg=")); Serial.print(deg, 1);
  Serial.print(F(" us="));  Serial.print(us);
  Serial.print(F(" ticks=")); Serial.println(ticks);
#endif
}

void ServoBus::writeNeutral(uint8_t ch) {
  if (ch >= 16 || !_attached[ch]) return;
  const ServoLimits& L = _limits[ch];
  const float midDeg = (L.minDeg + L.maxDeg) * 0.5f;
  writeDegrees(ch, midDeg);
}

void ServoBus::setAllOff() {
  for (uint8_t ch = 0; ch < 16; ++ch) {
    _pwm.setPWM(ch, 0, 0);
  }
#ifdef SERVO_BUS_DEBUG
  Serial.println(F("[ServoBus] setAllOff"));
#endif
}
