#include "CommandRouter.h"
#include <ArduinoJson.h>

// Motion modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"

namespace CommandRouter {

// ---------------- Internal state ----------------
static float g_pelvisLevel = 0.50f; // 0..1
static float g_spineLevel  = 0.50f; // 0..1
static float g_headPitch   = 0.50f; // 0..1  (up/down)
static float g_neckYaw     = 0.50f; // 0..1  (left/right)
static float g_tailYaw     = 0.50f; // 0..1  (left/right)

static const float NUDGE_FINE = 0.02f;

static inline float clamp01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }

// If your Head/Neck/Tail use different API names, edit in one place here:
static inline void applyHeadPitch(float level01) {
  (void)level01;
  // Example:
  // Head::setPitch(level01);
}

static inline void applyNeckYaw(float level01) {
  (void)level01;
  // Example:
  // Neck::setYaw(level01);
}

static inline void applyTailYaw(float level01) {
  (void)level01;
  // Example:
  // Tail::setYaw(level01);
}

static void applyAnalogs() {
  Pelvis::stabilize(g_pelvisLevel);
  Spine::set(g_spineLevel);
  applyHeadPitch(g_headPitch);
  applyNeckYaw(g_neckYaw);
  applyTailYaw(g_tailYaw);
}

void begin() {
  g_pelvisLevel = 0.50f;
  g_spineLevel  = 0.50f;
  g_headPitch   = 0.50f;
  g_neckYaw     = 0.50f;
  g_tailYaw     = 0.50f;
  applyAnalogs();
}

// ---------------- Helpers for new JSON schema ----------------
static void handleLegs(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "move_forward")       { Leg::walkForward(0.8f); }
    else if (command == "move_backward") { Leg::walkBackward(0.8f); }
    else if (command == "move_left")     { Leg::turnLeft(0.8f); }
    else if (command == "move_right")    { Leg::turnRight(0.8f); }
  } else if (phase == "stop") {
    Leg::stop();
  }
}

static void handlePelvis(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "pelvis_up")   g_pelvisLevel = clamp01(g_pelvisLevel + NUDGE_FINE);
    if (command == "pelvis_down") g_pelvisLevel = clamp01(g_pelvisLevel - NUDGE_FINE);
    Pelvis::stabilize(g_pelvisLevel);
  } else if (phase == "stop") {
    // Keep last position (or uncomment to re-center):
    // g_pelvisLevel = 0.5f; Pelvis::stabilize(g_pelvisLevel);
  }
}

static void handleSpine(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "spine_up")   g_spineLevel = clamp01(g_spineLevel + NUDGE_FINE);
    if (command == "spine_down") g_spineLevel = clamp01(g_spineLevel - NUDGE_FINE);
    Spine::set(g_spineLevel);
  } else if (phase == "stop") {
    // Keep last position (or re-center if you prefer)
  }
}

static void handleHead(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "head_up")   g_headPitch = clamp01(g_headPitch + NUDGE_FINE);
    if (command == "head_down") g_headPitch = clamp01(g_headPitch - NUDGE_FINE);
    applyHeadPitch(g_headPitch);
  }
}

static void handleNeck(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "neck_left")  g_neckYaw = clamp01(g_neckYaw - NUDGE_FINE);
    if (command == "neck_right") g_neckYaw = clamp01(g_neckYaw + NUDGE_FINE);
    applyNeckYaw(g_neckYaw);
  }
}

static void handleTail(const String& command, const String& phase) {
  if (phase == "start" || phase == "hold") {
    if (command == "tail_left")  g_tailYaw = clamp01(g_tailYaw - NUDGE_FINE);
    if (command == "tail_right") g_tailYaw = clamp01(g_tailYaw + NUDGE_FINE);
    applyTailYaw(g_tailYaw);
  }
}

// Fallback for full-body directional (if you choose to use it)
static void handleFullBody(const String& command, const String& phase) {
  if (phase == "stop") {
    Leg::stop();
    return;
  }
  if (phase == "start" || phase == "hold") {
    if (command == "up")        { Leg::walkForward(0.7f); }
    else if (command == "down") { Leg::walkBackward(0.7f); }
    else if (command == "left") { Leg::turnLeft(0.7f); }
    else if (command == "right"){ Leg::turnRight(0.7f); }
  }
}

// ---------------- Legacy command support ----------------
static void handleLegacyJson(const JsonDocument& doc) {
  const String cmd = doc["cmd"] | "";

  if (cmd == "rex_walk_forward") { Leg::walkForward(0.7f); return; }
  if (cmd == "rex_walk_backward"){ Leg::walkBackward(0.7f); return; }
  if (cmd == "rex_turn_left")    { Leg::turnLeft(0.6f);    return; }
  if (cmd == "rex_turn_right")   { Leg::turnRight(0.6f);   return; }
  if (cmd == "rex_stop")         { Leg::stop();            return; }

  if (cmd == "rex_spine_up")     { g_spineLevel  = clamp01(g_spineLevel + NUDGE_FINE);  Spine::set(g_spineLevel);  return; }
  if (cmd == "rex_spine_down")   { g_spineLevel  = clamp01(g_spineLevel - NUDGE_FINE);  Spine::set(g_spineLevel);  return; }
  if (cmd == "rex_tail_wag")     { /* optional legacy tail wag */ return; }

  if (cmd == "rex_gait") {
    const float speed  = doc["speed"]  | 0.7f;
    const float stride = doc["stride"] | 0.6f;
    const float lift   = doc["lift"]   | 0.4f;
    const String mode  = doc["mode"]   | String("walk");
    Leg::setGait(speed, stride, lift, mode);
    return;
  }

  if (cmd == "rex_speed_adjust") { Leg::adjustSpeed(doc["delta"] | 0.1f); return; }
  if (cmd == "rex_stride_set")   { Leg::setStride  (doc["value"] | 0.6f); return; }
  if (cmd == "rex_posture")      { Leg::setPosture (doc["level"] | 0.5f); return; }

  // Unknown legacy command
  Serial.print(F("Unknown legacy JSON cmd: "));
  Serial.println(cmd);
}

static void handleLegacyRaw(const String& s) {
  if (s == "rex_walk_forward") { Leg::walkForward(0.7f); return; }
  if (s == "rex_walk_backward"){ Leg::walkBackward(0.7f); return; }
  if (s == "rex_turn_left")    { Leg::turnLeft(0.6f); return; }
  if (s == "rex_turn_right")   { Leg::turnRight(0.6f); return; }
  if (s == "rex_stop")         { Leg::stop(); return; }

  if (s == "rex_spine_up")     { g_spineLevel = clamp01(g_spineLevel + NUDGE_FINE); Spine::set(g_spineLevel); return; }
  if (s == "rex_spine_down")   { g_spineLevel = clamp01(g_spineLevel - NUDGE_FINE); Spine::set(g_spineLevel); return; }

  Serial.print(F("Unknown raw cmd: "));
  Serial.println(s);
}

// ---------------- Public API ----------------
void handleLine(const String& line) {
  // Try new JSON contract first
  StaticJsonDocument<384> doc;
  DeserializationError err = deserializeJson(doc, line);

  if (!err) {
    const String target  = doc["target"]  | "";
    const String part    = doc["part"]    | "";
    const String command = doc["command"] | "";
    const String phase   = doc["phase"]   | "";

    if (target.length() && command.length() && phase.length()) {
      // Route primarily by part (more specific than target)
      if      (part == "legs")   handleLegs(command, phase);
      else if (part == "pelvis") handlePelvis(command, phase);
      else if (part == "spine")  handleSpine(command, phase);
      else if (part == "head")   handleHead(command, phase);
      else if (part == "neck")   handleNeck(command, phase);
      else if (part == "tail")   handleTail(command, phase);
      else {
        // Fallback: fullBody panel or generic directions
        if (target == "fullBody") handleFullBody(command, phase);
      }

      // Optional echo for debugging over Serial:
      Serial.print(F("RX OK: "));
      serializeJson(doc, Serial);
      Serial.println();
      return;
    }

    // If it wasn't the new schema, try legacy JSON ("cmd": "rex_*")
    if (doc.containsKey("cmd")) {
      handleLegacyJson(doc);
      return;
    }

    // Unknown JSON shape
    Serial.print(F("Unknown JSON: "));
    serializeJson(doc, Serial);
    Serial.println();
    return;
  }

  // Not JSON → legacy raw string path
  String s = line;
  s.trim();
  if (s.length()) {
    handleLegacyRaw(s);
  }
}

void tick() {
  // no-op for now (placeholder for smoothing if needed)
}

} // namespace CommandRouter