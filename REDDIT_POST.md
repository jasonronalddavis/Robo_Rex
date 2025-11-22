# [HELP] ESP32-S3 Robotic T-Rex Project - No Serial Output, PCA9685 Not Working, Servos Not Moving

## Hardware Setup
- **Board**: ESP32-S3 Freenove WROOM
- **MCU**: ESP32-S3 dual-core
- **Servo Driver**: PCA9685 (I2C @ 0x40)
- **Servos**: 16 total
  - 6 servos on GPIO (channels 0-5): Neck, Head (2), Pelvis, Spine, Tail
  - 10 servos on PCA9685 (channels 6-15): Leg servos via I2C

## Pin Configuration
**GPIO Servos (Direct ESP32 Control)**
- CH0: GPIO 1 (Neck Yaw)
- CH1: GPIO 2 (Head Jaw)
- CH2: GPIO 3 (Head Pitch)
- CH3: GPIO 7 (Pelvis Roll) - **NOTE: Using GPIO 7, NOT 4**
- CH4: GPIO 10 (Spine Yaw)
- CH5: GPIO 6 (Tail Wag)

**PCA9685 I2C Pins**
- SDA: GPIO 4
- SCL: GPIO 5
- I2C Address: 0x40
- I2C Clock: 100kHz

## The Problem
I'm experiencing three major issues:

### 1. **No Serial Output Whatsoever**
- Serial.begin(115200) is called in setup()
- Multiple Serial.println() statements throughout initialization
- Nothing appears in the serial monitor
- Have tried different baud rates (9600, 115200)
- Have tried both Arduino IDE and PlatformIO serial monitors

### 2. **PCA9685 Not Operating**
- Leg servos (channels 6-15) connected to PCA9685 do not move
- No way to verify if I2C communication is working (due to no serial output)
- Code should print "SUCCESS!" or "FAILED!" when detecting PCA9685

### 3. **All Servos Not Moving**
- Neither GPIO servos (0-5) nor PCA9685 servos (6-15) are responding
- Sweep test is enabled by default (should sweep all 16 servos 10-170°)

## What I've Already Tried

### Hardware Verification ✅
- **Voltage verified with multimeter**: Proper voltage to all components
- **Swapped ALL hardware**: New ESP32-S3, new PCA9685, new servos
- **Wiring checked multiple times**: Continuity tested
- **Power supply adequate**: 5V/10A for servos, separate from ESP32

### Software Attempts
- Flashed multiple times
- Verified code compiles without errors
- Tried different USB cables/ports
- No errors during upload process
- Upload appears successful (100% complete)

## Code Architecture

**PlatformIO Configuration:**
```ini
[env:adafruit_feather_esp32s3]
platform = espressif32
board = freenove_esp32_s3_wroom
framework = arduino
upload_speed = 115200
monitor_speed = 115200

build_flags =
  -DIMU_SENSOR_MPU6050
  -DIMU_SDA_PIN=8
  -DIMU_DEBUG
  -DPCA9685_SDA_PIN=4
  -DPCA9685_SCL_PIN=5

lib_deps =
  bblanchon/ArduinoJson@^6.21.3
  madhephaestus/ESP32Servo@^3.0.5
  adafruit/Adafruit PWM Servo Driver Library@^3.0.1
  adafruit/Adafruit MPU6050@^2.2.5
  adafruit/Adafruit BusIO@^1.15.0
  https://github.com/arduino-libraries/MadgwickAHRS.git
  https://github.com/Xander-Electronics/Base64.git
```

**Initialization Sequence in setup():**
```cpp
void setup() {
  Serial.begin(115200);
  delay(500);

  // Should print banner
  Serial.println(F("============================================"));
  Serial.println(F("    ROBO REX - 16 SERVO HYBRID MODE"));
  // ... more messages

  // Initialize ServoBus (GPIO + PCA9685)
  servoBus.begin();  // Should print detailed debug info

  // Initialize all servo functions
  Neck::begin(&servoBus, neckMap);
  Head::begin(&servoBus, headMap);
  // ... etc

  // Attach all 16 servos
  for (uint8_t ch = 0; ch < 16; ch++) {
    servoBus.attach(ch);  // Should print for each channel
  }

  // Ready message
  Serial.println(F("SYSTEM READY"));
}
```

**ServoBus::begin() should print:**
- Initialization messages for hybrid system
- GPIO LEDC timer allocation
- PCA9685 I2C initialization on Wire bus
- Either "SUCCESS!" or "FAILED!" for PCA9685 detection
- Error codes if PCA9685 not found

**Sweep Test (enabled by default):**
```cpp
#define ENABLE_SWEEP_TEST true
// Sweeps all 16 servos from 10° to 170° continuously
```

## Expected Behavior vs Actual

**Expected:**
1. Serial monitor shows initialization messages
2. PCA9685 detection succeeds
3. All 16 servos attach successfully (with printed confirmations)
4. Sweep test runs, moving all servos smoothly
5. Can send commands via Serial (e.g., "HELP", "STATUS", "WALK_FORWARD")

**Actual:**
1. ❌ No serial output at all
2. ❌ No servo movement
3. ❌ No visible signs of operation
4. ✅ Upload completes successfully
5. ✅ Code compiles without errors

## Questions

1. **Why would there be absolutely no serial output?** Is this a USB-CDC issue specific to ESP32-S3?
   - Do I need `-DARDUINO_USB_CDC_ON_BOOT=1` or `-DARDUINO_USB_MODE=1` build flags?

2. **Is GPIO 4 and 5 safe for I2C on ESP32-S3?** Some sources suggest these pins might have issues.
   - Should I use different I2C pins?

3. **Could the ESP32Servo library conflict with PCA9685?**
   - Using ESP32Servo for GPIO servos (channels 0-5)
   - Using Adafruit_PWMServoDriver for PCA9685 (channels 6-15)
   - Both in the same sketch

4. **Are there known issues with ESP32-S3 WROOM and USB serial?**
   - Do I need to select a specific USB mode in Arduino IDE?
   - Is there a CDC/JTAG configuration needed?

5. **Could all servos drawing current cause ESP32 to brown out during init?**
   - Even though they're on separate 5V supply?
   - Would this prevent serial output?

## Additional Notes
- This is a hybrid servo control system (first 6 GPIO, last 10 PCA9685)
- Previous commits show I recently removed IMU and Bluetooth functionality
- Using PlatformIO with VS Code
- The code structure works fine with just GPIO servos on other ESP32 boards

---

Any help would be greatly appreciated! I've been stuck on this for days. With zero serial output, I can't even begin to debug what's happening. Is there something fundamental I'm missing about ESP32-S3 serial communication?

**Repository**: https://github.com/jasonronalddavis/Robo_Rex (branch: claude/remove-bluetooth-012h5HTBtNmRaZ5DUYFBD1gk)
