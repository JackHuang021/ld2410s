import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_THROTTLE
from esphome.automation import maybe_simple_id

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@sebcaps", "@regevbr", "@Jack Huang"]
MULTI_CONF = True

ld2410s_ns = cg.esphome_ns.namespace("ld2410s")
LD2410SComponent = ld2410s_ns.class_("LD2410SComponent", cg.Component, uart.UARTDevice)

CONF_LD2410S_ID = "ld2410s_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LD2410SComponent),
    }
)

CONFIG_SCHEMA = cv.All(
    CONFIG_SCHEMA.extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "ld2410s",
    require_tx = True,
    require_rx = True,
    parity = "NONE",
    stop_bits = 1,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)


CALIBRATION_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(LD2410SComponent),
    }
)