// ServoBus.cpp - ESP32-S3 GPIO Version
// Complete implementation for direct GPIO servo control

#include "ServoBus.h"

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {
  // Parameters kept for API compatibility with old PCA9685 version
  // but are not used in GPIO mode
  (void)i2c_addr; // Suppress unused parameter warning
  (void)freq_hz;  // Suppress unused parameter warning
  
  Serial.println(F("[ServoBus] Initializing ESP32-S3 GPIO servo system..."));
  
  _freq = 50.0f; // Standard servo frequency
  
  // Initialize pin mapping array - maps channel number to GPIO pin
  _pins[0]  = SERVO_CH0_PIN;   // Channel 0: Neck Yaw → GPIO 1
  _pins[1]  = SERVO_CH1_PIN;   // Channel 1: Jaw → GPIO 2
  _pins[2]  = SERVO_CH2_PIN;   // Channel 2: Head Pitch → GPIO 3
  _pins[3]  = SERVO_CH3_PIN;   // Channel 3: Pelvis Roll → GPIO 4
  _pins[4]  = SERVO_CH4_PIN;   // Channel 4: Spine Yaw → GPIO 5
  _pins[5]  = SERVO_CH5_PIN;   // Channel 5: Tail Wag → GPIO 6
  _pins[6]  = SERVO_CH6_PIN;   // Channel 6: Right Hip X → GPIO 7
  _pins[7]  = SERVO_CH7_PIN;   // Channel 7: Right Hip Y → GPIO 10
  _pins[8]  = SERVO_CH8_PIN;   // Channel 8: Right Knee → GPIO 11
  _pins[9]  = SERVO_CH9_PIN;   // Channel 9: Right Ankle → GPIO 12
  _pins[10] = SERVO_CH10_PIN;  // Channel 10: Right Foot → GPIO 13
  _pins[11] = SERVO_CH11_PIN;  // Channel 11: Left Hip X → GPIO 14
  _pins[12] = SERVO_CH12_PIN;  // Channel 12: Left Hip Y → GPIO 15
  _pins[13] = SERVO_CH13_PIN;  // Channel 13: Left Knee → GPIO 16
  _pins[14] = SERVO_CH14_PIN;  // Channel 14: Left Ankle → GPIO 17
  _pins[15] = SERVO_CH15_PIN;  // Channel 15: Left Foot → GPIO 18
  
  // Initialize defaults for all channels
  for (uint8_t ch = 0; ch < SERVO_COUNT; ++ch) {
    _attached[ch] = false;
    _limits[ch] = ServoLimits(); // Default: 500..2500 µs, 0..180°
  }
  
  Serial.println(F("[ServoBus] GPIO mode initialized"));
  Serial.println(F("[ServoBus] NOTE: Servos will activate when attached via function begin() calls"));
  
  // Brief settle time
  delay(5);
  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  // Clamp to safe hobby-servo range
  if (freq_hz < 40.0f)   freq_hz = 40.0f;
  if (freq_hz > 1000.0f) freq_hz = 1000.0f;
  _freq = freq_hz;
  
  // ESP32Servo library handles frequency internally at 50Hz
  // This function is kept for API compatibility with PCA9685 version
  delay(2);
}

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {
  if (channel >= SERVO_COUNT) {
    Serial.print(F("[ServoBus] ERROR: Invalid channel "));
    Serial.println(channel);
    return;
  }
  
  // Store limits and mark as attached
  _limits[channel] = limits;
  _attached[channel] = true;
  
  // Get the GPIO pin for this channel
  uint8_t pin = _channelToPin(channel);
  if (pin == 255) {
    Serial.print(F("[ServoBus] ERROR: No GPIO pin mapped for channel "));
    Serial.println(channel);
    return;
  }
  
  // Attach the ESP32 servo to its GPIO pin
  _servos[channel].setPeriodHertz(50); // Standard 50Hz servo signal
  _servos[channel].attach(pin, limits.minPulse, limits.maxPulse);
  
  // CRITICAL: Move servo to center position immediately
  // This ensures servos activate and move to a safe position on startup
  float centerDeg = (limits.minDeg + limits.maxDeg) / 2.0f;
  _servos[channel].write(centerDeg);
  
  Serial.print(F("[ServoBus] Attached CH"));
  Serial.print(channel);
  Serial.print(F(" → GPIO"));
  Serial.print(pin);
  Serial.print(F(" (centered at "));
  Serial.print(centerDeg);
  Serial.println(F("°)"));
}

void ServoBus::detach(uint8_t channel) {
  if (channel >= SERVO_COUNT) return;
  
  _attached[channel] = false;
  
  if (_servos[channel].attached()) {
    _servos[channel].detach();
    Serial.print(F("[ServoBus] Detached CH"));
    Serial.println(channel);
  }
}

void ServoBus::setLimits(uint8_t channel, const ServoLimits& limits) {
  if (channel >= SERVO_COUNT) return;
  
  _limits[channel] = limits;
  
  // If servo is already attached, reattach with new limits
  if (_attached[channel] && _servos[channel].attached()) {
    uint8_t pin = _channelToPin(channel);
    _servos[channel].detach();
    _servos[channel].attach(pin, limits.minPulse, limits.maxPulse);
    
    Serial.print(F("[ServoBus] Updated limits for CH"));
    Serial.println(channel);
  }
}

uint16_t ServoBus::_degToUs(uint8_t ch, float deg) const {
  const ServoLimits& L = _limits[ch];
  const float d = _clampF(deg, L.minDeg, L.maxDeg);
  
  const float spanDeg = (L.maxDeg - L.minDeg);
  if (spanDeg <= 1e-6f) return L.minPulse;
  
  const float t = (d - L.minDeg) / spanDeg; // Normalize to 0..1
  const float us_f = static_cast<float>(L.minPulse) +
                     t * static_cast<float>(L.maxPulse - L.minPulse);
  
  // Keep pulses in a sane servo window (400-2800 microseconds)
  return _clampU16(static_cast<int32_t>(us_f + 0.5f), 400, 2800);
}

uint8_t ServoBus::_channelToPin(uint8_t channel) const {
  if (channel >= SERVO_COUNT) return 255; // Invalid channel
  return _pins[channel];
}

void ServoBus::writeMicroseconds(uint8_t channel, uint16_t us) {
  if (channel >= SERVO_COUNT || !_attached[channel]) return;
  
  if (_servos[channel].attached()) {
    _servos[channel].writeMicroseconds(us);
  }
}

void ServoBus::writeDegrees(uint8_t channel, float deg) {
  if (channel >= SERVO_COUNT || !_attached[channel]) return;
  
  // Convert degrees to microseconds using servo limits
  const uint16_t us = _degToUs(channel, deg);
  
  if (_servos[channel].attached()) {
    _servos[channel].writeMicroseconds(us);
  }
}

void ServoBus::writeNeutral(uint8_t channel) {
  if (channel >= SERVO_COUNT || !_attached[channel]) return;
  
  const ServoLimits& L = _limits[channel];
  const float midDeg = (L.minDeg + L.maxDeg) * 0.5f;
  writeDegrees(channel, midDeg);
  
  Serial.print(F("[ServoBus] CH"));
  Serial.print(channel);
  Serial.print(F(" → neutral ("));
  Serial.print(midDeg);
  Serial.println(F("°)"));
}

void ServoBus::setAllOff() {
  Serial.println(F("[ServoBus] Disabling all servos..."));
  
  for (uint8_t ch = 0; ch < SERVO_COUNT; ++ch) {
    if (_servos[ch].attached()) {
      _servos[ch].detach();
    }
    _attached[ch] = false;
  }
  
  Serial.println(F("[ServoBus] All servos disabled"));
}