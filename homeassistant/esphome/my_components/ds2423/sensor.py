import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    UNIT_EMPTY,
    ICON_COUNTER,
    STATE_CLASS_TOTAL_INCREASING,
)

# Define the namespace for the custom component
ds2423_ns = cg.esphome_ns.namespace('ds2423_custom')
DS2423Sensor = ds2423_ns.class_('DS2423Sensor', cg.PollingComponent)

CONF_COUNTER_A = "counter_a"
CONF_COUNTER_B = "counter_b"

# Configuration schema for the DS2423 component
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DS2423Sensor),
    cv.Required('one_wire_id'): cv.use_id(None),
    cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
    
    # Define the two counter sensors
    cv.Optional(CONF_COUNTER_A): 
        sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY, 
            accuracy_decimals=0,
            icon=ICON_COUNTER,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
    cv.Optional(CONF_COUNTER_B): 
        sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY, 
            accuracy_decimals=0,
            icon=ICON_COUNTER,
            state_class=STATE_CLASS_TOTAL_INCREASING
        ),
}).extend(cv.polling_component_schema('60s'))

async def to_code(config):
    hub = await cg.get_variable(config['one_wire_id'])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_ADDRESS])
    await cg.register_component(var, config)
    
    if CONF_COUNTER_A in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_A])
        cg.add(var.set_counter_a_sensor(sens))
        
    if CONF_COUNTER_B in config:
        sens = await sensor.new_sensor(config[CONF_COUNTER_B])
        cg.add(var.set_counter_b_sensor(sens))
