#include "IMU.h"
#include <Wire.h>
#include <MadgwickAHRS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------- Sensor selection --------------------
// This implementation targets the MPU6050 (GY‑521).
static Adafruit_MPU6050 s_mpu;

// -------------------- Filter & shared state ----------------
static Madgwick s_filter;
static ImuState s_state;
static ImuGains s_gains;

static float s_off_roll  = 0.0f;
static float s_off_pitch = 0.0f;
static float s_off_yaw   = 0.0f;

static volatile bool s_enabled = true;

static TaskHandle_t s_task = nullptr;
static SemaphoreHandle_t s_mutex = nullptr;

static inline float wrap180(float d) {
  while (d > 180.0f) d -= 360.0f;
  while (d < -180.0f) d += 360.0f;
  return d;
}

static void imuTask(void*) {
  const uint32_t target_hz = 200;
  const TickType_t delayTicks = pdMS_TO_TICKS(1000 / target_hz);
  uint32_t lastUs = micros();

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] Task started"));
#endif

  for (;;) {
    // Read sensor
    sensors_event_t a, g, temp;
    if (!s_mpu.getEvent(&a, &g, &temp)) {
#ifdef IMU_DEBUG
      Serial.println(F("[IMU] getEvent() failed"));
#endif
      vTaskDelay(delayTicks);
      continue;
    }

    // Inputs for Madgwick:
    //  - Gyro in rad/s
    //  - Accel in m/s^2
    const float gx = g.gyro.x;
    const float gy = g.gyro.y;
    const float gz = g.gyro.z;

    const float ax = a.acceleration.x;
    const float ay = a.acceleration.y;
    const float az = a.acceleration.z;

    // No magnetometer -> pass zeros
    const float mx = 0.0f, my = 0.0f, mz = 0.0f;

    // Update filter
    s_filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

    // Euler angles (deg)
    float roll  = s_filter.getRoll();
    float pitch = s_filter.getPitch();
    float yaw   = s_filter.getYaw();   // relative drift without mag

    // Apply user offsets
    roll  = wrap180(roll  - s_off_roll);
    pitch = wrap180(pitch - s_off_pitch);
    yaw   = wrap180(yaw   - s_off_yaw);

    // Publish
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      s_state.roll_deg  = roll;
      s_state.pitch_deg = pitch;
      s_state.yaw_deg   = yaw;
      s_state.sample_us = micros();
      s_state.healthy   = true;
      xSemaphoreGive(s_mutex);
    }

    vTaskDelay(delayTicks);
  }
}

// -------------------- Public API --------------------
namespace IMU {

bool begin() {
  #ifdef IMU_DEBUG
  Serial.println(F("[IMU] begin() using existing I2C bus"));
#endif
  if (!s_mpu.begin()) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] MPU6050 not found (check wiring, power, address)"));
#endif
    return false;
  }

  // Basic ranges & bandwidth
  s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  s_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] MPU6050 OK"));
#endif

  // Init filter (nominal update rate)
  s_filter.begin(200.0f);

  // Sync objects
  if (!s_mutex) s_mutex = xSemaphoreCreateMutex();

  // Default gains (kept simple)
  s_gains.k_roll  = 0.010f; s_gains.b_roll  = 0.50f;
  s_gains.k_pitch = 0.010f; s_gains.b_pitch = 0.50f;

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] init complete"));
#endif
  return true;
}

void startTask(uint32_t hz) {
  (void)hz; // current task uses fixed 200 Hz timing
  if (s_task) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] task already running"));
#endif
    return;
  }
#ifdef IMU_DEBUG
  Serial.println(F("[IMU] starting task..."));
#endif
  xTaskCreatePinnedToCore(imuTask, "imuTask", 4096, nullptr, 1, &s_task, 0);
}

void get(ImuState& out) {
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    out = s_state;
    xSemaphoreGive(s_mutex);
  } else {
    out = s_state; // fall back to last snapshot
  }
}

void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg) {
  s_off_roll  = roll0_deg;
  s_off_pitch = pitch0_deg;
  s_off_yaw   = yaw0_deg;
#ifdef IMU_DEBUG
  Serial.printf("[IMU] Offsets set  roll=%.2f  pitch=%.2f  yaw=%.2f\n",
                s_off_roll, s_off_pitch, s_off_yaw);
#endif
}

void setGains(const ImuGains& g) { s_gains = g; }
ImuGains getGains() { return s_gains; }

void enable(bool on) { s_enabled = on; }
bool isEnabled() { return s_enabled; }

} // namespace IMU
