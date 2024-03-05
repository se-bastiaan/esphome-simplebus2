import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_ICON
from .. import simplebus2_ns, Simplebus2, CONF_SIMPLEBUS2_ID

Simplebus2BinarySensor = simplebus2_ns.class_(
    "Simplebus2BinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_COMMAND = "command"
CONF_ADDRESS = "address"
CONF_NAME = "name"
CONF_AUTO_OFF = "auto_off"

DEPENDENCIES = ["simplebus2"]

CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(Simplebus2BinarySensor).extend(
        {
            cv.GenerateID(): cv.declare_id(Simplebus2BinarySensor),
            cv.GenerateID(CONF_SIMPLEBUS2_ID): cv.use_id(Simplebus2),
            cv.Required(CONF_ADDRESS): cv.int_,
            cv.Optional(CONF_COMMAND, default=50): cv.int_,
            cv.Optional(CONF_ICON, default="mdi:doorbell"): cv.icon,
            cv.Optional(CONF_NAME, default="Incoming call"): cv.string,
            cv.Optional(CONF_AUTO_OFF, default="30s"): cv.positive_time_period_seconds
        }
    ),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await binary_sensor.register_binary_sensor(var, config)
    cg.add(var.set_command(config[CONF_COMMAND]))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_auto_off(config[CONF_AUTO_OFF]))
    simplebus2 = await cg.get_variable(config[CONF_SIMPLEBUS2_ID])
    cg.add(simplebus2.register_listener(var))