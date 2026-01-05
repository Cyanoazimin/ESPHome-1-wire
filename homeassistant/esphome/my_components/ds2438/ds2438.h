#pragma once
#include "esphome.h"

namespace esphome {
namespace ds2438_custom {

class DS2438Sensor : public PollingComponent {
 public:
  sensor::Sensor *temperature_sensor{nullptr};
  sensor::Sensor *voltage_sensor{nullptr};      // Maps to VAD (General A/D input)
  sensor::Sensor *bus_voltage_sensor{nullptr};  // Maps to VDD (Bus Supply)
  sensor::Sensor *current_sensor{nullptr};      // Current (I)
  
  uint64_t address_;
  void *bus_ptr_; 
  float shunt_ohm_; // Stores the resistor value

  // Constructor
  DS2438Sensor(void *bus, uint64_t address, float shunt_ohm) 
    : PollingComponent(60000), address_(address), bus_ptr_(bus), shunt_ohm_(shunt_ohm) {}

  void set_temperature_sensor(sensor::Sensor *s) { temperature_sensor = s; }
  void set_voltage_sensor(sensor::Sensor *s) { voltage_sensor = s; }
  void set_bus_voltage_sensor(sensor::Sensor *s) { bus_voltage_sensor = s; }
  void set_current_sensor(sensor::Sensor *s) { current_sensor = s; }

  void update() override {
    auto *bus = (esphome::one_wire::OneWireBus *)bus_ptr_;
    
    // Declare variables at the top so they are available for logging at the end
    float temp = NAN;
    float vdd = NAN;
    float vad = NAN;
    float current_amp = NAN;
	
    if (!this->bus_reset(bus)) {
       ESP_LOGW("ds2438", "Device 0x%llX not reachable", address_);
       return;
    }

    // --- PHASE 1: Temperature, VDD (Bus Supply Voltage) & Current ---
    
    // 1. Configure Control Register: According to Datasheet AD bit = 1 selects VDD (Battery Input)
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0x4E); // Write Scratchpad command
    bus->write8(0x00); // Target Page 0
    // Config: AD=1 (Select VDD), IAD=1 (Enable Current Measurement), CA=0, EE=0
    // Binary: 0000 1001 -> 0x09
    bus->write8(0x09); 
    delay(10);

    // 2. Trigger Conversions
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0x44); // Convert Temperature
    delay(10);

    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xB4); // Convert Voltage (VDD)
    delay(100); // Datasheet requires ~10ms, we wait 100ms for stability
	
    // Note: No "Convert Current" command exists. 
    // The DS2438 measures current continuously in the background at 36.41Hz when IAD=1.

    // 3. Recall Memory & Read Results (Copies EEPROM/Latches to Scratchpad)
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xB8); // Recall Memory (copies internal registers to scratchpad)
    bus->write8(0x00); 
    delay(10);

    // Read Scratchpad (9 Bytes)
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xBE);
    bus->write8(0x00); // Read Scratchpad

    uint8_t data[9];
    for (int i = 0; i < 9; i++) data[i] = bus->read8();

    // Temperature: 13-bit resolution (stored in bytes 1 & 2)
    int16_t temp_raw = (int16_t)((data[2] << 8) | data[1]);
    temp = temp_raw / 256.0f;
    if (temperature_sensor != nullptr) temperature_sensor->publish_state(temp);
	
    // VDD (Bus Voltage): 10mV resolution (stored in bytes 3 & 4)
    uint16_t vdd_raw = (uint16_t)((data[4] << 8) | data[3]);
    vdd = vdd_raw / 100.0f;
	if (bus_voltage_sensor != nullptr) bus_voltage_sensor->publish_state(vdd);

    // 3. Current (Bytes 5 & 6)
    // The current register holds the voltage drop across the shunt.
    // Resolution: 244.1 microvolts per LSB (0.0002441 Volts).
    int16_t current_raw = (int16_t)((data[6] << 8) | data[5]);
    // Formula: I = V_shunt / R_shunt
    current_amp = (current_raw * 0.0002441f) / shunt_ohm_;
    
    if (current_sensor != nullptr) current_sensor->publish_state(current_amp);
    
    // --- PHASE 2: VAD (General Purpose A/D Input) ---
    // According to Datasheet: AD bit = 0 selects VAD
    // Only perform second measurement if the voltage_sensor (VAD) is actually used in YAML
    if (voltage_sensor != nullptr) {
        
        // 1. Configure Control Register: Set AD bit = 1 (Select VAD)
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0x4E); 
        bus->write8(0x00); 
        // Config: AD=0 (Select VAD), IAD=1 (Keep Current Measurement ON)
        // Binary: 0000 0001 -> 0x01
        bus->write8(0x01); 
        delay(10);

        // 2. Trigger Voltage Conversion for VAD
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xB4); // Convert Voltage (VAD)
        delay(100); 

        // 3. Recall & Read Results
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xB8); // Recall
        bus->write8(0x00); 
        delay(10);

        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xBE); // Read
        bus->write8(0x00);

        for (int i = 0; i < 9; i++) data[i] = bus->read8();

        // VAD (measured Voltage): 10mV resolution
        uint16_t vad_raw = (uint16_t)((data[4] << 8) | data[3]);
        vad = vad_raw / 100.0f;
        voltage_sensor->publish_state(vad);
    }
    // Now all variables (temp, vdd, vad) are "in scope" and can be logged
    ESP_LOGD("ds2438", "VDD: %.2f V, VAD: %.2f V, I: %.4f A, Temp: %.1f °C", vdd, vad, current_amp, temp);
  }

  /**
   * Helper to access the protected reset_() method of the OneWireBus.
   * Required for ESPHome versions 2025.12 and newer.
   */
  bool bus_reset(esphome::one_wire::OneWireBus *bus) {
    class AccessBus : public esphome::one_wire::OneWireBus {
      public: static bool do_reset(esphome::one_wire::OneWireBus *b) {
        return ((AccessBus*)b)->reset_();
      }
    };
    return AccessBus::do_reset(bus);
  }
};

} // namespace ds2438_custom
} // namespace esphome