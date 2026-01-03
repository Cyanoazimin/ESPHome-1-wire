#pragma once
#include "esphome.h"

namespace esphome {
namespace ds2438_custom {

class DS2438Sensor : public PollingComponent {
 public:
  sensor::Sensor *temperature_sensor{nullptr};
  sensor::Sensor *voltage_sensor{nullptr};      // Maps to VAD (General A/D input)
  sensor::Sensor *bus_voltage_sensor{nullptr};  // Maps to VDD (Bus Supply)
  
  uint64_t address_;
  void *bus_ptr_; 

  // Constructor
  DS2438Sensor(void *bus, uint64_t address) 
    : PollingComponent(60000), address_(address), bus_ptr_(bus) {}

  void set_temperature_sensor(sensor::Sensor *s) { temperature_sensor = s; }
  void set_voltage_sensor(sensor::Sensor *s) { voltage_sensor = s; }
  void set_bus_voltage_sensor(sensor::Sensor *s) { bus_voltage_sensor = s; }

  void update() override {
    auto *bus = (esphome::one_wire::OneWireBus *)bus_ptr_;
    
    // Declare variables at the top so they are available for logging at the end
    float temp = NAN;
    float vdd = NAN;
    float vad = NAN;
	
    if (!this->bus_reset(bus)) {
       ESP_LOGW("ds2438", "Device 0x%llX not reachable", address_);
       return;
    }

    // --- PHASE 1: Temperature & VDD (Bus Supply Voltage) ---
    
    // 1. Configure Control Register: According to Datasheet AD bit = 1 selects VDD (Battery Input)
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0x4E); // Write Scratchpad command
    bus->write8(0x00); // Target Page 0
    bus->write8(0x08); // Config: AD=1 (Selects VDD), IAD=0, CA=0, EE=0
    delay(10);

    // 2. Trigger Conversions
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0x44); // Convert Temperature
    delay(10);

    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xB4); // Convert Voltage (for VDD)
    delay(100); // Datasheet requires ~10ms, we wait 100ms for stability

    // 3. Recall & Read Results
    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xB8); // Recall Memory (copies internal registers to scratchpad)
    bus->write8(0x00); 
    delay(10);

    this->bus_reset(bus);
    bus->select(address_);
    bus->write8(0xBE); // Read Scratchpad
    bus->write8(0x00);

    uint8_t data[9];
    for (int i = 0; i < 9; i++) data[i] = bus->read8();

    // Temperature: 13-bit resolution (stored in bytes 1 & 2)
    int16_t temp_raw = (int16_t)((data[2] << 8) | data[1]);
    temp = temp_raw / 256.0f;
    // VDD (Bus Voltage): 10mV resolution (stored in bytes 3 & 4)
    uint16_t vdd_raw = (uint16_t)((data[4] << 8) | data[3]);
    vdd = vdd_raw / 100.0f;

    if (temperature_sensor != nullptr) temperature_sensor->publish_state(temp);
    if (bus_voltage_sensor != nullptr) bus_voltage_sensor->publish_state(vdd);

    // --- PHASE 2: VAD (General Purpose A/D Input) ---
    // According to Datasheet: AD bit = 0 selects VAD
    // Only perform second measurement if the voltage_sensor (VAD) is actually used in YAML
    if (voltage_sensor != nullptr) {
        
        // 1. Configure Control Register: Set AD bit = 1 (Select VAD)
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0x4E); 
        bus->write8(0x00); 
        bus->write8(0x00); // Config: AD=0 (Selects VAD)
        delay(10);

        // 2. Trigger Voltage Conversion for VAD
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xB4); // Convert Voltage (VAD)
        delay(100); 

        // 3. Recall & Read Results
        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xB8); 
        bus->write8(0x00); 
        delay(10);

        this->bus_reset(bus);
        bus->select(address_);
        bus->write8(0xBE);
        bus->write8(0x00);

        for (int i = 0; i < 9; i++) data[i] = bus->read8();

        // VAD (measured Voltage): 10mV resolution
        uint16_t vad_raw = (uint16_t)((data[4] << 8) | data[3]);
        vad = vad_raw / 100.0f;
        voltage_sensor->publish_state(vad);
    }
    // Now all variables (temp, vdd, vad) are "in scope" and can be logged
    ESP_LOGD("ds2438", "Results -> Bus(VDD): %.2f V, Voltage(VAD): %.2f V, Temp: %.1f °C", vdd, vad, temp);
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