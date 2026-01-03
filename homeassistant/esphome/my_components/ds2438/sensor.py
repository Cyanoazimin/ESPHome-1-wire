import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_VOLTAGE,
    UNIT_CELSIUS,
    UNIT_VOLT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
)

# Define the namespace for the custom component
ds2438_ns = cg.esphome_ns.namespace('ds2438_custom')
DS2438Sensor = ds2438_ns.class_('DS2438Sensor', cg.PollingComponent)

# Define the key for the second voltage sensor
CONF_BUS_VOLTAGE = "bus_voltage"

# Configuration schema
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(DS2438Sensor),
    
    # Using cv.use_id(None) allows the Python loader to find the 1-wire hub
    # without strict type checking that often fails in custom components
    cv.Required('one_wire_id'): cv.use_id(None),
    
    cv.Required(CONF_ADDRESS): cv.hex_uint64_t,
    
    # Define the temperature sensor
    cv.Optional(CONF_TEMPERATURE): 
        sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS, 
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT
        ),
        
    # Define the VAD voltage sensor (Input)
    cv.Optional(CONF_VOLTAGE): 
        sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT, 
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT
        ),

    # Define the VDD voltage sensor (Bus)
    cv.Optional(CONF_BUS_VOLTAGE): 
        sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT, 
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT
        ),
}).extend(cv.polling_component_schema('60s'))

async def to_code(config):
    # Retrieve the 1-Wire bus variable
    hub = await cg.get_variable(config['one_wire_id'])
    
    # Create the C++ sensor object
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_ADDRESS])
    await cg.register_component(var, config)
    
    # Setup the sub-sensors in C++ if they are in the YAML
    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
        
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))

    if CONF_BUS_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BUS_VOLTAGE])
        cg.add(var.set_bus_voltage_sensor(sens))