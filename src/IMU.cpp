#include "IMU.h"
#include <Wire.h>
#include <MadgwickAHRS.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------- Sensor selection --------------------
// This implementation targets the MPU6050 (GY‑521) on Wire1 bus.
static Adafruit_MPU6050 s_mpu;
static uint8_t s_mpu_address = 0x68;  // Track which address we're using

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

// Test basic I2C communication on Wire1
static bool testI2CConnection(uint8_t addr = 0x68) {
  Wire1.beginTransmission(addr);
  byte error = Wire1.endTransmission();
  
#ifdef IMU_DEBUG
  if (error == 0) {
    Serial.printf("[IMU] Wire1: Device 0x%02X ACK received\n", addr);
  } else {
    Serial.printf("[IMU] Wire1: Device 0x%02X NACK (error %d)\n", addr, error);
  }
#endif
  
  return (error == 0);
}

// Read a single register from MPU6050
static bool readRegister(uint8_t reg, uint8_t &value, uint8_t addr = 0x68) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  if (Wire1.endTransmission() != 0) {
    return false;
  }
  
  Wire1.requestFrom(addr, (uint8_t)1);
  if (Wire1.available()) {
    value = Wire1.read();
    return true;
  }
  return false;
}

// Write a single register to MPU6050
static bool writeRegister(uint8_t reg, uint8_t value, uint8_t addr = 0x68) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  Wire1.write(value);
  return (Wire1.endTransmission() == 0);
}

// Wake up MPU6050 from sleep mode
static bool wakeUpMPU6050(uint8_t addr = 0x68) {
#ifdef IMU_DEBUG
  Serial.printf("[IMU] Attempting to wake up MPU6050 at 0x%02X\n", addr);
#endif

  // Read current power management register
  uint8_t pwr_mgmt;
  if (!readRegister(0x6B, pwr_mgmt, addr)) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Failed to read PWR_MGMT_1 register"));
#endif
    return false;
  }

#ifdef IMU_DEBUG
  Serial.printf("[IMU] PWR_MGMT_1: 0x%02X (0x40=sleep, 0x00=awake)\n", pwr_mgmt);
#endif

  // If device is in sleep mode, wake it up
  if (pwr_mgmt & 0x40) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Device is in sleep mode, waking up..."));
#endif
    
    if (!writeRegister(0x6B, 0x00, addr)) {
#ifdef IMU_DEBUG
      Serial.println(F("[IMU] Failed to write wake-up command"));
#endif
      return false;
    }
    
    delay(100);  // Give device time to wake up
    
    // Verify wake-up
    if (!readRegister(0x6B, pwr_mgmt, addr)) {
#ifdef IMU_DEBUG
      Serial.println(F("[IMU] Failed to verify wake-up"));
#endif
      return false;
    }
    
#ifdef IMU_DEBUG
    Serial.printf("[IMU] PWR_MGMT_1 after wake: 0x%02X\n", pwr_mgmt);
#endif
  } else {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] Device is already awake"));
#endif
  }
  
  return true;
}

// Detect MPU6050 address and wake it up
static uint8_t detectAndWakeupMPU6050() {
  uint8_t addresses[] = {0x68, 0x69};
  
  for (uint8_t addr : addresses) {
#ifdef IMU_DEBUG
    Serial.printf("[IMU] Trying address 0x%02X\n", addr);
#endif
    
    if (testI2CConnection(addr)) {
      // Check WHO_AM_I register
      uint8_t whoami;
      if (readRegister(0x75, whoami, addr)) {
#ifdef IMU_DEBUG
        Serial.printf("[IMU] WHO_AM_I at 0x%02X: 0x%02X\n", addr, whoami);
#endif
        
        if (whoami == 0x68 || whoami == 0x98) {  // MPU6050 (0x68) or MPU9250 (0x98)
          if (wakeUpMPU6050(addr)) {
#ifdef IMU_DEBUG
            const char* chipName = (whoami == 0x68) ? "MPU6050" : "MPU9250";
            Serial.printf("[IMU] Successfully detected and woke %s at 0x%02X\n", chipName, addr);
#endif
            return addr;
          }
        }
      }
    }
  }
  
#ifdef IMU_DEBUG
  Serial.println(F("[IMU] No valid MPU6050 found at any address"));
#endif
  return 0x00;  // No device found
}

// -------------------- Main IMU task --------------------
static void imuTask(void*) {
  const uint32_t target_hz = 200;
  const TickType_t delayTicks = pdMS_TO_TICKS(1000 / target_hz);
  uint32_t consecutiveErrors = 0;
  const uint32_t maxConsecutiveErrors = 10;

#ifdef IMU_DEBUG
  Serial.println(F("[IMU] Task started on Wire1"));
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
        
        // Try to wake up device first
        if (wakeUpMPU6050(s_mpu_address)) {
          // Try to reinitialize the sensor on Wire1
          if (s_mpu.begin(s_mpu_address, &Wire1)) {
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
        } else {
#ifdef IMU_DEBUG
          Serial.println(F("[IMU] Wake-up failed during reinit"));
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
  Serial.println(F("[IMU] begin() - initializing Wire1 for dedicated IMU bus"));
#endif

  // Initialize Wire1 for IMU on pins 19/20
  Wire1.begin(IMU_SDA_PIN, IMU_SCL_PIN);
  Wire1.setClock(400000);  // 400kHz for fast IMU updates
  delay(100);  // Allow bus to settle

#ifdef IMU_DEBUG
  Serial.printf("[IMU] Wire1 initialized: SDA=%d, SCL=%d, 400kHz\n", IMU_SDA_PIN, IMU_SCL_PIN);
#endif

  // Detect and wake up MPU6050
  s_mpu_address = detectAndWakeupMPU6050();
  if (s_mpu_address == 0x00) {
#ifdef IMU_DEBUG
    Serial.println(F("[IMU] No MPU6050 detected"));
#endif
    return false;
  }

  // Try to initialize the sensor on Wire1 with detected address
  if (!s_mpu.begin(s_mpu_address, &Wire1)) {
#ifdef IMU_DEBUG
    Serial.printf("[IMU] MPU6050 begin() failed at address 0x%02X\n", s_mpu_address);
#endif
    return false;
  }

  // Configure sensor ranges and filters
  s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  s_mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  s_mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

#ifdef IMU_DEBUG
  Serial.printf("[IMU] MPU6050 configured successfully at 0x%02X\n", s_mpu_address);
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
  Serial.println(F("[IMU] Initialization complete on dedicated Wire1 bus"));
#endif
  return true;
}

bool beginWithAddressDetection() {
  return begin();  // The new begin() already includes address detection
}

bool wakeUpDevice() {
  return wakeUpMPU6050(s_mpu_address);
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
  
  Serial.println(F("[IMU] Status (Wire1):"));
  Serial.printf("  Address: 0x%02X\n", s_mpu_address);
  Serial.printf("  Healthy: %s\n", state.healthy ? "YES" : "NO");
  Serial.printf("  Success count: %u\n", state.success_count);
  Serial.printf("  Error count: %u\n", state.error_count);
  Serial.printf("  Roll: %.2f°, Pitch: %.2f°, Yaw: %.2f°\n", 
                state.roll_deg, state.pitch_deg, state.yaw_deg);
  Serial.printf("  Last sample: %u μs ago\n", micros() - state.sample_us);
  Serial.printf("  Task running: %s\n", s_task_running ? "YES" : "NO");
}

bool testConnection() {
  return testI2CConnection(s_mpu_address);
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