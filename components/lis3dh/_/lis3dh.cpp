#include "lis3dh.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lis3dh {

static const char *const TAG = "lis3dh";

// Acceleration of gravity (m/s²)
static constexpr float GRAVITY = 9.80665f;

// ─── setup ────────────────────────────────────────────────────────────────────

void LIS3DHComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LIS3DH...");

  // Verify device identity
  uint8_t who_am_i = 0;
  if (!this->read_register(LIS3DH_REG_WHO_AM_I, &who_am_i, 1)) {
    ESP_LOGE(TAG, "Failed to read WHO_AM_I register");
    this->mark_failed();
    return;
  }
  if (who_am_i != LIS3DH_WHO_AM_I_VALUE) {
    ESP_LOGE(TAG, "Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", who_am_i, LIS3DH_WHO_AM_I_VALUE);
    this->mark_failed();
    return;
  }

  // CTRL_REG1: ODR | LPen | Zen | Yen | Xen
  uint8_t ctrl_reg1 = static_cast<uint8_t>(data_rate_)
                    | (low_power_ ? 0x08 : 0x00)
                    | 0x07;  // enable X, Y, Z
  if (!this->write_register(LIS3DH_REG_CTRL_REG1, &ctrl_reg1, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG1");
    this->mark_failed();
    return;
  }

  // CTRL_REG4: BDU | FS[1:0] | HR
  //   BDU = 1  → block data update until MSB/LSB read
  //   HR  = 1  → high-resolution mode (12-bit), ignored in LP mode
  uint8_t ctrl_reg4 = 0x80                             // BDU
                    | static_cast<uint8_t>(range_)     // FS
                    | (low_power_ ? 0x00 : 0x08);      // HR
  if (!this->write_register(LIS3DH_REG_CTRL_REG4, &ctrl_reg4, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG4");
    this->mark_failed();
    return;
  }

  initialized_ = true;
  ESP_LOGD(TAG, "LIS3DH initialized successfully");
}

// ─── update ───────────────────────────────────────────────────────────────────

void LIS3DHComponent::update() {
  if (!initialized_) return;

  int16_t raw_x, raw_y, raw_z;
  if (!this->read_accel_(raw_x, raw_y, raw_z)) {
    ESP_LOGW(TAG, "Failed to read acceleration data");
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();

  // In HR mode (12-bit) data is left-aligned → shift right 4
  // In LP mode  (8-bit)  data is left-aligned → shift right 8
  // In Normal   (10-bit) data is left-aligned → shift right 6
  int shift = low_power_ ? 8 : 4;
  float scale = range_to_scale_();

  float ax = static_cast<float>(raw_x >> shift) * scale;
  float ay = static_cast<float>(raw_y >> shift) * scale;
  float az = static_cast<float>(raw_z >> shift) * scale;

  ESP_LOGD(TAG, "Accel: X=%.4f Y=%.4f Z=%.4f m/s²", ax, ay, az);

  if (accel_x_ != nullptr) accel_x_->publish_state(ax);
  if (accel_y_ != nullptr) accel_y_->publish_state(ay);
  if (accel_z_ != nullptr) accel_z_->publish_state(az);
}

// ─── dump_config ──────────────────────────────────────────────────────────────

void LIS3DHComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LIS3DH Accelerometer:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Failed to initialize!");
    return;
  }

  int range_g = 0;
  switch (range_) {
    case RANGE_2G:  range_g = 2;  break;
    case RANGE_4G:  range_g = 4;  break;
    case RANGE_8G:  range_g = 8;  break;
    case RANGE_16G: range_g = 16; break;
  }

  int odr_hz = 0;
  switch (data_rate_) {
    case DATA_RATE_1HZ:   odr_hz = 1;   break;
    case DATA_RATE_10HZ:  odr_hz = 10;  break;
    case DATA_RATE_25HZ:  odr_hz = 25;  break;
    case DATA_RATE_50HZ:  odr_hz = 50;  break;
    case DATA_RATE_100HZ: odr_hz = 100; break;
    case DATA_RATE_200HZ: odr_hz = 200; break;
    case DATA_RATE_400HZ: odr_hz = 400; break;
  }

  ESP_LOGCONFIG(TAG, "  Range:      ±%dg", range_g);
  ESP_LOGCONFIG(TAG, "  Data rate:  %d Hz", odr_hz);
  ESP_LOGCONFIG(TAG, "  Low-power:  %s", low_power_ ? "yes" : "no");

  LOG_SENSOR("  ", "Accel X", accel_x_);
  LOG_SENSOR("  ", "Accel Y", accel_y_);
  LOG_SENSOR("  ", "Accel Z", accel_z_);
  LOG_UPDATE_INTERVAL(this);
}

// ─── helpers ──────────────────────────────────────────────────────────────────

bool LIS3DHComponent::read_accel_(int16_t &x, int16_t &y, int16_t &z) {
  // Poll STATUS_REG until ZYXDA (bit 3) is set
  uint8_t status = 0;
  for (int attempt = 0; attempt < 10; ++attempt) {
    if (!this->read_register(LIS3DH_REG_STATUS_REG, &status, 1)) return false;
    if (status & 0x08) break;  // ZYXDA
    delay(1);
  }

  // Use auto-increment (MSB of register address = 1 for multi-byte read)
  uint8_t buf[6];
  if (!this->read_register(LIS3DH_REG_OUT_X_L | 0x80, buf, 6)) return false;

  x = static_cast<int16_t>((buf[1] << 8) | buf[0]);
  y = static_cast<int16_t>((buf[3] << 8) | buf[2]);
  z = static_cast<int16_t>((buf[5] << 8) | buf[4]);
  return true;
}

float LIS3DHComponent::range_to_scale_() const {
  // Full-scale sensitivity in mg/digit (12-bit HR mode)
  // After right-shift by 4 the value is in signed 12-bit range: –2048…+2047
  float mg_per_digit = 0.0f;
  switch (range_) {
    case RANGE_2G:  mg_per_digit = 1.0f;  break;
    case RANGE_4G:  mg_per_digit = 2.0f;  break;
    case RANGE_8G:  mg_per_digit = 4.0f;  break;
    case RANGE_16G: mg_per_digit = 12.0f; break;
  }
  if (low_power_) {
    // 8-bit mode: right-shift by 8 → range –128…+127; scale doubles per bit
    mg_per_digit *= 16.0f;
  }
  // mg → m/s²
  return mg_per_digit * 0.001f * GRAVITY;
}

}  // namespace lis3dh
}  // namespace esphome
