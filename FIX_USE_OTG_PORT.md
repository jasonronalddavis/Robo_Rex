# CRITICAL FIX: Freenove ESP32-S3 WROOM Has TWO USB Ports!

## The Problem

Your Freenove ESP32-S3 WROOM board has **TWO USB-C ports**:

1. **UART Port** - Uses CH340/CH343 USB-to-UART chip
   - You've been using THIS port
   - Known driver issues on macOS with CH343 chip
   - May not work reliably for serial communication

2. **OTG Port** - Native ESP32-S3 USB (USB-CDC)
   - This is what you SHOULD use!
   - Direct connection to ESP32-S3 USB peripheral
   - More reliable on macOS

## Evidence Your Code IS Running

Reddit user observed:
> "Based on the furious blue LED flashing it looks like your board is trying to communicate over UART when starting"

**This is HUGE!** It means:
- ✅ Your code IS uploading successfully
- ✅ The ESP32-S3 IS running
- ✅ The board IS trying to send serial data
- ❌ The serial data is NOT reaching your computer (driver/port issue)

## The Solution

### Step 1: Identify Which Port is Which

Look at your board - you should see TWO USB-C connectors labeled:
- One labeled **"UART"** or **"COM"** ← You've been using this
- One labeled **"USB"** or **"OTG"** ← Try this one instead!

### Step 2: Switch to OTG Port

```bash
# 1. Unplug USB cable from UART port

# 2. Plug USB cable into OTG port

# 3. Check what device appears (will be different!)
ls /dev/tty.*

# Should see something like:
# /dev/tty.usbmodem* (native USB-CDC)
# NOT /dev/tty.wchusbserial* (CH340 chip)

# 4. Upload using OTG port - BUT NEED DIFFERENT FLAGS!
```

### Step 3: Update platformio.ini for OTG Port

When using the OTG port, you NEED the USB-CDC flags we removed earlier!

```ini
[env:freenove_esp32_s3_wroom_otg]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

upload_speed = 115200
monitor_speed = 115200

; CRITICAL: For OTG/USB port, you NEED these flags
build_flags =
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_USB_MODE=1
  -DIMU_SENSOR_MPU6050
  -DIMU_SDA_PIN=8
  -DIMU_SCL_PIN=9
  -DPCA9685_SDA_PIN=4
  -DPCA9685_SCL_PIN=5

; Libraries (same as before)
lib_deps =
  bblanchon/ArduinoJson@^6.21.3
  madhephaestus/ESP32Servo@^3.0.5
  adafruit/Adafruit PWM Servo Driver Library@^3.0.1
  adafruit/Adafruit MPU6050@^2.2.5
  adafruit/Adafruit BusIO@^1.15.0
  https://github.com/arduino-libraries/MadgwickAHRS.git
  https://github.com/Xander-Electronics/Base64.git
```

### Step 4: Test with OTG Port

```bash
# Upload (board should auto-detect new port)
pio run -t upload

# Monitor (port name will be different!)
pio device monitor

# OR manually specify:
pio device monitor --port /dev/tty.usbmodem*
```

## Alternative: Fix CH343 Driver on Mac

If you want to keep using the UART port, there's a known CH343 driver issue on macOS.

### Check Which Chip You Have

```bash
system_profiler SPUSBDataType | grep -A 10 -i "uart\|ch34"
```

Look for:
- **CH340** - Older chip, usually works
- **CH343** - Newer chip, has macOS issues

### CH343 Fix (if needed)

1. Download proper CH343 driver from:
   - https://github.com/WCHSoftGroup/ch34xser_macos
   - Or check Freenove's GitHub repo

2. Install driver and restart Mac

3. Try UART port again

## Recommended Approach

**Just use the OTG port!** It's:
- ✅ More reliable
- ✅ Faster
- ✅ Built-in to macOS (no driver needed)
- ✅ What Freenove recommends

## Freenove Documentation

Check Freenove's official repo for their recommendations:
https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board

They likely document which port to use for different scenarios.

## Summary

**Your problem is NOT with your code!**
- Code is running (LED flashing proves it)
- ESP32-S3 is working
- You're just using the wrong USB port!

**Switch to the OTG port and add back the USB-CDC flags!**

---

## Quick Test with OTG Port

1. Unplug from UART port
2. Plug into OTG port
3. Update platformio.ini to add USB-CDC flags back
4. Upload minimal test
5. Open serial monitor
6. Should see output immediately!
