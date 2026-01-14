#pragma once
#include "esphome.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ds2408_custom {

// DS2408 Command Codes
const uint8_t DS2408_CMD_READ_PIO_REGISTERS = 0xF0;
const uint16_t DS2408_ADDR_PIO_LOGIC_STATE = 0x0088;

class DS2408Component : public PollingComponent {
 public:
  uint64_t address_;
  void *bus_ptr_;
  binary_sensor::BinarySensor *channels_[8] = {nullptr};
  uint8_t last_state_ = 0xFF; // Initialize to all high (inactive)

  // Constructor
  DS2408Component(void *bus, uint64_t address) 
    : PollingComponent(1000), address_(address), bus_ptr_(bus) {}

  void register_channel(uint8_t pin, binary_sensor::BinarySensor *sensor) {
    if (pin < 8) {
      this->channels_[pin] = sensor;
    }
  }

  void setup() override {
    ESP_LOGD("ds2408", "Setting up DS2408 at 0x%llX", this->address_);
  }

  void update() override {
    auto *bus = (esphome::one_wire::OneWireBus *)this->bus_ptr_;
    
    if (!this->bus_reset(bus)) {
       ESP_LOGW("ds2408", "Device 0x%llX not reachable", this->address_);
       this->publish_all_unavailable_();
       return;
    }

    bus->select(this->address_);
    bus->write8(DS2408_CMD_READ_PIO_REGISTERS);
    // Write target address, LSB first
    bus->write8(DS2408_ADDR_PIO_LOGIC_STATE & 0xFF); 
    bus->write8(DS2408_ADDR_PIO_LOGIC_STATE >> 8);
    
    // Read the single byte representing the state of all 8 PIO pins.
    uint8_t pio_state = bus->read8();

    // Read the 2-byte CRC16
    uint16_t crc = bus->read16();

    // Verify CRC
    uint16_t calculated_crc = 0xFFFF;
    calculated_crc = crc16_update(calculated_crc, DS2408_CMD_READ_PIO_REGISTERS);
    calculated_crc = crc16_update(calculated_crc, DS2408_ADDR_PIO_LOGIC_STATE & 0xFF);
    calculated_crc = crc16_update(calculated_crc, DS2408_ADDR_PIO_LOGIC_STATE >> 8);
    calculated_crc = crc16_update(calculated_crc, pio_state);
    
    if (calculated_crc != crc) {
      ESP_LOGW("ds2408", "CRC check failed for device 0x%llX!", this->address_);
      this->publish_all_unavailable_();
      return;
    }

    ESP_LOGD("ds2408", "Read state byte: 0b" BINARY_PATTERN, BINARY_ARGS(pio_state));

    // Update each registered binary sensor if its state has changed
    for (int i = 0; i < 8; i++) {
      if (this->channels_[i] != nullptr) {
        // A LOW signal (0) on the pin is typically 'ON' or 'ACTIVE' for a binary sensor.
        // We invert the logic here to reflect that.
        bool current_pin_state = !((pio_state >> i) & 1);
        bool last_pin_state = !((this->last_state_ >> i) & 1);

        if (current_pin_state != last_pin_state) {
            this->channels_[i]->publish_state(current_pin_state);
        }
      }
    }

    this->last_state_ = pio_state;
  }

 protected:
  void publish_all_unavailable_() {
    for (int i = 0; i < 8; i++) {
      if (this->channels_[i] != nullptr) {
        this->channels_[i]->publish_state(NAN);
      }
    }
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

} // namespace ds2408_custom
} // namespace esphome
