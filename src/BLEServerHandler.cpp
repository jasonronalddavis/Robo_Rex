// BLEServerHandler.cpp
#include "BLEServerHandler.h"
#include <NimBLEDevice.h>
#include "CommandRouter.h"

static NimBLECharacteristic* rxChar = nullptr;
// Optional notify characteristic for logs back to client:
static NimBLECharacteristic* txChar = nullptr;

static const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"; // Nordic UART Service style
static const char* RX_UUID      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write
static const char* TX_UUID      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    String s = String(v.c_str());
    s.trim();
    if (s.length()) CommandRouter::handle(s);
  }
};

bool BLEServerHandler::begin(const char* deviceName) {
  NimBLEDevice::init(deviceName);
  NimBLEServer* server = NimBLEDevice::createServer();
  NimBLEService* svc = server->createService(SERVICE_UUID);

  rxChar = svc->createCharacteristic(RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());

  txChar = svc->createCharacteristic(TX_UUID, NIMBLE_PROPERTY::NOTIFY);

  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();
  return true;
}

void BLEServerHandler::notify(const String& msg) {
  if (!txChar) return;
  txChar->setValue(msg.c_str());
  txChar->notify();
}
