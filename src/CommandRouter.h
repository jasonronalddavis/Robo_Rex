#pragma once
#include <Arduino.h>

namespace CommandRouter {
  // Accepts either JSON like {"cmd":"rex_walk_forward","speed":0.7}
  // or plain strings like rex_roar
  void handle(const String& payload);
}
