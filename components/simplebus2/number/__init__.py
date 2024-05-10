import esphome.codegen as cg
import esphome.config_validation as cv
from .. import simplebus2_ns, Simplebus2, CONF_SIMPLEBUS2_ID
from esphome.components import number

VoltageLevelNumber = simplebus2_ns.class_("VoltageLevelNumber", number.Number)
GainNumber = simplebus2_ns.class_("GainNumber", number.Number)

CONF_VOLTAGE_LEVEL = "voltage_level"
CONF_GAIN = "gain"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SIMPLEBUS2_ID): cv.use_id(Simplebus2),
        cv.Optional(CONF_VOLTAGE_LEVEL): number.number_schema(
            VoltageLevelNumber,
        ),
        cv.Optional(CONF_GAIN): number.number_schema(
            GainNumber
        ),
    }
)

async def to_code(config):
    simplebus2 = await cg.get_variable(config[CONF_SIMPLEBUS2_ID])
    if voltage_limit_config := config.get(CONF_VOLTAGE_LEVEL):
        n = await number.new_number(
            voltage_limit_config, min_value=100, max_value=1500, step=50
        )
        n.set_initial_value(simplebus2.get_voltage_level())
        await cg.register_parented(n, config[CONF_SIMPLEBUS2_ID])
    if gain_config := config.get(CONF_GAIN):
        n = await number.new_number(
            gain_config, min_value=2, max_value=40, step=1
        )
        n.set_initial_value(simplebus2.get_gain())
        await cg.register_parented(n, config[CONF_SIMPLEBUS2_ID])