import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, i2c
from esphome.const import CONF_SCL, CONF_SDA, CONF_ID, CONF_LAMBDA, CONF_INTENSITY, CONF_MODE, CONF_POWER, CONF_LENGTH
import re

DEPENDENCIES = ["i2c"]

tm1650_ns = cg.esphome_ns.namespace("tm1650")
TM1650Display = tm1650_ns.class_("TM1650Display", cg.PollingComponent, i2c.I2CDevice)
TM1650DisplayRef = TM1650Display.operator("ref")

CONF_SEGMENT_MAP = "segment_map"

CONFIG_SCHEMA = display.BASIC_DISPLAY_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(TM1650Display),
    cv.Optional(CONF_INTENSITY, default=1): cv.All(
        cv.uint8_t, cv.Range(min=0, max=8)
    ),
    cv.Optional(CONF_MODE, default=0): cv.All(
        cv.uint8_t, cv.Range(min=0, max=1)
    ),
    cv.Optional(CONF_SEGMENT_MAP, default="PABCDEFG"): cv.All(
        cv.string, cv.Length(min=8, max=8)
    ),
    cv.Optional(CONF_POWER, default=True): cv.boolean,
    cv.Optional(CONF_LENGTH, default=4): cv.All(cv.uint8_t, cv.Range(min=1, max=16)),
}).extend(cv.polling_component_schema("1s")).extend(i2c.i2c_device_schema(0x24))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await display.register_display(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_intensity(config[CONF_INTENSITY]))
    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_segment_map(re.sub(r"[^A-G]", "H", config[CONF_SEGMENT_MAP].upper())))
    cg.add(var.set_power(config[CONF_POWER]))
    cg.add(var.set_length(config[CONF_LENGTH]))

    if CONF_LAMBDA in config:
        cg.add(var.set_writer(await cg.process_lambda(
            config[CONF_LAMBDA], [(TM1650DisplayRef, "it")], return_type=cg.void
        )))
