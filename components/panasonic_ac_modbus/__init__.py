import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import modbus, uart
from esphome.const import CONF_ID, CONF_ADDRESS

AUTO_LOAD = ["modbus"]
DEPENDENCIES = ["uart", "modbus"]

panasonic_ac_modbus_ns = cg.esphome_ns.namespace("panasonic_ac_modbus")
PanasonicACModbus = panasonic_ac_modbus_ns.class_(
    "PanasonicACModbus", cg.Component, modbus.ModbusDevice
)

CONF_PANASONIC_AC_MODBUS_ID = "panasonic_ac_modbus_id"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PanasonicACModbus),
            cv.Optional(CONF_ADDRESS, default=0x01): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(modbus.modbus_device_schema(0x01))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await modbus.register_modbus_device(var, config)
