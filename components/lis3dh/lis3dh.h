#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace lis3dh {

// ─── Регистры ──────────────────────────────────────────────────────────────
static const uint8_t LIS3DH_REG_WHO_AM_I   = 0x0F;
static const uint8_t LIS3DH_REG_CTRL_REG1  = 0x20;
static const uint8_t LIS3DH_REG_CTRL_REG2  = 0x21;
static const uint8_t LIS3DH_REG_CTRL_REG3  = 0x22;  // INT1 routing
static const uint8_t LIS3DH_REG_CTRL_REG4  = 0x23;  // range / HR
static const uint8_t LIS3DH_REG_CTRL_REG5  = 0x24;  // latch INT
static const uint8_t LIS3DH_REG_CTRL_REG6  = 0x25;  // INT2 routing
static const uint8_t LIS3DH_REG_REFERENCE  = 0x26;
static const uint8_t LIS3DH_REG_STATUS_REG = 0x27;
static const uint8_t LIS3DH_REG_OUT_X_L    = 0x28;

// Interrupt 1
static const uint8_t LIS3DH_REG_INT1_CFG   = 0x30;  // interrupt config
static const uint8_t LIS3DH_REG_INT1_SRC   = 0x31;  // interrupt source (read to clear)
static const uint8_t LIS3DH_REG_INT1_THS   = 0x32;  // threshold 0..127
static const uint8_t LIS3DH_REG_INT1_DUR   = 0x33;  // duration  0..127

static const uint8_t LIS3DH_WHO_AM_I_VALUE = 0x33;

// ─── Перечисления ──────────────────────────────────────────────────────────

enum DataRate : uint8_t {
  DATA_RATE_1HZ   = 0x10,
  DATA_RATE_10HZ  = 0x20,
  DATA_RATE_25HZ  = 0x30,
  DATA_RATE_50HZ  = 0x40,
  DATA_RATE_100HZ = 0x50,
  DATA_RATE_200HZ = 0x60,
  DATA_RATE_400HZ = 0x70,
};

enum AccelRange : uint8_t {
  RANGE_2G  = 0x00,
  RANGE_4G  = 0x10,
  RANGE_8G  = 0x20,
  RANGE_16G = 0x30,
};

// Биты INT1_CFG: комбинируйте через OR
//   ZHIE / ZLIE — Z high / low event
//   YHIE / YLIE — Y high / low event
//   XHIE / XLIE — X high / low event
enum WakeupAxis : uint8_t {
  WAKEUP_AXIS_NONE = 0x00,
  WAKEUP_AXIS_X    = 0x02,  // XHIE
  WAKEUP_AXIS_Y    = 0x08,  // YHIE
  WAKEUP_AXIS_Z    = 0x20,  // ZHIE
  WAKEUP_AXIS_XYZ  = 0x2A,  // X | Y | Z high events
};

// ─── Компонент ─────────────────────────────────────────────────────────────

class LIS3DHComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ── Сенсоры ──
  void set_accel_x_sensor(sensor::Sensor *s) { accel_x_ = s; }
  void set_accel_y_sensor(sensor::Sensor *s) { accel_y_ = s; }
  void set_accel_z_sensor(sensor::Sensor *s) { accel_z_ = s; }

  // ── Основная конфигурация ──
  void set_data_rate(DataRate rate)  { data_rate_ = rate; }
  void set_range(AccelRange range)   { range_ = range; }
  void set_low_power(bool lp)        { low_power_ = lp; }

  // ── Конфигурация прерывания / wake-up ──
  void set_wakeup_enabled(bool en)          { wakeup_enabled_ = en; }
  void set_wakeup_threshold(float thr_g)    { wakeup_threshold_g_ = thr_g; }
  void set_wakeup_duration(uint8_t dur)     { wakeup_duration_ = dur; }
  void set_wakeup_axes(WakeupAxis axes)     { wakeup_axes_ = axes; }
  void set_wakeup_latch(bool latch)         { wakeup_latch_ = latch; }

 protected:
  bool read_accel_(int16_t &x, int16_t &y, int16_t &z);
  float range_to_scale_() const;
  float range_full_scale_mg_() const;
  bool configure_wakeup_interrupt_();
  bool clear_interrupt_();

  // Сенсоры
  sensor::Sensor *accel_x_{nullptr};
  sensor::Sensor *accel_y_{nullptr};
  sensor::Sensor *accel_z_{nullptr};

  // Параметры
  DataRate   data_rate_{DATA_RATE_50HZ};
  AccelRange range_{RANGE_2G};
  bool       low_power_{false};
  bool       initialized_{false};

  // Wake-up / прерывание
  bool       wakeup_enabled_{false};
  float      wakeup_threshold_g_{0.1f};
  uint8_t    wakeup_duration_{1};      // количество периодов ODR
  WakeupAxis wakeup_axes_{WAKEUP_AXIS_XYZ};
  bool       wakeup_latch_{true};
};

}  // namespace lis3dh
}  // namespace esphome
