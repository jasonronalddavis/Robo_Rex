#include "ServoBus.h"
#include <Wire.h>

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {
  _freq = freq_hz;
  _i2cAddr = i2c_addr;

  Serial.println(F("[ServoBus] Initializing PCA9685 servo system"));
  Serial.print(F("  16 servos on PCA9685 ports 0-15 (direct mapping)"));
  Serial.println();

  // Initialize per-port defaults
  for (uint8_t port = 0; port < SERVO_COUNT; ++port) {
    _attached[port] = false;
    _limits[port]   = ServoLimits();   // default 500–2500 µs, 0–180 deg
  }

  // Initialize I2C bus for PCA9685
  Wire.begin(PCA9685_SDA_PIN, PCA9685_SCL_PIN);
  Wire.setClock(100000);  // 100kHz for PCA9685 (standard I2C speed)
  delay(50);

  Serial.print(F("[ServoBus] PCA9685: I2C on GPIO "));
  Serial.print(PCA9685_SDA_PIN);
  Serial.print(F(" (SDA), GPIO "));
  Serial.print(PCA9685_SCL_PIN);
  Serial.print(F(" (SCL) @ 0x"));
  Serial.println(_i2cAddr, HEX);

  // Initialize PCA9685
  _pca9685 = Adafruit_PWMServoDriver(_i2cAddr, Wire);

  Serial.print(F("[ServoBus] PCA9685: Calling begin()... "));
  _pca9685.begin();

  // Verify I2C communication
  Wire.beginTransmission(_i2cAddr);
  uint8_t i2c_error = Wire.endTransmission();

  if (i2c_error != 0) {
    Serial.println(F("FAILED!"));
    Serial.print(F("[ServoBus] ERROR: PCA9685 not responding (I2C error "));
    Serial.print(i2c_error);
    Serial.println(F(")"));
    Serial.println(F("[ServoBus] Check: 1) Wiring 2) V+ power 3) I2C address"));
    return false;
  }

  Serial.println(F("SUCCESS!"));
  _pca9685.setPWMFreq(freq_hz);
  delay(50);

  Serial.print(F("[ServoBus] PCA9685: Ready at "));
  Serial.print(freq_hz);
  Serial.println(F(" Hz"));
  return true;
}

void ServoBus::setFrequency(float freq_hz) {
  _freq = freq_hz;
  _pca9685.setPWMFreq(freq_hz);
  Serial.print(F("[ServoBus] Frequency: "));
  Serial.print(freq_hz);
  Serial.println(F(" Hz"));
}

void ServoBus::attach(uint8_t port, const ServoLimits& limits) {
  if (port >= SERVO_COUNT) {
    Serial.print(F("[ServoBus] ERROR: Port out of range: "));
    Serial.println(port);
    return;
  }

  _limits[port] = limits;
  _attached[port] = true;

  Serial.print(F("[ServoBus] Attached port "));
  Serial.println(port);
}

void ServoBus::detach(uint8_t port) {
  if (port >= SERVO_COUNT) return;

  if (_attached[port]) {
    _pca9685.setPWM(port, 0, 0);  // Turn off PWM
    _attached[port] = false;
    Serial.print(F("[ServoBus] Detached port "));
    Serial.println(port);
  }
}

void ServoBus::setLimits(uint8_t port, const ServoLimits& limits) {
  if (port >= SERVO_COUNT) return;
  _limits[port] = limits;
}

uint16_t ServoBus::_degToUs(uint8_t port, float deg) const {
  if (port >= SERVO_COUNT) return 1500;

  const ServoLimits& lim = _limits[port];
  float d = _clampF(deg, lim.minDeg, lim.maxDeg);

  // Map [minDeg, maxDeg] → [minPulse, maxPulse]
  float t = (d - lim.minDeg) / (lim.maxDeg - lim.minDeg);
  int32_t us = (int32_t)(lim.minPulse + t * (lim.maxPulse - lim.minPulse));
  return _clampU16(us, lim.minPulse, lim.maxPulse);
}

void ServoBus::writeMicroseconds(uint8_t port, uint16_t us) {
  if (port >= SERVO_COUNT) return;
  if (!_attached[port]) {
    Serial.print(F("[ServoBus] WARNING: Port "));
    Serial.print(port);
    Serial.println(F(" not attached"));
    return;
  }

  const ServoLimits& lim = _limits[port];
  uint16_t clamped = _clampU16(us, lim.minPulse, lim.maxPulse);

  // Convert microseconds to 12-bit PWM value
  // Formula: pwm = (microseconds * 4096 * frequency) / 1,000,000
  uint32_t pwm = (uint32_t)((clamped * 4096.0 * _freq) / 1000000.0);
  pwm = (pwm > 4095) ? 4095 : pwm;  // Clamp to 12-bit max

  _pca9685.setPWM(port, 0, pwm);
}

void ServoBus::writeDegrees(uint8_t port, float deg) {
  if (port >= SERVO_COUNT) return;
  if (!_attached[port]) return;

  uint16_t us = _degToUs(port, deg);
  writeMicroseconds(port, us);
}

void ServoBus::writeNeutral(uint8_t port) {
  if (port >= SERVO_COUNT) return;
  if (!_attached[port]) return;

  const ServoLimits& lim = _limits[port];
  float mid = 0.5f * (lim.minDeg + lim.maxDeg);
  writeDegrees(port, mid);
}

void ServoBus::setAllOff() {
  Serial.println(F("[ServoBus] Turning off all ports"));
  for (uint8_t port = 0; port < SERVO_COUNT; ++port) {
    if (_attached[port]) {
      _pca9685.setPWM(port, 0, 0);
      _attached[port] = false;
    }
  }
}
