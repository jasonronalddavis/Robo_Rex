#include "IMU.h"
#include <Wire.h>
#include <MadgwickAHRS.h>

#ifdef IMU_SENSOR_ICM20948
  #include <ICM_20948.h>       // SparkFun
  static ICM_20948_I2C icm;
#endif

#ifdef IMU_SENSOR_MPU6050
  #include <Adafruit_MPU6050.h>
  #include <Adafruit_Sensor.h>
  static Adafruit_MPU6050 mpu;
#endif

// ---------- State ----------
static Madgwick filter;
static ImuState g_state;
static ImuGains g_gains;
static float off_roll=0, off_pitch=0, off_yaw=0;
static volatile bool g_enabled = true;

static TaskHandle_t s_task = nullptr;
static SemaphoreHandle_t s_mutex;

static inline float rad2deg(float r){ return r * 57.2957795f; }
static inline float wrap180(float d) {
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

static void imuTask(void* param) {
  // Desired loop rate passed as parameter (default 200 Hz)
  uint32_t target_hz = (uint32_t)(uintptr_t)param;
  if (target_hz == 0) target_hz = 200;
  const TickType_t delayTicks = pdMS_TO_TICKS(1000 / target_hz);
  uint32_t lastUs = micros();

  for (;;) {
    // Read raw
    float ax=0, ay=0, az=0, gx=0, gy=0, gz=0, mx=0, my=0, mz=0;

#ifdef IMU_SENSOR_ICM20948
    if (icm.dataReady()) icm.getAGMT(); // updates internal buffers if ready
    ax = icm.accX(); ay = icm.accY(); az = icm.accZ();
    gx = icm.gyrX()*DEG_TO_RAD; gy = icm.gyrY()*DEG_TO_RAD; gz = icm.gyrZ()*DEG_TO_RAD; // rad/s
    // Magnetometer optional: mx = icm.magX(); my = icm.magY(); mz = icm.magZ();
#endif

#ifdef IMU_SENSOR_MPU6050
    sensors_event_t a, g, temp;
    if (!mpu.getEvent(&a, &g, &temp)) {
      vTaskDelay(delayTicks);
      continue;
    }
    ax = a.acceleration.x; ay = a.acceleration.y; az = a.acceleration.z;
    gx = g.gyro.x; gy = g.gyro.y; gz = g.gyro.z; // Adafruit units are rad/s
#endif

    // dt
    const uint32_t nowUs = micros();
    const float dt = (nowUs - lastUs) / 1e6f;
    lastUs = nowUs;
    if (dt <= 0) { vTaskDelay(delayTicks); continue; }

    // Update filter (gyros rad/s, acc m/s^2)
    filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

    // Euler (deg)
    float roll  = filter.getRoll();
    float pitch = filter.getPitch();
    float yaw   = filter.getYaw();

    // Offsets / mounting correction
    roll  = wrap180(roll  - off_roll);
    pitch = wrap180(pitch - off_pitch);
    yaw   = wrap180(yaw   - off_yaw);

    // Publish
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      g_state.roll_deg  = roll;
      g_state.pitch_deg = pitch;
      g_state.yaw_deg   = yaw;
      g_state.sample_us = nowUs;
      g_state.healthy   = true;
      xSemaphoreGive(s_mutex);
    }

    vTaskDelay(delayTicks);
  }
}

namespace IMU {

bool begin() {
  Wire.begin();
  Wire.setClock(400000);

#ifdef IMU_SENSOR_ICM20948
  if (icm.begin(Wire, 0) != ICM_20948_Stat_Ok) {
    return false;
  }
  // Default sensor configs are fine for stabilization.
#endif

#ifdef IMU_SENSOR_MPU6050
  if (!mpu.begin()) {
    return false;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
#endif

  // Nominal update rate (we'll still compute dt per loop)
  filter.begin(200);

  s_mutex = xSemaphoreCreateMutex();
  return true;
}

void startTask(uint32_t hz) {
  if (s_task) return;
  if (hz == 0) hz = 200;
  xTaskCreatePinnedToCore(
    imuTask,
    "imuTask",
    4096,
    (void*)(uintptr_t)hz, // pass desired Hz
    1,
    &s_task,
    0
  );
}

void get(ImuState& out) {
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    out = g_state;
    xSemaphoreGive(s_mutex);
  } else {
    out = g_state;
  }
}

void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg) {
  off_roll  = roll0_deg;
  off_pitch = pitch0_deg;
  off_yaw   = yaw0_deg;
}

void setGains(const ImuGains& g) { g_gains = g; }
ImuGains getGains() { return g_gains; }

void enable(bool on) { g_enabled = on; }
bool isEnabled() { return g_enabled; }

} // namespace IMU
