import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ICON
from .. import simplebus2_ns, Simplebus2, CONF_SIMPLEBUS2_ID

Simplebus2TextSensor = simplebus2_ns.class_(
    "Simplebus2TextSensor", text_sensor.TextSensor, cg.Component
)

CONF_NAME = "name"

DEPENDENCIES = ["simplebus2"]

CONFIG_SCHEMA = text_sensor.text_sensor_schema(Simplebus2TextSensor).extend(
        {
            cv.GenerateID(CONF_SIMPLEBUS2_ID): cv.use_id(Simplebus2),
            cv.Optional(CONF_ICON, default="mdi:message-cog"): cv.icon,
            cv.Optional(CONF_NAME, default="Last telegram"): cv.string,
        }
    )

async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    simplebus2 = await cg.get_variable(config[CONF_SIMPLEBUS2_ID])
    cg.add(simplebus2.register_listener(var))