import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_PIN,
)

# Define the namespace for the custom component
ds2408_ns = cg.esphome_ns.namespace('ds2408_custom')
DS2408Component = ds2408_ns.class_('DS2408Component', cg.PollingComponent)

# Schema for an individual binary sensor channel
DS2408_BINARY_SENSOR_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(binary_sensor.BinarySensor),
    cv.Required(CONF_PIN): cv.int_range(min=0, max=7),
})

# Main configuration schema for the DS2408 hub/component
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DS2408Component),
    cv.Required('one_wire_id'): cv.use_id(None),
    cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
    
    # Define a list of channels, each being a binary sensor
    cv.Optional('channels'): cv.ensure_list(DS2408_BINARY_SENSOR_SCHEMA),
    
}).extend(cv.polling_component_schema('1s')) # Poll quickly for responsive binary sensors

async def to_code(config):
    hub = await cg.get_variable(config['one_wire_id'])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_ADDRESS])
    await cg.register_component(var, config)

    if 'channels' in config:
        for i, channel_config in enumerate(config['channels']):
            pin_number = channel_config[CONF_PIN]
            # Create a new binary sensor
            sens = await binary_sensor.new_binary_sensor(channel_config)
            # Register the binary sensor with the parent DS2408 component
            cg.add(var.register_channel(pin_number, sens))
