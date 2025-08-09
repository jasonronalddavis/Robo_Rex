#include "ServoBus.h"
#include "BLEServerHandler.h"
#include "CommandRouter.h"

// Modules
#include "Servo_Functions/Leg_Function.h"
#include "Servo_Functions/Spine_Function.h"
#include "Servo_Functions/Head_Function.h"
#include "Servo_Functions/Neck_Function.h"
#include "Servo_Functions/Tail_Function.h"
#include "Servo_Functions/Pelvis_Function.h"


ServoBus SERVO;
// === EDIT THESE TO MATCH YOUR WIRING ===
Leg::Map legMap{};
Spine::Map spineMap{ .spinePitch = 10 };
Head::Map headMap{ .jaw = 11, .neckPitch = 12 };
Neck::Map neckMap{ .yaw = 13, .pitch = 12 };
Tail::Map tailMap{ .wag = 14 };
Pelvis::Map pelvisMap{ .roll = 15 };

void setup() {
  Serial.begin(115200);
  delay(100);

  // Servo bus
  SERVO.begin(0x40, 50.0f);

  // Init modules
  Leg::begin(&SERVO, legMap);
  Spine::begin(&SERVO, spineMap);
  Head::begin(&SERVO, headMap);
  Neck::begin(&SERVO, neckMap);
  Tail::begin(&SERVO, tailMap);
  Pelvis::begin(&SERVO, pelvisMap);

  // BLE
  BLEServerHandler::begin("Robo_Rex_ESP32S3");
  Serial.println("BLE ready. Awaiting commands…");
}

void loop() {
  // (Optional) periodic stabilization hooks later (IMU-based)
  delay(20);
}
