#include "lis3dh.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lis3dh {

static const char *const TAG = "lis3dh";
static constexpr float GRAVITY = 9.80665f;

// ─── setup ─────────────────────────────────────────────────────────────────

void LIS3DHComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LIS3DH...");

  // Проверка WHO_AM_I
  uint8_t who = 0;
  if (!this->read_register(LIS3DH_REG_WHO_AM_I, &who, 1)) {
    ESP_LOGE(TAG, "I2C read failed — check wiring / address");
    this->mark_failed();
    return;
  }
  if (who != LIS3DH_WHO_AM_I_VALUE) {
    ESP_LOGE(TAG, "Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", who, LIS3DH_WHO_AM_I_VALUE);
    this->mark_failed();
    return;
  }

  // CTRL_REG1: ODR | LPen | Zen | Yen | Xen
  uint8_t ctrl1 = static_cast<uint8_t>(data_rate_)
                | (low_power_ ? 0x08 : 0x00)
                | 0x07;  // X Y Z enabled
  if (!this->write_register(LIS3DH_REG_CTRL_REG1, &ctrl1, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG1");
    this->mark_failed();
    return;
  }

  // CTRL_REG4: BDU | FS[1:0] | HR
  uint8_t ctrl4 = 0x80                              // BDU
                | static_cast<uint8_t>(range_)      // FS
                | (low_power_ ? 0x00 : 0x08);       // HR (12-bit)
  if (!this->write_register(LIS3DH_REG_CTRL_REG4, &ctrl4, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG4");
    this->mark_failed();
    return;
  }

  // Настройка прерывания для wake-up
  if (wakeup_enabled_) {
    if (!configure_wakeup_interrupt_()) {
      this->mark_failed();
      return;
    }
  }

  initialized_ = true;
  ESP_LOGD(TAG, "LIS3DH initialized OK");
}

// ─── configure_wakeup_interrupt_ ───────────────────────────────────────────

bool LIS3DHComponent::configure_wakeup_interrupt_() {
  // Сбрасываем старое прерывание (чтением INT1_SRC)
  clear_interrupt_();

  // CTRL_REG5: latch interrupt request (LIR_INT1)
  uint8_t ctrl5 = wakeup_latch_ ? 0x08 : 0x00;
  if (!this->write_register(LIS3DH_REG_CTRL_REG5, &ctrl5, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG5");
    return false;
  }

  // CTRL_REG3: роутим IA1 (прерывание от INT1) на вывод INT1
  uint8_t ctrl3 = 0x40;  // IA1 -> INT1 pin
  if (!this->write_register(LIS3DH_REG_CTRL_REG3, &ctrl3, 1)) {
    ESP_LOGE(TAG, "Failed to write CTRL_REG3");
    return false;
  }

  // INT1_THS: порог
  // full_scale_mg / 128 = мг на 1 LSB
  float lsb_mg = range_full_scale_mg_() / 128.0f;
  uint8_t ths = static_cast<uint8_t>(
      std::min(127.0f, std::max(1.0f, (wakeup_threshold_g_ * 1000.0f) / lsb_mg))
  );
  if (!this->write_register(LIS3DH_REG_INT1_THS, &ths, 1)) {
    ESP_LOGE(TAG, "Failed to write INT1_THS");
    return false;
  }

  // INT1_DUR: минимальная продолжительность (в периодах ODR)
  uint8_t dur = wakeup_duration_ & 0x7F;
  if (!this->write_register(LIS3DH_REG_INT1_DUR, &dur, 1)) {
    ESP_LOGE(TAG, "Failed to write INT1_DUR");
    return false;
  }

  // INT1_CFG: какие оси и какой тип события вызывают прерывание
  // Bit pattern: ZHIE ZLIE YHIE YLIE XHIE XLIE
  // AOI=0 → OR (любое событие); AOI=1 → AND (все)
  uint8_t cfg = static_cast<uint8_t>(wakeup_axes_);
  if (!this->write_register(LIS3DH_REG_INT1_CFG, &cfg, 1)) {
    ESP_LOGE(TAG, "Failed to write INT1_CFG");
    return false;
  }

  float actual_ths_g = (ths * lsb_mg) / 1000.0f;
  ESP_LOGD(TAG, "Wakeup interrupt configured:");
  ESP_LOGD(TAG, "  Threshold: %.4f g (reg=0x%02X, ~%.4f g actual)", wakeup_threshold_g_, ths, actual_ths_g);
  ESP_LOGD(TAG, "  Duration:  %u ODR periods", dur);
  ESP_LOGD(TAG, "  Axes cfg:  0x%02X", cfg);
  return true;
}

// ─── update ────────────────────────────────────────────────────────────────

void LIS3DHComponent::update() {
  if (!initialized_) return;

  int16_t raw_x, raw_y, raw_z;
  if (!read_accel_(raw_x, raw_y, raw_z)) {
    ESP_LOGW(TAG, "Failed to read acceleration");
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  // HR mode (12-bit): данные left-aligned → сдвиг 4
  // LP mode  (8-bit): данные left-aligned → сдвиг 8
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

// ─── dump_config ───────────────────────────────────────────────────────────

void LIS3DHComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LIS3DH Accelerometer:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed!");
    return;
  }

  static const char *range_str[]  = {"±2g", "±4g", "±8g", "±16g"};
  static const int   odr_table[]  = {1, 10, 25, 50, 100, 200, 400};

  int range_idx = static_cast<int>(range_) >> 4;
  int odr_idx   = (static_cast<int>(data_rate_) >> 4) - 1;

  ESP_LOGCONFIG(TAG, "  Range:      %s", range_str[range_idx]);
  ESP_LOGCONFIG(TAG, "  Data rate:  %d Hz", odr_table[odr_idx]);
  ESP_LOGCONFIG(TAG, "  Low-power:  %s", low_power_ ? "yes" : "no");

  if (wakeup_enabled_) {
    ESP_LOGCONFIG(TAG, "  Wakeup interrupt: ENABLED");
    ESP_LOGCONFIG(TAG, "    Threshold: %.3f g", wakeup_threshold_g_);
    ESP_LOGCONFIG(TAG, "    Duration:  %u ODR periods", wakeup_duration_);
    ESP_LOGCONFIG(TAG, "    Latch:     %s", wakeup_latch_ ? "yes" : "no");
  }

  LOG_SENSOR("  ", "Accel X", accel_x_);
  LOG_SENSOR("  ", "Accel Y", accel_y_);
  LOG_SENSOR("  ", "Accel Z", accel_z_);
  LOG_UPDATE_INTERVAL(this);
}

// ─── helpers ───────────────────────────────────────────────────────────────

bool LIS3DHComponent::read_accel_(int16_t &x, int16_t &y, int16_t &z) {
  // Ждём флаг ZYXDA в STATUS_REG
  uint8_t status = 0;
  for (int i = 0; i < 10; ++i) {
    if (!this->read_register(LIS3DH_REG_STATUS_REG, &status, 1)) return false;
    if (status & 0x08) break;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // Авто-инкремент: бит 7 адреса = 1
  uint8_t buf[6];
  if (!this->read_register(LIS3DH_REG_OUT_X_L | 0x80, buf, 6)) return false;

  x = static_cast<int16_t>((buf[1] << 8) | buf[0]);
  y = static_cast<int16_t>((buf[3] << 8) | buf[2]);
  z = static_cast<int16_t>((buf[5] << 8) | buf[4]);
  return true;
}

bool LIS3DHComponent::clear_interrupt_() {
  uint8_t src = 0;
  return this->read_register(LIS3DH_REG_INT1_SRC, &src, 1);
}

float LIS3DHComponent::range_to_scale_() const {
  // мг/digit в HR-режиме (после сдвига 4)
  float mg = range_full_scale_mg_() / 2048.0f;  // 12-bit: 2^11 steps for half range
  if (low_power_) mg = range_full_scale_mg_() / 128.0f;  // 8-bit
  return mg * 0.001f * GRAVITY;
}

float LIS3DHComponent::range_full_scale_mg_() const {
  switch (range_) {
    case RANGE_2G:  return 2000.0f;
    case RANGE_4G:  return 4000.0f;
    case RANGE_8G:  return 8000.0f;
    case RANGE_16G: return 16000.0f;
    default:        return 2000.0f;
  }
}

}  // namespace lis3dh
}  // namespace esphome
