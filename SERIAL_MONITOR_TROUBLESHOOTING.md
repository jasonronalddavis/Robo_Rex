# Serial Monitor Not Working - Step-by-Step Troubleshooting

## Issue
Upload succeeds, but serial monitor shows no output on Freenove ESP32-S3 WROOM with CH340 chip.

---

## Step 1: Try Both Serial Port Types (macOS)

On macOS, CH340 creates TWO devices:
```bash
# For UPLOAD (what you're using now):
/dev/cu.wchusbserial5AE70754231

# For MONITORING (try this):
/dev/tty.wchusbserial5AE70754231
```

**Test both:**

### Option A: Try tty.* port
```bash
# Open monitor with tty port
pio device monitor --port /dev/tty.wchusbserial5AE70754231
```

### Option B: Try cu.* port
```bash
# Open monitor with cu port (what upload used)
pio device monitor --port /dev/cu.wchusbserial5AE70754231
```

### Option C: Let PlatformIO auto-detect
```bash
# Just run monitor without specifying port
pio device monitor
```

---

## Step 2: Manual Reset After Upload

The board might not be auto-resetting properly.

**Try this sequence:**
```bash
# 1. Upload
pio run -t upload

# 2. Wait 2 seconds
sleep 2

# 3. Press RESET button on board (if available)

# 4. Open monitor immediately
pio device monitor
```

---

## Step 3: Use Screen Command (macOS Native)

Sometimes PlatformIO's monitor has issues. Try native macOS `screen`:

```bash
# Upload first
pio run -t upload

# Use screen to monitor (try tty port first)
screen /dev/tty.wchusbserial5AE70754231 115200

# Or try cu port
screen /dev/cu.wchusbserial5AE70754231 115200

# To exit screen: Press Ctrl-A then K, then Y
```

---

## Step 4: Check if Code is Running (LED Blink Test)

Upload the LED blink test to verify the ESP32 is actually running code:

```bash
# Use the LED test sketch
mv src/main.cpp src/main.cpp.backup
cp test_led_serial.cpp src/main.cpp

# Upload
pio run -t upload

# Look for blinking LED on GPIO 48 (onboard RGB LED)
# If LED blinks: Code IS running, serial output is the issue
# If LED doesn't blink: Code is NOT running, deeper problem
```

---

## Step 5: Increase Serial Init Delay

ESP32-S3 sometimes needs more time for serial to stabilize.

**Already applied in test sketch, but for reference:**
```cpp
void setup() {
  Serial.begin(115200);
  delay(2000);  // Increased from 500ms to 2000ms

  while(!Serial && millis() < 5000) {
    delay(10);  // Wait for serial, but timeout after 5s
  }

  Serial.println("Starting...");
}
```

---

## Step 6: Check for Port Conflicts

Make sure no other program is using the serial port:

```bash
# Check for processes using the port
lsof | grep wchusbserial

# Kill any conflicting processes if found
# (but be careful - don't kill system processes)
```

---

## Step 7: CH340 Driver Verification (macOS)

Check if CH340 driver is properly loaded:

```bash
# Check system info
system_profiler SPUSBDataType | grep -A 10 "CH340"

# Or check kernel extensions
kextstat | grep -i ch34

# If driver not loaded, may need to install:
# https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
```

---

## Step 8: Test with Arduino IDE

If PlatformIO still doesn't work, try Arduino IDE:

1. Open Arduino IDE
2. Tools → Board → ESP32-S3 Dev Module
3. Tools → Port → /dev/cu.wchusbserial* (or tty.*)
4. Tools → Serial Monitor → 115200 baud
5. Upload the minimal sketch
6. Watch for output

---

## Step 9: Hardware Checks

### Check USB Cable
- Try a different USB cable (some are power-only)
- Try a different USB port on your computer

### Check Board Power
- Does the power LED turn on?
- Try powering from external 5V supply (if available)

### Check for Short Circuits
- Disconnect ALL servos and peripherals
- Test with ONLY USB connected

---

## Quick Test Sequence (Recommended)

Run these commands in order:

```bash
# 1. Clean build
pio run -t clean

# 2. Upload
pio run -t upload

# 3. Wait for upload to complete and board to reset

# 4. Try monitor with tty port
pio device monitor --port /dev/tty.wchusbserial5AE70754231 --baud 115200

# If nothing appears after 5 seconds, press RESET button on board

# If still nothing, Ctrl-C to exit and try:
screen /dev/tty.wchusbserial5AE70754231 115200
```

---

## Expected Output (If Working)

You should see:
```
============================================
    ROBO REX - 16 SERVO HYBRID MODE
   GPIO (0-5) + PCA9685 (6-15)
============================================

[Hybrid] Initializing servo system...
[ServoBus] Initializing HYBRID servo system
...
```

---

## If Still No Output

If none of the above works, we need to determine:

1. **Is the code running at all?** → Use LED blink test
2. **Is the serial port receiving data?** → Try different terminal programs
3. **Is there a hardware issue?** → Test minimal sketch on bare board

Report back with:
- Which port type worked (cu vs tty)?
- Does LED blink in LED test?
- What does `lsof | grep wchusbserial` show?
- Does Arduino IDE work?

---

## Most Likely Solutions

Based on common ESP32-S3 + CH340 issues on macOS:

1. ✅ **Use `/dev/tty.wchusbserial*` instead of `/dev/cu.wchusbserial*`** for monitoring
2. ✅ **Manually press RESET button** after upload
3. ✅ **Use `screen` command** instead of PlatformIO monitor
4. ✅ **Add 2-second delay** after Serial.begin()
