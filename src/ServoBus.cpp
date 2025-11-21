#include "ServoBus.h"

#include <Wire.h>

 

// Map logical channels 0-5 to GPIO pins

uint8_t ServoBus::_channelToGpioPin(uint8_t channel) const {

  static const uint8_t kPins[PCA9685_FIRST_CHANNEL] = {

    SERVO_CH0_PIN,  // ch 0 -> GPIO 1

    SERVO_CH1_PIN,  // ch 1 -> GPIO 2

    SERVO_CH2_PIN,  // ch 2 -> GPIO 3

    SERVO_CH3_PIN,  // ch 3 -> GPIO 4

    SERVO_CH4_PIN,  // ch 4 -> GPIO 5

    SERVO_CH5_PIN   // ch 5 -> GPIO 6

  };

 

  if (channel >= PCA9685_FIRST_CHANNEL) {

    return 0; // Invalid for GPIO channels

  }

 

  return kPins[channel];

}

 

// Map logical channels 6-15 to PCA9685 ports 0-9

uint8_t ServoBus::_channelToPcaPort(uint8_t channel) const {

  if (channel < PCA9685_FIRST_CHANNEL || channel >= SERVO_COUNT) {

    return 0; // Invalid

  }

 

  return channel - PCA9685_FIRST_CHANNEL;  // ch6->port0, ch7->port1, ..., ch15->port9

}

 

bool ServoBus::begin(uint8_t i2c_addr, float freq_hz) {

  _freq = freq_hz;

  _i2cAddr = i2c_addr;

 

  Serial.println(F("[ServoBus] Initializing HYBRID servo system"));

  Serial.println(F("  Channels 0-5:  ESP32 GPIO direct control"));

  Serial.println(F("  Channels 6-15: PCA9685 I2C PWM driver"));

 

  // Initialize per-channel defaults

  for (uint8_t ch = 0; ch < SERVO_COUNT; ++ch) {

    _attached[ch] = false;

    _limits[ch]   = ServoLimits();   // default 500–2500 µs, 0–180 deg

  }

 

  // Store GPIO pins for channels 0-5

  for (uint8_t ch = 0; ch < PCA9685_FIRST_CHANNEL; ++ch) {

    _gpioPins[ch] = _channelToGpioPin(ch);

  }

 

  // ========== ESP32 GPIO Setup (Channels 0-5) ==========

  // Allocate LEDC timers for GPIO servos (6 servos = need 2 timers)

  ESP32PWM::allocateTimer(0);  // LEDC channels 0-3

  ESP32PWM::allocateTimer(1);  // LEDC channels 4-7

 

  Serial.println(F("[ServoBus] GPIO: Allocated 2 LEDC timers for channels 0-5"));

 

  // ========== PCA9685 I2C Setup (Channels 6-15) ==========

  // Initialize I2C bus for PCA9685

  Wire.begin(PCA9685_SDA_PIN, PCA9685_SCL_PIN);

  Wire.setClock(100000);  // 100kHz for PCA9685 (standard I2C speed)

  delay(50);

 

  Serial.print(F("[ServoBus] PCA9685: Initializing on Wire (SDA="));

  Serial.print(PCA9685_SDA_PIN);

  Serial.print(F(", SCL="));

  Serial.print(PCA9685_SCL_PIN);

  Serial.print(F(") @ 0x"));

  Serial.println(_i2cAddr, HEX);

 

  // Initialize PCA9685

  _pca9685 = Adafruit_PWMServoDriver(_i2cAddr, Wire);

 

  // Check if PCA9685 responds

  Serial.print(F("[ServoBus] PCA9685: Calling begin()... "));

  _pca9685.begin();

 

  // Verify I2C communication by reading MODE1 register

  Wire.beginTransmission(_i2cAddr);

  uint8_t i2c_error = Wire.endTransmission();

 

  if (i2c_error != 0) {

    Serial.println(F("FAILED!"));

    Serial.print(F("[ServoBus] ERROR: PCA9685 not responding on I2C address 0x"));

    Serial.print(_i2cAddr, HEX);

    Serial.print(F(" (I2C error code: "));

    Serial.print(i2c_error);

    Serial.println(F(")"));

    Serial.println(F("[ServoBus] Check: 1) Wiring  2) V+ power  3) I2C address jumpers"));

    return false;

  }

 

  Serial.println(F("SUCCESS!"));

  _pca9685.setPWMFreq(freq_hz);

  delay(50);

 

  Serial.print(F("[ServoBus] PCA9685: Initialized at "));

  Serial.print(freq_hz);

  Serial.println(F(" Hz"));

 

  Serial.println(F("[ServoBus] ✓ Hybrid servo system ready"));

  return true;

}

 

void ServoBus::setFrequency(float freq_hz) {

  _freq = freq_hz;

 

  // Only update PCA9685 frequency (GPIO servos are fixed at 50Hz)

  _pca9685.setPWMFreq(freq_hz);

 

  Serial.print(F("[ServoBus] PCA9685 frequency set to "));

  Serial.print(freq_hz);

  Serial.println(F(" Hz"));

}

 

void ServoBus::attach(uint8_t channel, const ServoLimits& limits) {

  if (channel >= SERVO_COUNT) {

    Serial.print(F("[ServoBus] ERROR: attach channel out of range: "));

    Serial.println(channel);

    return;

  }

 

  // Store limits

  _limits[channel] = limits;

 

  if (_isGpioChannel(channel)) {

    // ========== GPIO Servo (Channels 0-5) ==========

    // Detach first if already attached

    if (_attached[channel]) {

      _gpioServos[channel].detach();

      _attached[channel] = false;

    }

 

    uint8_t pin = _gpioPins[channel];

 

    // Attach with limit pulses

    if (_gpioServos[channel].attach(pin, limits.minPulse, limits.maxPulse)) {

      _attached[channel] = true;

      Serial.print(F("[ServoBus] GPIO: Attached ch="));

      Serial.print(channel);

      Serial.print(F(" -> GPIO "));

      Serial.println(pin);

    } else {

      Serial.print(F("[ServoBus] ERROR: GPIO attach failed ch="));

      Serial.print(channel);

      Serial.print(F(" to GPIO "));

      Serial.println(pin);

      _attached[channel] = false;

    }

  } else {

    // ========== PCA9685 Servo (Channels 6-15) ==========

    uint8_t pcaPort = _channelToPcaPort(channel);

    _attached[channel] = true;

    Serial.print(F("[ServoBus] PCA9685: Attached ch="));

    Serial.print(channel);

    Serial.print(F(" -> PCA port "));

    Serial.println(pcaPort);

  }

}

 

void ServoBus::detach(uint8_t channel) {

  if (channel >= SERVO_COUNT) return;

 

  if (_isGpioChannel(channel)) {

    // ========== GPIO Servo ==========

    if (_attached[channel]) {

      _gpioServos[channel].detach();

      _attached[channel] = false;

      Serial.print(F("[ServoBus] GPIO: Detached ch="));

      Serial.println(channel);

    }

  } else {

    // ========== PCA9685 Servo ==========

    if (_attached[channel]) {

      uint8_t pcaPort = _channelToPcaPort(channel);

      _pca9685.setPWM(pcaPort, 0, 0);  // Turn off PWM

      _attached[channel] = false;

      Serial.print(F("[ServoBus] PCA9685: Detached ch="));

      Serial.println(channel);

    }

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

 

  if (_isGpioChannel(channel)) {

    // ========== GPIO Servo ==========

    _gpioServos[channel].writeMicroseconds(clamped);

  } else {

    // ========== PCA9685 Servo ==========

    uint8_t pcaPort = _channelToPcaPort(channel);

    // Convert microseconds to 12-bit PWM value

    // PCA9685 uses 12-bit resolution (0-4095) at the set frequency

    // Formula: pwm = (microseconds * 4096 * frequency) / 1,000,000

    uint32_t pwm = (uint32_t)((clamped * 4096.0 * _freq) / 1000000.0);

    pwm = (pwm > 4095) ? 4095 : pwm;  // Clamp to 12-bit max

    _pca9685.setPWM(pcaPort, 0, pwm);

  }

}

 

void ServoBus::writeDegrees(uint8_t channel, float deg) {

  if (channel >= SERVO_COUNT) return;

  if (!_attached[channel]) return;

 

  uint16_t us = _degToUs(channel, deg);

  writeMicroseconds(channel, us);

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

 

  // Detach GPIO servos (channels 0-5)

  for (uint8_t ch = 0; ch < PCA9685_FIRST_CHANNEL; ++ch) {

    if (_attached[ch]) {

      _gpioServos[ch].detach();

      _attached[ch] = false;

    }

  }

 

  // Turn off PCA9685 servos (channels 6-15)

  for (uint8_t ch = PCA9685_FIRST_CHANNEL; ch < SERVO_COUNT; ++ch) {

    if (_attached[ch]) {

      uint8_t pcaPort = _channelToPcaPort(ch);

      _pca9685.setPWM(pcaPort, 0, 0);

      _attached[ch] = false;

    }

  }

}

