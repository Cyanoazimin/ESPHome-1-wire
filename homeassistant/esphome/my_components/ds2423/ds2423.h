#pragma once
#include "esphome.h"

namespace esphome {
namespace ds2423_custom {

// DS2423 Command Codes
const uint8_t DS2423_CMD_READ_MEMORY_COUNTER = 0xA5;

class DS2423Sensor : public PollingComponent {
 public:
  sensor::Sensor *counter_a_sensor{nullptr};
  sensor::Sensor *counter_b_sensor{nullptr};
  
  uint64_t address_;
  void *bus_ptr_; 

  // Constructor
  DS2423Sensor(void *bus, uint64_t address) 
    : PollingComponent(60000), address_(address), bus_ptr_(bus) {}

  void set_counter_a_sensor(sensor::Sensor *s) { counter_a_sensor = s; }
  void set_counter_b_sensor(sensor::Sensor *s) { counter_b_sensor = s; }

  void setup() override {
    ESP_LOGD("ds2423", "Setting up DS2423 sensor at address 0x%llX", address_);
  }

  void update() override {
    // Read Counter A if it is configured
    if (this->counter_a_sensor) {
      // Counter A is on Page 14, which starts at address 0x01C0
      float counter_a_val = this->read_counter_page(0x01C0);
      if (!isnan(counter_a_val)) {
        ESP_LOGD("ds2423", "Read Counter A: %.0f", counter_a_val);
        this->counter_a_sensor->publish_state(counter_a_val);
      }
    }

    // Read Counter B if it is configured
    if (this->counter_b_sensor) {
      // Counter B is on Page 15, which starts at address 0x01E0
      float counter_b_val = this->read_counter_page(0x01E0);
      if (!isnan(counter_b_val)) {
        ESP_LOGD("ds2423", "Read Counter B: %.0f", counter_b_val);
        this->counter_b_sensor->publish_state(counter_b_val);
      }
    }
  }

 protected:
  float read_counter_page(uint16_t address) {
    auto *bus = (esphome::one_wire::OneWireBus *)this->bus_ptr_;
    
    if (!this->bus_reset(bus)) {
       ESP_LOGW("ds2423", "Device 0x%llX not reachable for address 0x%04X", this->address_, address);
       return NAN;
    }

    bus->select(this->address_);
    bus->write8(DS2423_CMD_READ_MEMORY_COUNTER);
    // Write the 2-byte target address, LSB first
    bus->write8(address & 0xFF);
    bus->write8(address >> 8);

    // Read and discard the 32 bytes of memory data from the page
    for (int i = 0; i < 32; i++) {
      bus->read8();
    }

    // Read the 4 bytes of the counter
    uint8_t counter_data[4];
    for (int i = 0; i < 4; i++) {
      counter_data[i] = bus->read8();
    }

    // The datasheet indicates LSB is transferred first.
    uint32_t counter_raw = (uint32_t)counter_data[0] | 
                           (uint32_t)counter_data[1] << 8 | 
                           (uint32_t)counter_data[2] << 16 | 
                           (uint32_t)counter_data[3] << 24;
    
    // The remaining bytes are 32 zero-bits and a 16-bit CRC, which we will ignore for now.
    
    return (float)counter_raw;
  }

  /**
   * Helper to access the protected reset_() method of the OneWireBus.
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

} // namespace ds2423_custom
} // namespace esphome
