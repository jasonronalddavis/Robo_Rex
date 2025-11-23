# Pull Request: Fix ESP32-S3 serial communication and remove Bluetooth functionality

## Summary

This PR addresses critical ESP32-S3 serial communication issues and removes all Bluetooth functionality to simplify debugging. Multiple fixes were applied based on troubleshooting and Reddit community feedback.

## Critical Fixes

### 1. ✅ ESP32-S3 USB-CDC Configuration
- **Added build flags**: `-DARDUINO_USB_CDC_ON_BOOT=1` and `-DARDUINO_USB_MODE=1`
- **Issue**: ESP32-S3 requires these flags for `Serial.begin()` to function
- **Impact**: Resolves complete lack of serial output
- **File**: `platformio.ini`

### 2. ✅ Board Configuration Mismatch (Reddit Community Fix)
- **Changed**: `[env:adafruit_feather_esp32s3]` → `[env:freenove_esp32_s3_wroom]`
- **Issue**: Environment name didn't match board type, causing USB enumeration failures
- **Impact**: Proper board initialization and USB communication
- **File**: `platformio.ini`
- **Credit**: Spotted by Reddit user nitram_gorre

### 3. ✅ Serial Command Handler Restoration
- **Added**: Command buffer and serial reading in `loop()`
- **Issue**: `handleCommand()` was orphaned after Bluetooth removal
- **Impact**: Enables interactive control via serial monitor (HELP, STATUS, etc.)
- **File**: `src/main.cpp`

### 4. ✅ Removed All Bluetooth Functionality
- **Removed**: NimBLE library, BLE includes, callbacks, and setup code
- **Impact**: Simplifies codebase, reduces memory usage, easier debugging
- **Files**: `src/main.cpp`, `platformio.ini`, `notes`

## Testing & Troubleshooting Tools Added

### Minimal Serial Test (`test_serial_minimal.cpp`)
- Bare-bones "Hello World" sketch for ESP32-S3
- Tests ONLY serial communication, no libraries or hardware
- Recommended first step for troubleshooting
- Usage instructions included in file

### Troubleshooting Guide (`TROUBLESHOOTING_SERIAL.md`)
- Comprehensive step-by-step debugging process
- Decision tree for diagnosing serial vs hardware issues
- Reddit community insights documented
- Covers: bare board testing, USB drivers, brown-out issues

### Reddit Post (`REDDIT_POST.md`)
- Complete problem documentation for community assistance
- Hardware specs, pin configuration, troubleshooting attempts
- Repository reference for community review

## Changes by File

**platformio.ini**
- Fixed environment name to match board type
- Added USB-CDC build flags for ESP32-S3

**src/main.cpp**
- Removed all Bluetooth/BLE code
- Added serial command buffer (`g_cmdBuffer`)
- Implemented serial reading in `loop()` function
- Removed orphaned BLE callback classes
- Removed `txNotify()` helper and all calls

**notes**
- Removed NimBLE library dependency

**New Files**
- `test_serial_minimal.cpp` - Minimal test sketch
- `TROUBLESHOOTING_SERIAL.md` - Debugging guide
- `REDDIT_POST.md` - Community assistance request

## Expected Behavior After Merge

### Serial Output Should Show:
```
============================================
    ROBO REX - 16 SERVO HYBRID MODE
   GPIO (0-5) + PCA9685 (6-15)
============================================

[Hybrid] Initializing servo system...
[ServoBus] Initializing HYBRID servo system
  Channels 0-5:  ESP32 GPIO direct control
  Channels 6-15: PCA9685 I2C PWM driver
[ServoBus] PCA9685: Initializing on Wire (SDA=4, SCL=5) @ 0x40
[ServoBus] PCA9685: Calling begin()... SUCCESS!
...
[SWEEP] Position: 10.0 deg - Writing to all 16 channels
```

### Interactive Commands Available:
- `HELP` - Show all available commands
- `STATUS` - Display system status
- `SWEEP_ON` / `SWEEP_OFF` - Control servo sweep test
- Movement commands: `WALK_FORWARD`, `TURN_LEFT`, `ROAR`, etc.

## Testing Recommendations

1. **Test minimal sketch first** (bare board, nothing connected)
   ```bash
   mv src/main.cpp src/main.cpp.backup
   cp test_serial_minimal.cpp src/main.cpp
   pio run -t upload && pio device monitor
   ```

2. **If minimal works**, restore main code and test full system
   ```bash
   mv src/main.cpp.backup src/main.cpp
   pio run -t upload && pio device monitor
   ```

3. **If servos still don't move**, check serial output for specific errors

## Community Credits

Special thanks to Reddit r/esp32 community members:
- **nitram_gorre** - Spotted board configuration mismatch
- **RooperK** - Suggested bare board testing, brown-out diagnosis
- **plierhead** - Recommended "Hello World" first approach

## Related Issues

Resolves:
- ❌ No serial output whatsoever
- ❌ PCA9685 not operating (can now debug via serial)
- ❌ Servos not moving (can now see initialization errors)
- ❌ Unable to send commands to robot

## Commits Included

- `0f4a78d` Fix critical board configuration mismatch and add minimal test
- `80f0576` Fix ESP32-S3 serial communication and restore command handling
- `6381d3c` Add comprehensive Reddit post for ESP32 troubleshooting
- `3341c61` Remove all Bluetooth functionality

---

**Ready for review and testing!** 🚀
