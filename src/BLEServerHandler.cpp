// #include "BLEServerHandler.h"

// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLE2902.h>

// #include "CommandRouter.h"

// // Nordic UART UUIDs (match your web client)
// static const char* SERVICE_UUID          = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
// static const char* CHARACTERISTIC_UUID_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; // Write (Web -> ESP32)
// static const char* CHARACTERISTIC_UUID_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // Notify (ESP32 -> Web)

// // State
// static BLEServer*         g_server = nullptr;
// static BLECharacteristic* g_tx     = nullptr; // notify
// static BLECharacteristic* g_rx     = nullptr; // write

// static bool     g_clientConnected = false;
// static String   g_rxBuffer;
// static bool     g_echoSerial = false;
// static uint16_t g_mtu = 23;  // default ATT MTU before exchange

// // Forward decls
// static void startAdvertising();

// // -------------------- Callbacks --------------------
// class RexServerCallbacks : public BLEServerCallbacks {
//   void onConnect(BLEServer* pServer) override {
//     g_clientConnected = true;
//     // Update MTU if available; note: MTU is negotiated per connection.
//     // Many stacks use 185 as practical max for notifications.
//     // We read it from the first connected peer.
//     g_mtu = pServer->getConnId() >= 0 ? pServer->getPeerMTU(pServer->getConnId()) : g_mtu;
// #ifdef SERIAL_DEBUG
//     Serial.print(F("[BLE] Central connected, MTU="));
//     Serial.println(g_mtu);
// #endif
//   }
//   void onDisconnect(BLEServer* pServer) override {
//     g_clientConnected = false;
// #ifdef SERIAL_DEBUG
//     Serial.println(F("[BLE] Central disconnected"));
// #endif
//     // Resume advertising so web client can reconnect.
//     startAdvertising();
//   }
// };

// class RexRxCallbacks : public BLECharacteristicCallbacks {
//   void onWrite(BLECharacteristic* pCharacteristic) override {
//     std::string value = pCharacteristic->getValue();
//     if (value.empty()) return;

//     // Append to buffer and split on '\n'
//     g_rxBuffer += String(value.c_str());

//     int nl;
//     while ((nl = g_rxBuffer.indexOf('\n')) >= 0) {
//       String line = g_rxBuffer.substring(0, nl);
//       g_rxBuffer.remove(0, nl + 1);
//       line.trim();
//       if (line.length() == 0) continue;

//       if (g_echoSerial) {
//         Serial.print(F("[BLE RX] "));
//         Serial.println(line);
//       }

//       // Route the complete line to the command router
//       CommandRouter::handleLine(line);
//     }
//   }
// };

// // -------------------- Internals --------------------
// static void startAdvertising() {
//   if (!g_server) return;
//   BLEAdvertising* adv = BLEDevice::getAdvertising();
//   adv->setScanResponse(false);
//   adv->setMinPreferred(0x06);  // functions that help with iPhone connections issue
//   adv->setMinPreferred(0x12);
//   BLEDevice::startAdvertising();
// #ifdef SERIAL_DEBUG
//   Serial.println(F("[BLE] Advertising started"));
// #endif
// }

// // -------------------- Public API --------------------
// namespace BLEServerHandler {

// void begin(const char* deviceName) {
//   BLEDevice::init(deviceName && *deviceName ? deviceName : "Robo_Rex_ESP32S3");

//   g_server = BLEDevice::createServer();
//   g_server->setCallbacks(new RexServerCallbacks());

//   // Service
//   BLEService* service = g_server->createService(SERVICE_UUID);

//   // TX (Notify, ESP32 -> Web)
//   g_tx = service->createCharacteristic(
//       CHARACTERISTIC_UUID_TX,
//       BLECharacteristic::PROPERTY_NOTIFY
//   );
//   g_tx->addDescriptor(new BLE2902()); // CCCD

//   // RX (Write, Web -> ESP32)
//   g_rx = service->createCharacteristic(
//       CHARACTERISTIC_UUID_RX,
//       BLECharacteristic::PROPERTY_WRITE |
//       BLECharacteristic::PROPERTY_WRITE_NR
//   );
//   g_rx->setCallbacks(new RexRxCallbacks());

//   service->start();

//   // Start advertising
//   BLEAdvertising* adv = BLEDevice::getAdvertising();
//   adv->addServiceUUID(SERVICE_UUID);
//   adv->setScanResponse(false);
//   adv->setMinPreferred(0x06);
//   adv->setMinPreferred(0x12);
//   startAdvertising();

// #ifdef SERIAL_DEBUG
//   Serial.println(F("[BLE] Server ready (NUS)"));
// #endif
// }

// void tick() {
//   // Nothing required; callbacks handle traffic.
// }

// bool isClientConnected() {
//   return g_clientConnected;
// }

// static inline size_t notifChunkSize() {
//   // ATT MTU includes 3-byte L2CAP/ATT header; ESP-IDF notify payload max is (mtu - 3).
//   // Keep a margin for safety.
//   uint16_t mtu = g_mtu < 23 ? 23 : g_mtu;
//   uint16_t maxPayload = (mtu > 23) ? (mtu - 3) : 20;
//   // Cap at a sensible default to avoid fragmentation issues on some stacks.
//   if (maxPayload > 180) maxPayload = 180;
//   return (size_t)maxPayload;
// }

// bool notifyLine(const String& lineIn) {
//   if (!g_tx || !g_clientConnected) return false;

//   // Ensure newline-terminated and chunk if needed
//   String line = lineIn;
//   if (!line.endsWith("\n")) line += "\n";

//   const size_t CHUNK = notifChunkSize();
//   size_t offset = 0;
//   while (offset < line.length()) {
//     size_t len = min(CHUNK, (size_t)(line.length() - offset));
//     g_tx->setValue((uint8_t*)line.substring(offset, offset + len).c_str(), len);
//     g_tx->notify();
//     offset += len;
//     // Small yield to be nice to the BLE stack
//     delay(1);
//   }
//   return true;
// }

// void setEcho(bool on) {
//   g_echoSerial = on;
// }

// uint16_t currentMtu() {
//   return g_mtu;
// }

// } // namespace BLEServerHandler
