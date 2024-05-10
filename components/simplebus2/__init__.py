import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.const import CONF_ID, CONF_FILTER, CONF_IDLE, PLATFORM_ESP32
from esphome.core import coroutine_with_priority, TimePeriod

CODEOWNERS = ["@se-bastiaan"]
simplebus2_ns = cg.esphome_ns.namespace("simplebus2")
Simplebus2 = simplebus2_ns.class_("Simplebus2Component", cg.Component)

Simplebus2SendAction = simplebus2_ns.class_(
    "Simplebus2SendAction", automation.Action
)

CONF_SIMPLEBUS2_ID = "simplebus2"
CONF_GAIN = "gain"
CONF_VOLTAGE_LEVEL = "voltage_level"
CONF_RX_PIN = "rx_pin"
CONF_TX_PIN = "tx_pin"
CONF_EVENT = "event"
CONF_COMMAND = "command"
CONF_ADDRESS = "address"
MULTI_CONF = False

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Simplebus2),
            cv.Optional(CONF_GAIN, default=20): cv.int_,
            cv.Optional(CONF_VOLTAGE_LEVEL, default=600): cv.All(
                cv.int_, cv.Range(min=100, max=1500)
            ),
            cv.Optional(CONF_RX_PIN, default=2): pins.internal_gpio_input_pullup_pin_schema,
            cv.Optional(CONF_TX_PIN, default=3): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_FILTER, default="1000us"): cv.All(
                cv.positive_time_period_microseconds,
                cv.Range(max=TimePeriod(microseconds=2500)),
            ),
            cv.Optional(CONF_IDLE, default="10ms"): cv.positive_time_period_microseconds,
            cv.Optional(CONF_EVENT, default="simplebus2"): cv.string,
        },
        cv.only_on([PLATFORM_ESP32]),
    )
    .extend(cv.COMPONENT_SCHEMA)
)

@coroutine_with_priority(1.0)
async def to_code(config):
    cg.add_global(simplebus2_ns.using)
    cg.add_library("Wire", None)
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))

    pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(pin))

    cg.add(var.set_gain(config[CONF_GAIN]))
    cg.add(var.set_voltage_level(config[CONF_VOLTAGE_LEVEL]))
    cg.add(var.set_event("esphome." + config[CONF_EVENT]))


SIMPLEBUS2_SEND_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Simplebus2),
        cv.Required(CONF_COMMAND): cv.templatable(cv.hex_uint16_t),
        cv.Required(CONF_ADDRESS): cv.templatable(cv.hex_uint16_t),
    }
)


@automation.register_action(
    "simplebus2.send", Simplebus2SendAction, SIMPLEBUS2_SEND_SCHEMA
)
async def simplebus2_send_to_code(config, action_id, template_args, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_args, paren)
    template = await cg.templatable(config[CONF_COMMAND], args, cg.uint16)
    cg.add(var.set_command(template))
    template = await cg.templatable(config[CONF_ADDRESS], args, cg.uint16)
    cg.add(var.set_address(template))
    return var