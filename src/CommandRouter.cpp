#include "CommandRouter.h"
#include <ArduinoJson.h>
#include "ServoBus.h"
#include "Servo Functions/Leg_Function.h"
#include "Servo Functions/Spine_Function.h"
#include "Servo Functions/Head_Function.h"
#include "Servo Functions/Neck_Function.h"
#include "Servo Functions/Tail_Function.h"
#include "Servo Functions/Pelvis_Function.h"

static float getOr(const JsonVariant v, const char* key, float def) {
  if (!v.isNull() && v.containsKey(key)) return v[key].as<float>();
  return def;
}

namespace CommandRouter {
void handle(const String& s) {
  // Try JSON first
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, s);
  if (!err) {
    String cmd = doc["cmd"] | "";
    if (cmd == "rex_walk_forward") { Leg::walkForward(getOr(doc,"speed",0.7)); return; }
    if (cmd == "rex_walk_backward"){ Leg::walkBackward(getOr(doc,"speed",0.7)); return; }
    if (cmd == "rex_turn_left")    { Leg::turnLeft(getOr(doc,"rate",0.6));     return; }
    if (cmd == "rex_turn_right")   { Leg::turnRight(getOr(doc,"rate",0.6));    return; }
    if (cmd == "rex_run")          { Leg::setGait(getOr(doc,"factor",1.5), 0.6, 0.4, "run"); return; }
    if (cmd == "rex_stop")         { Leg::stop(); return; }
    if (cmd == "rex_gait")         { Leg::setGait(getOr(doc,"speed",0.7), getOr(doc,"stride",0.6), getOr(doc,"lift",0.4), (const char*)doc["mode"] | "walk"); return; }
    if (cmd == "rex_speed_adjust") { Leg::adjustSpeed(getOr(doc,"delta",0.1)); return; }
    if (cmd == "rex_stride_set")   { Leg::setStride(getOr(doc,"value",0.6)); return; }
    if (cmd == "rex_posture")      { Leg::setPosture(getOr(doc,"level",0.5)); return; }

    if (cmd == "rex_spine_up")     { Spine::up(); return; }
    if (cmd == "rex_spine_down")   { Spine::down(); return; }
    if (cmd == "rex_spine_set")    { Spine::set(getOr(doc,"level",0.5)); return; }

    if (cmd == "rex_tail_wag")     { Tail::wag(); return; }
    if (cmd == "rex_tail_set")     { Tail::set(getOr(doc,"level",0.5)); return; }

    if (cmd == "rex_roar")         { Head::roar(); return; }
    if (cmd == "rex_mouth_open")   { Head::mouthOpen(); return; }
    if (cmd == "rex_mouth_close")  { Head::mouthClose(); return; }

    if (cmd == "rex_pelvis_set")   { Pelvis::set(getOr(doc,"level",0.5)); return; }
    return;
  }

  // Plain string fallback:
  if (s == "rex_roar") { Head::roar(); return; }
  if (s == "rex_tail_wag") { Tail::wag(); return; }
  if (s == "rex_spine_up") { Spine::up(); return; }
  if (s == "rex_spine_down") { Spine::down(); return; }
  if (s == "rex_walk_forward") { Leg::walkForward(0.7); return; }
  if (s == "rex_turn_left") { Leg::turnLeft(0.6); return; }
  if (s == "rex_turn_right") { Leg::turnRight(0.6); return; }
}
}
