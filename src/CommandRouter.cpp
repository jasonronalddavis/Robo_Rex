#include "CommandRouter.h"
#include <ArduinoJson.h>
#include "ServoBus.h"
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

// Include IMU controls if present
#include "IMU.h"

// --------- helpers ---------
static float getOrF(const JsonVariantConst& v, const char* key, float defVal) {
  if (!v.isNull() && v.containsKey(key)) return v[key].as<float>();
  return defVal;
}

static const char* getOrCStr(const JsonVariantConst& v, const char* key, const char* defVal) {
  if (!v.isNull() && v.containsKey(key)) {
    const char* s = v[key];
    return s ? s : defVal;
  }
  return defVal;
}

// --------- router ----------
namespace CommandRouter {

void handle(const String& s) {
  // Try JSON first
  StaticJsonDocument<384> doc;  // a bit larger for future fields
  DeserializationError err = deserializeJson(doc, s);

  if (!err) {
    String cmd = doc["cmd"] | "";

    // ---- Locomotion ----
    if (cmd == "rex_walk_forward") { Leg::walkForward(getOrF(doc, "speed", 0.7f)); return; }
    if (cmd == "rex_walk_backward"){ Leg::walkBackward(getOrF(doc, "speed", 0.7f)); return; }
    if (cmd == "rex_turn_left")    { Leg::turnLeft   (getOrF(doc, "rate",  0.6f)); return; }
    if (cmd == "rex_turn_right")   { Leg::turnRight  (getOrF(doc, "rate",  0.6f)); return; }
    if (cmd == "rex_run")          { Leg::setGait    (getOrF(doc,"factor", 1.5f), 0.6f, 0.4f, "run"); return; }
    if (cmd == "rex_stop")         { Leg::stop(); return; }

    if (cmd == "rex_gait") {
      const char* mode = getOrCStr(doc, "mode", "walk");   // proper ArduinoJson fallback
      Leg::setGait(getOrF(doc,"speed",  0.7f),
                   getOrF(doc,"stride", 0.6f),
                   getOrF(doc,"lift",   0.4f),
                   String(mode));
      return;
    }

    if (cmd == "rex_speed_adjust") { Leg::adjustSpeed(getOrF(doc, "delta",  0.1f)); return; }
    if (cmd == "rex_stride_set")   { Leg::setStride  (getOrF(doc, "value",  0.6f)); return; }
    if (cmd == "rex_posture")      { Leg::setPosture (getOrF(doc, "level",  0.5f)); return; }

    // ---- Body (spine / tail / head / pelvis) ----
    if (cmd == "rex_spine_up")     { Spine::up(); return; }
    if (cmd == "rex_spine_down")   { Spine::down(); return; }
    if (cmd == "rex_spine_set")    { Spine::set(getOrF(doc, "level", 0.5f)); return; }

    if (cmd == "rex_tail_wag")     { Tail::wag(); return; }
    if (cmd == "rex_tail_set")     { Tail::set(getOrF(doc, "level", 0.5f)); return; }

    if (cmd == "rex_roar")         { Head::roar(); return; }
    if (cmd == "rex_mouth_open")   { Head::mouthOpen(); return; }
    if (cmd == "rex_mouth_close")  { Head::mouthClose(); return; }

    if (cmd == "rex_pelvis_set")   { Pelvis::set(getOrF(doc, "level", 0.5f)); return; }

    // ---- IMU / stabilization (optional; requires IMU.h) ----
    if (cmd == "rex_stab_enable")  { IMU::enable(true);  return; }
    if (cmd == "rex_stab_disable") { IMU::enable(false); return; }

    if (cmd == "rex_stab_cal") {
      // If caller passes explicit zeros, they’ll overwrite offsets—which is fine for manual calibration
      IMU::setOffsets(getOrF(doc,"roll0",  0.0f),
                      getOrF(doc,"pitch0", 0.0f),
                      getOrF(doc,"yaw0",   0.0f));
      return;
    }

    if (cmd == "rex_stab_gains") {
      ImuGains g = IMU::getGains();
      if (doc.containsKey("k_roll"))   g.k_roll  = doc["k_roll"].as<float>();
      if (doc.containsKey("b_roll"))   g.b_roll  = doc["b_roll"].as<float>();
      if (doc.containsKey("k_pitch"))  g.k_pitch = doc["k_pitch"].as<float>();
      if (doc.containsKey("b_pitch"))  g.b_pitch = doc["b_pitch"].as<float>();
      IMU::setGains(g);
      return;
    }

    // Unknown JSON command
    #ifdef SERIAL_DEBUG
    Serial.print("Unknown JSON cmd: "); Serial.println(cmd);
    #endif
    return;
  }

  // -------- Plain string fallback --------
  if (s == "rex_roar")         { Head::roar(); return; }
  if (s == "rex_tail_wag")     { Tail::wag(); return; }
  if (s == "rex_spine_up")     { Spine::up(); return; }
  if (s == "rex_spine_down")   { Spine::down(); return; }
  if (s == "rex_walk_forward") { Leg::walkForward(0.7f); return; }
  if (s == "rex_turn_left")    { Leg::turnLeft(0.6f); return; }
  if (s == "rex_turn_right")   { Leg::turnRight(0.6f); return; }

  #ifdef SERIAL_DEBUG
  Serial.print("Unknown raw cmd: "); Serial.println(s);
  #endif
}

} // namespace CommandRouter