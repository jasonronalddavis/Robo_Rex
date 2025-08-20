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
static volatile bool s_task_running = false;

static TaskHandle_t s_task = nullptr;
static SemaphoreHandle_t s_mutex = nullptr;

// -------------------- Helper functions --------------------
static inline float wrap180(float d) {
  while (d > 180.0f) d -= 360.0f;
  while (d < -180.0f) d += 360.0f;
  return d;
}

// Test basic I2C communication
static bool testI2CConnection() {
  Wire.beginTransmission(0x68);  // MPU6050 default address
  byte error = Wire.endTransmission();
  
#ifdef IMU_DEBUG
  if (error == 0) {
    Serial.println(F("[IMU] I2C: MPU6050 ACK received"));
  } else {
    Serial.printf("[IMU] I2C: MPU6050 NACK (error %d)\n", error);
  }
#endif
  
  return (error == 0);
}

// -------------------- Main IMU task --------------------
static void imuTask(void*) {
  const uint32_t target_hz = 200;
  const TickType_t delayTicks = pdMS_TO_TICKS(1000 / target_hz);
  uint32_t consecutiveErrors = 0;
  const uint32_t maxConsecutiveErrors = 10;

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] Task started"));
#endif

  s_task_running = true;

  for (;;) {
    // Check if task should continue
    if (!s_task_running) {
#ifdef IMU_DEBUG
      Serial.println(F("[IMU] Task stopping"));
#endif
      break;
    }

    // Read sensor with error handling
    sensors_event_t a, g, temp;
    bool readSuccess = s_mpu.getEvent(&a, &g, &temp);
    
    if (!readSuccess) {
      consecutiveErrors++;
      
#ifdef IMU_DEBUG
      if (consecutiveErrors <= 3) {  // Don't spam logs
        Serial.printf("[IMU] getEvent() failed (consecutive: %d)\n", consecutiveErrors);
      }
#endif

      // Update error state
      if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
        s_state.healthy = false;
        s_state.error_count++;
        xSemaphoreGive(s_mutex);
      }

      // If too many consecutive errors, try to reinitialize
      if (consecutiveErrors >= maxConsecutiveErrors) {
#ifdef IMU_DEBUG
        Serial.println(F("[IMU] Too many errors, attempting reinit..."));
#endif
        
        // Try to reinitialize the sensor
        if (s_mpu.begin()) {
          s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
          s_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
          s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
          consecutiveErrors = 0;
#ifdef IMU_DEBUG
          Serial.println(F("[IMU] Reinit successful"));
#endif
        } else {
#ifdef IMU_DEBUG
          Serial.println(F("[IMU] Reinit failed"));
#endif
        }
      }

      vTaskDelay(delayTicks);
      continue;
    }

    // Successful read - reset error counter
    consecutiveErrors = 0;

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

    // Publish with timeout protection
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      s_state.roll_deg  = roll;
      s_state.pitch_deg = pitch;
      s_state.yaw_deg   = yaw;
      s_state.sample_us = micros();
      s_state.healthy   = true;
      s_state.success_count++;
      xSemaphoreGive(s_mutex);
    }

    vTaskDelay(delayTicks);
  }

  s_task_running = false;
  s_task = nullptr;
  vTaskDelete(NULL);
}

// -------------------- Public API --------------------
namespace IMU {

bool begin() {
#ifdef IMU_DEBUG
  Serial.println(F("[IMU] begin() - using existing I2C bus"));
#endif

  // Test I2C communication first
  if (!testI2CConnection()) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] I2C test failed - device not responding"));
#endif
    return false;
  }

  // Try to initialize the sensor
  if (!s_mpu.begin()) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] MPU6050 begin() failed"));
#endif
    return false;
  }

  // Configure sensor ranges and filters
  s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  s_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] MPU6050 configured successfully"));
#endif

  // Initialize filter with nominal update rate
  s_filter.begin(200.0f);

  // Create mutex if it doesn't exist
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
#ifdef IMU_DEBUG
      Serial.println(F("[IMU] Failed to create mutex"));
#endif
      return false;
    }
  }

  // Initialize state
  s_state.healthy = false;
  s_state.error_count = 0;
  s_state.success_count = 0;

  // Default gains (kept simple)
  s_gains.k_roll  = 0.010f; s_gains.b_roll  = 0.50f;
  s_gains.k_pitch = 0.010f; s_gains.b_pitch = 0.50f;

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] Initialization complete"));
#endif
  return true;
}

void startTask(uint32_t hz) {
  (void)hz; // current task uses fixed 200 Hz timing
  
  if (s_task) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Task already running"));
#endif
    return;
  }

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] Starting background task..."));
#endif

  s_task_running = false;  // Will be set to true in task
  BaseType_t result = xTaskCreatePinnedToCore(
    imuTask, 
    "imuTask", 
    4096, 
    nullptr, 
    1, 
    &s_task, 
    0  // Core 0
  );

  if (result != pdPASS) {
#ifdef IMU_DEBUG
    Serial.printf("[IMU] Failed to create task (error %d)\n", result);
#endif
    s_task = nullptr;
  } else {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Task created successfully"));
#endif
  }
}

void stopTask() {
  if (s_task) {
    s_task_running = false;
    // Wait a bit for task to clean up
    vTaskDelay(pdMS_TO_TICKS(100));
    s_task = nullptr;
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Task stopped"));
#endif
  }
}

void get(ImuState& out) {
  if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    out = s_state;
    xSemaphoreGive(s_mutex);
  } else {
    // Timeout - return last known state
    out = s_state;
#ifdef IMU_DEBUG
    static uint32_t lastTimeoutWarning = 0;
    uint32_t now = millis();
    if (now - lastTimeoutWarning > 5000) {  // Warn every 5 seconds max
      Serial.println(F("[IMU] Mutex timeout in get()"));
      lastTimeoutWarning = now;
    }
#endif
  }
}

bool isHealthy() {
  ImuState state;
  get(state);
  return state.healthy && (millis() - (state.sample_us / 1000) < 1000);  // Updated within 1 second
}

void printStatus() {
  ImuState state;
  get(state);
  
  Serial.println(F("[IMU] Status:"));
  Serial.printf("  Healthy: %s\n", state.healthy ? "YES" : "NO");
  Serial.printf("  Success count: %u\n", state.success_count);
  Serial.printf("  Error count: %u\n", state.error_count);
  Serial.printf("  Roll: %.2f°, Pitch: %.2f°, Yaw: %.2f°\n", 
                state.roll_deg, state.pitch_deg, state.yaw_deg);
  Serial.printf("  Last sample: %u μs ago\n", micros() - state.sample_us);
  Serial.printf("  Task running: %s\n", s_task_running ? "YES" : "NO");
}

bool testConnection() {
  return testI2CConnection();
}

void setOffsets(float roll0_deg, float pitch0_deg, float yaw0_deg) {
  s_off_roll  = roll0_deg;
  s_off_pitch = pitch0_deg;
  s_off_yaw   = yaw0_deg;
#ifdef IMU_DEBUG
  Serial.printf("[IMU] Offsets set: roll=%.2f°, pitch=%.2f°, yaw=%.2f°\n",
                s_off_roll, s_off_pitch, s_off_yaw);
#endif
}

void setGains(const ImuGains& g) { 
  s_gains = g; 
#ifdef IMU_DEBUG
  Serial.printf("[IMU] Gains updated: k_roll=%.4f, k_pitch=%.4f\n", 
                g.k_roll, g.k_pitch);
#endif
}

ImuGains getGains() { 
  return s_gains; 
}

void enable(bool on) { 
  s_enabled = on; 
#ifdef IMU_DEBUG
  Serial.printf("[IMU] %s\n", on ? "Enabled" : "Disabled");
#endif
}

bool isEnabled() { 
  return s_enabled; 
}

} // namespace IMU