import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_RANGE,
    CONF_UPDATE_INTERVAL,
    ICON_BRIEFCASE_DOWNLOAD,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER_PER_SECOND_SQUARED,
)

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]

CONF_ACCEL_X        = "accel_x"
CONF_ACCEL_Y        = "accel_y"
CONF_ACCEL_Z        = "accel_z"
CONF_DATA_RATE      = "data_rate"
CONF_LOW_POWER      = "low_power"
CONF_WAKEUP         = "wakeup"
CONF_THRESHOLD      = "threshold"
CONF_DURATION       = "duration"
CONF_AXES           = "axes"
CONF_LATCH          = "latch"

lis3dh_ns = cg.esphome_ns.namespace("lis3dh")
LIS3DHComponent = lis3dh_ns.class_(
    "LIS3DHComponent", cg.PollingComponent, i2c.I2CDevice
)

DataRate = lis3dh_ns.enum("DataRate")
DATA_RATES = {
    1:   DataRate.DATA_RATE_1HZ,
    10:  DataRate.DATA_RATE_10HZ,
    25:  DataRate.DATA_RATE_25HZ,
    50:  DataRate.DATA_RATE_50HZ,
    100: DataRate.DATA_RATE_100HZ,
    200: DataRate.DATA_RATE_200HZ,
    400: DataRate.DATA_RATE_400HZ,
}

AccelRange = lis3dh_ns.enum("AccelRange")
ACCEL_RANGES = {
    2:  AccelRange.RANGE_2G,
    4:  AccelRange.RANGE_4G,
    8:  AccelRange.RANGE_8G,
    16: AccelRange.RANGE_16G,
}

WakeupAxis = lis3dh_ns.enum("WakeupAxis")
WAKEUP_AXES = {
    "x":   WakeupAxis.WAKEUP_AXIS_X,
    "y":   WakeupAxis.WAKEUP_AXIS_Y,
    "z":   WakeupAxis.WAKEUP_AXIS_Z,
    "xyz": WakeupAxis.WAKEUP_AXIS_XYZ,
}

WAKEUP_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_THRESHOLD, default=0.1): cv.float_range(min=0.001, max=16.0),
        cv.Optional(CONF_DURATION,  default=1):   cv.int_range(min=0, max=127),
        cv.Optional(CONF_AXES,      default="xyz"): cv.enum(WAKEUP_AXES),
        cv.Optional(CONF_LATCH,     default=True):  cv.boolean,
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LIS3DHComponent),
            cv.Optional(CONF_ACCEL_X): sensor.sensor_schema(
                unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
                icon=ICON_BRIEFCASE_DOWNLOAD,
                accuracy_decimals=4,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACCEL_Y): sensor.sensor_schema(
                unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
                icon=ICON_BRIEFCASE_DOWNLOAD,
                accuracy_decimals=4,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACCEL_Z): sensor.sensor_schema(
                unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
                icon=ICON_BRIEFCASE_DOWNLOAD,
                accuracy_decimals=4,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DATA_RATE, default=50): cv.enum(DATA_RATES, int=True),
            cv.Optional(CONF_RANGE,     default=2):  cv.enum(ACCEL_RANGES, int=True),
            cv.Optional(CONF_LOW_POWER, default=False): cv.boolean,
            cv.Optional(CONF_WAKEUP): WAKEUP_SCHEMA,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x18))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_data_rate(config[CONF_DATA_RATE]))
    cg.add(var.set_range(config[CONF_RANGE]))
    cg.add(var.set_low_power(config[CONF_LOW_POWER]))

    if CONF_ACCEL_X in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_X])
        cg.add(var.set_accel_x_sensor(sens))

    if CONF_ACCEL_Y in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_Y])
        cg.add(var.set_accel_y_sensor(sens))

    if CONF_ACCEL_Z in config:
        sens = await sensor.new_sensor(config[CONF_ACCEL_Z])
        cg.add(var.set_accel_z_sensor(sens))

    if CONF_WAKEUP in config:
        wakeup = config[CONF_WAKEUP]
        cg.add(var.set_wakeup_enabled(True))
        cg.add(var.set_wakeup_threshold(wakeup[CONF_THRESHOLD]))
        cg.add(var.set_wakeup_duration(wakeup[CONF_DURATION]))
        cg.add(var.set_wakeup_axes(wakeup[CONF_AXES]))
        cg.add(var.set_wakeup_latch(wakeup[CONF_LATCH]))
