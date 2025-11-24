# ESP32-S3 Serial Output Troubleshooting Guide

Based on Reddit community feedback and common ESP32-S3 issues.

## 🔧 Pick the correct PlatformIO environment first

Two environments now live in `platformio.ini` so you can try **both** USB paths without manually toggling flags:

- `freenove_esp32_s3_otg` → Native USB-CDC via the OTG port (`/dev/tty.usbmodem*` on macOS). Includes `-DARDUINO_USB_MODE=1` and `-DARDUINO_USB_CDC_ON_BOOT=1`.
- `freenove_esp32_s3_uart` → External CH34x USB-UART bridge (`/dev/tty.wchusbserial*` on macOS). **No ARDUINO_USB_* flags.**

Switch between them with `-e <env>` or by changing `default_envs` at the top of `platformio.ini`:

```bash
# Try native USB-CDC first (OTG port)
pio run -e freenove_esp32_s3_otg -t upload && pio device monitor -e freenove_esp32_s3_otg

# If that fails, try the CH34x UART port
pio run -e freenove_esp32_s3_uart -t upload && pio device monitor -e freenove_esp32_s3_uart
```

This mirrors the Reddit guidance: test the bare board on the OTG port first, then fall back to the UART/CH34x path if needed.

---

## 📋 Step-by-Step Troubleshooting (From Reddit Community)

### STEP 1: Bare Board Test (MOST IMPORTANT)

**What Reddit says:**
> "By bare I mean just board connected and solely powered by right Type-C, with nothing else" - RooperK
> "Get your serial working first. Just hello world." - plierhead

**Action:**
1. **DISCONNECT EVERYTHING**:
   - Remove ALL servos
   - Remove PCA9685 board
   - Remove all wiring
   - Remove power supplies
   - ONLY USB-C cable to PC

2. **Use the minimal test**:
   ```bash
   # Backup your main code
   mv src/main.cpp src/main.cpp.backup

   # Copy minimal test
   cp test_serial_minimal.cpp src/main.cpp

   # Upload
   pio run -t upload

   # Open serial monitor
   pio device monitor
   ```

3. **Expected output**:
   ```
   ========================================
   ESP32-S3 MINIMAL SERIAL TEST
   ========================================
   If you can read this, USB-CDC is working!

   Hello from ESP32-S3! Counter: 0
   Hello from ESP32-S3! Counter: 1
   Hello from ESP32-S3! Counter: 2
   ```

**If this works**: Problem is in your main code
**If this fails**: Continue to Step 2

---

### STEP 2: Check USB Port Type (Critical for ESP32-S3)

**What Reddit says:**
> "Some S3 boards come with two USB C, one connected to UART0 and one connected directly to the S3 USB pins" - nitram_gorre

**Freenove ESP32-S3 WROOM has ONE USB-C port**:
- Connected directly to ESP32-S3 native USB
- Requires `-DARDUINO_USB_CDC_ON_BOOT=1` flag (already added ✅)
- No external USB-UART chip

**Quick environment sanity check** (matches the port names you see):
- If macOS shows `/dev/cu.usbmodem*`, use `-e freenove_esp32_s3_otg` (or set `default_envs` to that).
- If macOS shows `/dev/cu.wchusbserial*`, use `-e freenove_esp32_s3_uart` (and set `default_envs` accordingly).

**Check in Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac)**:
- Should show as "USB JTAG/serial debug unit" or similar
- NOT "CP210x" or "CH340" (those are external UART chips)

---

### STEP 3: Verify USB Driver Installation

**What Reddit says:**
> "driver for USB-UART converter might not be installed" - RooperK

**For Freenove ESP32-S3 (Native USB)**:
- Should use built-in CDC drivers (no special driver needed)
- Windows: Should auto-install
- Linux: Built-in kernel support
- Mac: Built-in support

**If not detected**:
1. Try different USB cable (some are charge-only)
2. Try different USB port
3. Check Device Manager for unknown devices
4. Manually install ESP32-S3 drivers from Espressif

---

### STEP 4: Check for Brown-Out Issues

**What Reddit says:**
> "most likely there's no serial because of brown out from so many PWMs" - RooperK
> "make sure you are not attempting to power them through the USB or 3v3" - nitram_gorre

**You mentioned testing with only 1 servo - GOOD!**

**But for final testing**:
- ❌ Don't power servos from ESP32 3.3V or 5V pins
- ✅ Use separate 5V/8.4V supply for servos
- ✅ Common ground between ESP32 and servo power
- ✅ Separate buck converters (you have this ✅)

**Brown-out can cause**:
- ESP32 to reset continuously
- Serial output to stop mid-initialization
- No serial output at all

---

### STEP 5: PlatformIO Upload Settings

**Check upload output carefully**:
```bash
pio run -t upload -v
```

**Look for**:
- Successful chip detection: "Detecting chip type... ESP32-S3"
- Successful flash: "Hash of data verified"
- Successful reset: "Leaving... Hard resetting via RTS pin..."

**If upload fails or gets stuck**:
1. Hold BOOT button while plugging in USB
2. Upload while holding BOOT
3. Press RST button after upload completes

---

### STEP 6: Serial Monitor Settings

**PlatformIO Monitor**:
```bash
pio device monitor --baud 115200
```

**Arduino IDE**:
- Select correct port
- Set baud to 115200
- Set line ending to "Newline"

**If using screen/minicom (Linux)**:
```bash
screen /dev/ttyACM0 115200
# or
minicom -D /dev/ttyACM0 -b 115200
```

---

## 🔍 Additional Checks

### Check Board Selection in PlatformIO

Your `platformio.ini` should show:
```ini
[env:freenove_esp32_s3_wroom]
platform = espressif32
board = freenove_esp32_s3_wroom
framework = arduino

build_flags =
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DARDUINO_USB_MODE=1
```

### Check for Library Conflicts

**Temporarily disable all libraries** to test:
```ini
lib_deps =
  # Comment out ALL libraries for bare test
  # bblanchon/ArduinoJson@^6.21.3
  # madhephaestus/ESP32Servo@^3.0.5
  # etc...
```

### Check Flash Settings

Some ESP32-S3 boards need specific flash settings:
```ini
board_build.flash_mode = dio  # ✅ You have this
board_build.f_flash = 80000000L  # ✅ You have this
```

---

## 🎯 Quick Decision Tree

```
Can you see serial output with minimal test sketch?
│
├─ YES → Problem is in your main code
│   ├─ Check ServoBus initialization
│   ├─ Check PCA9685 I2C communication
│   ├─ Add debug prints throughout setup()
│   └─ Check for stack overflow (large objects)
│
└─ NO → Problem is USB/driver/board config
    ├─ Try different USB cable
    ├─ Try different USB port
    ├─ Check Device Manager for device
    ├─ Verify board selection in platformio.ini
    ├─ Try holding BOOT button during upload
    └─ Check Windows/Linux driver installation
```

---

## 📝 Summary of Fixes Applied

1. ✅ **Fixed board configuration mismatch** - Changed environment name to match board
2. ✅ **Added USB-CDC flags** - Essential for ESP32-S3 serial
3. ✅ **Created minimal test sketch** - Isolate USB/serial from main code
4. ✅ **Added serial command handler** - Restored after Bluetooth removal

---

## 🚀 Recommended Next Steps

1. **Test with minimal sketch FIRST** (bare board, nothing connected)
2. **If minimal works**: Gradually add complexity
   - Add servo libraries
   - Add PCA9685
   - Add one servo
   - Test at each step
3. **If minimal fails**: Focus on USB/driver/board issues
   - Check USB cable
   - Check drivers
   - Try Arduino IDE instead of PlatformIO
   - Check for BOOT/RST button combinations

---

## 📞 If Still Stuck

Report back with:
1. Does minimal test work? (yes/no)
2. What shows in Device Manager when ESP32-S3 is plugged in?
3. Does upload succeed? (copy full upload log)
4. Any error messages during upload?
5. Which USB port are you using on the board?
