import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_DISTANCE,
    UNIT_CENTIMETER,
    UNIT_PERCENT,
    ICON_SIGNAL,
    ICON_FLASH,
    ICON_MOTION_SENSOR,
)
from . import CONF_LD2410S_ID, LD2410SComponent

DEPENDENCIES = ["ld2410s"]
CONF_MOVING_DISTANCE = "moving_distance"
CONF_STILL_DISTANCE = "still_distance"
CONF_MOVING_ENERGY = "moving_energy"
CONF_STILL_ENERGY = "still_energy"
CONF_DETECTION_DISTANCE = "detection_distance"
CONF_MOVE_ENERGY = "move_energy"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LD2410S_ID): cv.use_id(LD2410SComponent),
        cv.Optional(CONF_MOVING_DISTANCE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DISTANCE,
            unit_of_measurement=UNIT_CENTIMETER,
            icon=ICON_SIGNAL,
        ),
        cv.Optional(CONF_STILL_DISTANCE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DISTANCE,
            unit_of_measurement=UNIT_CENTIMETER,
            icon=ICON_SIGNAL,
        ),
        cv.Optional(CONF_MOVING_ENERGY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon=ICON_MOTION_SENSOR,
        ),
        cv.Optional(CONF_STILL_ENERGY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon=ICON_FLASH,
        ),
        cv.Optional(CONF_DETECTION_DISTANCE): sensor.sensor_schema(
            device_class=DEVICE_CLASS_DISTANCE,
            unit_of_measurement=UNIT_CENTIMETER,
            icon=ICON_SIGNAL,
        ),
    }
)

async def to_code(config):
    ld2410s_component = await cg.get_variable(config[CONF_LD2410S_ID])
    if moving_distance_config := config.get(CONF_MOVING_DISTANCE):
        sens = await sensor.new_sensor(moving_distance_config)
        cg.add(ld2410s_component.set_moving_target_distance_sensor(sens))
    if still_distance_config := config.get(CONF_STILL_DISTANCE):
        sens = await sensor.new_sensor(still_distance_config)
        cg.add(ld2410s_component.set_still_target_distance_sensor(sens))
    if moving_energy_config := config.get(CONF_MOVING_ENERGY):
        sens = await sensor.new_sensor(moving_energy_config)
        cg.add(ld2410s_component.set_moving_target_energy_sensor(sens))
    if still_energy_config := config.get(CONF_STILL_ENERGY):
        sens = await sensor.new_sensor(still_energy_config)
        cg.add(ld2410s_component.set_still_target_energy_sensor(sens))
    if detection_distance_config := config.get(CONF_DETECTION_DISTANCE):
        sens = await sensor.new_sensor(detection_distance_config)
        cg.add(ld2410s_component.set_detection_distance_sensor(sens))
