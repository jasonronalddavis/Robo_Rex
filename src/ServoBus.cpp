#include "ServoBus.h"

// Map logical channels 0–15 → GPIO pins using the macros
uint8_t ServoBus::_channelToPin(uint8_t channel) const {
  static const uint8_t kPins[SERVO_COUNT] = {
    SERVO_CH0_PIN,  // ch 0
    SERVO_CH1_PIN,  // ch 1
    SERVO_CH2_PIN,  // ch 2
    SERVO_CH3_PIN,  // ch 3
    SERVO_CH4_PIN,  // ch 4
    SERVO_CH5_PIN,  // ch 5
    SERVO_CH6_PIN,  // ch 6
    SERVO_CH7_PIN,  // ch 7
    SERVO_CH8_PIN,  // ch 8
    SERVO_CH9_PIN,  // ch 9
    SERVO_CH10_PIN, // ch10
    SERVO_CH11_PIN, // ch11
    SERVO_CH12_PIN, // ch12
    SERVO_CH13_PIN, // ch13
    SERVO_CH14_PIN, // ch14
    SERVO_CH15_PIN  // ch15
  };

  if (channel >= SERVO_COUNT) {
    return 0; // invalid, won't be used if we check bounds elsewhere
  }
  return kPins[channel];
}

bool ServoBus::begin(uint8_t /*i2c_addr*/, float freq_hz) {
  _freq = freq_hz;

  // Initialize per-channel defaults
  for (uint8_t ch = 0; ch < SERVO_COUNT; ++ch) {
    _attached[ch] = false;
    _limits[ch]   = ServoLimits();   // default 500–2500 µs, 0–180 deg
    _pins[ch]     = _channelToPin(ch);
  }

  // ESP32Servo global config if needed
  // (Most boards work fine with defaults, but you can tune here.)
  // Example:
  // ESP32PWM::allocateTimer(0);
  // ESP32PWM::allocateTimer(1);
  // ESP32PWM::allocateTimer(2);
  // ESP32PWM::allocateTimer(3);

  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  // In pure GPIO mode with ESP32Servo, the frequency is effectively 50 Hz.
  // We just store it for reference; no hardware change here.
  _freq = freq_hz;
}

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {
  if (channel >= SERVO_COUNT) {
    Serial.print(F("[ServoBus] ERROR: attach channel out of range: "));
    Serial.println(channel);
    return;
  }

  // Detach first if already attached
  if (_attached[channel]) {
    _servos[channel].detach();
    _attached[channel] = false;
  }

  _limits[channel] = limits;
  uint8_t pin = _pins[channel];

  // Attach with limit pulses
  if (_servos[channel].attach(pin, limits.minPulse, limits.maxPulse)) {
    _attached[channel] = true;
    Serial.print(F("[ServoBus] attach ch="));
    Serial.print(channel);
    Serial.print(F(" -> GPIO "));
    Serial.println(pin);
  } else {
    Serial.print(F("[ServoBus] ERROR: failed to attach ch="));
    Serial.print(channel);
    Serial.print(F(" to GPIO "));
    Serial.println(pin);
    _attached[channel] = false;
  }
}

void ServoBus::detach(uint8_t channel) {
  if (channel >= SERVO_COUNT) return;
  if (_attached[channel]) {
    _servos[channel].detach();
    _attached[channel] = false;
    Serial.print(F("[ServoBus] detach ch="));
    Serial.println(channel);
  }
}

void ServoBus::setLimits(uint8_t channel, const ServoLimits& limits) {
  if (channel >= SERVO_COUNT) return;
  _limits[channel] = limits;
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  if (ch >= SERVO_COUNT) return 1500;

  const ServoLimits& lim = _limits[ch];
  // Clamp degrees into allowed range
  float d = _clampF(deg, lim.minDeg, lim.maxDeg);

  // Map [minDeg, maxDeg] → [minPulse, maxPulse]
  float t = (d - lim.minDeg) / (lim.maxDeg - lim.minDeg);
  int32_t us = (int32_t)(lim.minPulse + t * (lim.maxPulse - lim.minPulse));
  return _clampU16(us, lim.minPulse, lim.maxPulse);
}

void ServoBus::writeMicroseconds(uint8_t channel, uint16_t us) {
  if (channel >= SERVO_COUNT) return;
  if (!_attached[channel]) return;

  const ServoLimits& lim = _limits[channel];
  uint16_t clamped = _clampU16(us, lim.minPulse, lim.maxPulse);
  _servos[channel].writeMicroseconds(clamped);
}

void ServoBus::writeDegrees(uint8_t channel, float deg) {
  if (channel >= SERVO_COUNT) return;
  if (!_attached[channel]) return;

  uint16_t us = _degToUs(channel, deg);
  _servos[channel].writeMicroseconds(us);
}

void ServoBus::writeNeutral(uint8_t channel) {
  if (channel >= SERVO_COUNT) return;
  if (!_attached[channel]) return;

  const ServoLimits& lim = _limits[channel];
  float mid = 0.5f * (lim.minDeg + lim.maxDeg);
  writeDegrees(channel, mid);
}

void ServoBus::setAllOff() {
  Serial.println(F("[ServoBus] setAllOff() - detaching all channels"));
  for (uint8_t ch = 0; ch < SERVO_COUNT; ++ch) {
    if (_attached[ch]) {
      _servos[ch].detach();
      _attached[ch] = false;
    }
  }
}
