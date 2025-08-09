// BLEServerHandler.h
#pragma once
#include <Arduino.h>

namespace BLEServerHandler {
  bool begin(const char* deviceName);
  void notify(const String& msg); // optional TX notify if you add a notify characteristic
}
