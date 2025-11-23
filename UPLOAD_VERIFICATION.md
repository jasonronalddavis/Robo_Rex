# Upload Verification for Freenove ESP32-S3 WROOM

## Check if Upload is Actually Working

When you run `pio run -t upload`, look for these critical lines:

### Should See:
```
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
...
Hash of data verified.  ← CRITICAL - Should see this multiple times
...
Leaving...
Hard resetting via RTS pin...  ← Board should reset here
```

### If You See:
- **"Chip is ESP32-S3"** → Good, chip detected
- **"Hash of data verified"** → Good, code uploaded successfully
- **"Hard resetting via RTS pin"** → Board should auto-reset and run code

### Problem Signs:
- Chip detected but then **timeout** or **error** → Upload failed
- No **"Hard resetting"** message → Board didn't reset, code not running
- Upload succeeds but board doesn't reset → Need manual RESET button press

---

## Manual Reset Procedure

If upload succeeds but board doesn't auto-reset:

```bash
# 1. Upload
pio run -t upload

# 2. Wait for "Hard resetting via RTS pin..." message

# 3. If board doesn't reset automatically, manually:
#    Press RESET button on board

# 4. Immediately open screen
screen /dev/tty.wchusbserial5AE70754231 115200
```

---

## Try Different Board Definition

The Freenove ESP32-S3 WROOM might need generic ESP32-S3 settings.

Edit `platformio.ini` and try this board instead:

```ini
[env:freenove_esp32_s3_wroom]
platform = espressif32
board = esp32-s3-devkitc-1  ; Generic ESP32-S3 with external UART
framework = arduino
```

Then:
```bash
pio run -t clean
pio run -t upload
```

---

## Check for Boot Mode Issues

ESP32-S3 can get stuck in download mode. Force normal boot:

### Physical Button Sequence:
1. **Press and hold BOOT button**
2. **While holding BOOT, press RESET button**
3. **Release RESET button** (still holding BOOT)
4. **Release BOOT button**
5. Board should now boot normally
6. Try opening serial monitor

---

## Verify CH340 Driver

Check if macOS sees the CH340:

```bash
# Check USB devices
system_profiler SPUSBDataType | grep -i "ch340\|wch\|uart"

# Should see something like:
# Product ID: 0x7523
# Vendor ID: 0x1a86 (Jiangsu Qinheng Co., Ltd.)
```

If nothing appears, CH340 driver might not be loaded.

---

## Alternative: Check Raw Upload Output

Run upload with verbose mode to see exactly what's happening:

```bash
pio run -t upload -v 2>&1 | tee upload.log
```

This saves full upload log. Look for any errors or warnings.

---

## Nuclear Option: Erase Flash

Sometimes ESP32-S3 flash gets corrupted:

```bash
# Erase entire flash
pio run -t erase

# Then upload again
pio run -t upload
```

This forces a completely clean slate.

---

## "Port is busy or doesn't exist" (most common upload failure)

PlatformIO will throw this when the selected environment doesn't match the USB port you are actually using, or when another program still has the port open.

1. **Close anything that might be holding the port**
   - Quit Serial Monitor / screen / minicom / Arduino IDE
   - If using VS Code, close the built-in serial monitor tab

2. **Confirm which device node appears after plugging in**
   ```bash
   ls /dev/cu.*
   ```

   - **OTG/native USB-CDC (``freenove_esp32_s3_otg`` env):** Expect `/dev/cu.usbmodem*` on macOS (or `/dev/ttyACM*` on Linux).
   - **CH34x UART bridge (``freenove_esp32_s3_uart`` env):** Expect `/dev/cu.wchusbserial*` on macOS (or `/dev/ttyUSB*` on Linux).

3. **Run upload with the matching environment and port**
   ```bash
   # Native USB-CDC via OTG port
   pio run -e freenove_esp32_s3_otg -t upload --upload-port /dev/cu.usbmodemXXXXX

   # CH34x UART bridge
   pio run -e freenove_esp32_s3_uart -t upload --upload-port /dev/cu.wchusbserialXXXXX
   ```

4. **If the port name keeps changing** (some macOS systems): rerun `ls /dev/cu.*` each time to grab the new path before uploading.

5. **If VS Code keeps invoking the wrong environment** (e.g., `freenove_esp32_s3_otg` while your port is `/dev/cu.wchusbserial*`), edit `platformio.ini` and set:
   ```ini
   default_envs = freenove_esp32_s3_uart
   ```
   Then retry the upload so the flags and port expectations match your CH34x connection.

If you still see the error after matching env + port and closing all monitors, unplug/replug the USB cable and retry.
