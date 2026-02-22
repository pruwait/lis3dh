#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace lis3dh {

// ─── Registers ────────────────────────────────────────────────────────────────
static const uint8_t LIS3DH_REG_STATUS_AUX   = 0x07;
static const uint8_t LIS3DH_REG_WHO_AM_I     = 0x0F;
static const uint8_t LIS3DH_REG_CTRL_REG1    = 0x20;
static const uint8_t LIS3DH_REG_CTRL_REG2    = 0x21;
static const uint8_t LIS3DH_REG_CTRL_REG3    = 0x22;
static const uint8_t LIS3DH_REG_CTRL_REG4    = 0x23;
static const uint8_t LIS3DH_REG_CTRL_REG5    = 0x24;
static const uint8_t LIS3DH_REG_STATUS_REG   = 0x27;
static const uint8_t LIS3DH_REG_OUT_X_L      = 0x28;
static const uint8_t LIS3DH_REG_OUT_Y_L      = 0x2A;
static const uint8_t LIS3DH_REG_OUT_Z_L      = 0x2C;

static const uint8_t LIS3DH_WHO_AM_I_VALUE   = 0x33;

// ─── Enums ────────────────────────────────────────────────────────────────────

enum DataRate : uint8_t {
  DATA_RATE_1HZ   = 0x10,  // ODR[3:0] = 0001
  DATA_RATE_10HZ  = 0x20,  // ODR[3:0] = 0010
  DATA_RATE_25HZ  = 0x30,  // ODR[3:0] = 0011
  DATA_RATE_50HZ  = 0x40,  // ODR[3:0] = 0100
  DATA_RATE_100HZ = 0x50,  // ODR[3:0] = 0101
  DATA_RATE_200HZ = 0x60,  // ODR[3:0] = 0110
  DATA_RATE_400HZ = 0x70,  // ODR[3:0] = 0111
};

enum AccelRange : uint8_t {
  RANGE_2G  = 0x00,
  RANGE_4G  = 0x10,
  RANGE_8G  = 0x20,
  RANGE_16G = 0x30,
};

// ─── Component ────────────────────────────────────────────────────────────────

class LIS3DHComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_accel_x_sensor(sensor::Sensor *s) { accel_x_ = s; }
  void set_accel_y_sensor(sensor::Sensor *s) { accel_y_ = s; }
  void set_accel_z_sensor(sensor::Sensor *s) { accel_z_ = s; }
  void set_data_rate(DataRate rate)          { data_rate_ = rate; }
  void set_range(AccelRange range)           { range_ = range; }
  void set_low_power(bool lp)               { low_power_ = lp; }

 protected:
  bool read_accel_(int16_t &x, int16_t &y, int16_t &z);
  float range_to_scale_() const;

  sensor::Sensor *accel_x_{nullptr};
  sensor::Sensor *accel_y_{nullptr};
  sensor::Sensor *accel_z_{nullptr};

  DataRate  data_rate_{DATA_RATE_50HZ};
  AccelRange range_{RANGE_2G};
  bool       low_power_{false};

  bool       initialized_{false};
};

}  // namespace lis3dh
}  // namespace esphome
