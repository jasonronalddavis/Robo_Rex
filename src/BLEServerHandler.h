// #pragma once
// #include <Arduino.h>

// // Minimal Nordic UART–style BLE server for ESP32.
// // RX (write from client) -> line-buffered -> CommandRouter::handleLine(line)
// // TX (notify to client)  -> use notifyLine() to send newline-terminated text.

// namespace BLEServerHandler {

// // Start BLE with the given advertised device name (default matches your web app filter).
// void begin(const char* deviceName = "Robo_Rex_ESP32S3");

// // Optional periodic tasks (not strictly needed, provided for symmetry).
// void tick();

// // True if at least one central is connected.
// bool isClientConnected();

// // Send a newline-terminated UTF-8 line to the client via TX notify.
// // Large payloads are chunked to fit ATT MTU.
// bool notifyLine(const String& line);

// // Convenience: send a small JSON string (will append '\n' if missing).
// inline bool notifyJson(const String& json) { return notifyLine(json); }

// // Enable/disable serial echo of every received line (off by default).
// void setEcho(bool on);

// // Current ATT MTU seen by the server (informational).
// uint16_t currentMtu();

// } // namespace BLEServerHandler
