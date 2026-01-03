# ESPHome-1-wire
Connecting Maxim's 1-wire devices via ESPHome to Home Assistant 

A robust custom component for **ESPHome** to read temperature, input voltage (VAD), and supply voltage (VDD) from the **Maxim DS2438 Smart Battery Monitor** via the 1-Wire bus.

## 🚀 Key Features

* **Fully Compatible with ESPHome 2025.12+**: Resolves Python class path issues and protected member access (`reset_`) introduced in recent ESPHome updates.
* **Dual Voltage Support**: Measures both **Input Voltage (VAD)** and **Bus Voltage (VDD)** by dynamically reconfiguring the chip's control register.
* **Reliable Data**: Implements the "Recall Memory" sequence to fetch fresh data from internal latches.
* **Stable Timing**: Includes appropriate delays for A/D conversion.

## 🛠 Installation
1.  Create a folder named `my_components` in your ESPHome configuration directory (if it doesn't exist).
2.  Create a subfolder `ds2438`.
3.  Copy `sensor.py` and `ds2438.h` into `/config/esphome/my_components/ds2438/`.

Directory structure:
```text
/config/esphome/
├── my_components/
│   └── ds2438/
│       ├── sensor.py
│       └── ds2438.h
└── your_device.yaml


## ⚙️ Configuration
Add the following to your ESPHome YAML configuration file:
```yaml
# 1. Define the 1-Wire Bus
one_wire:
  - platform: gpio
    pin: GPIO4
    id: one_wire_bus

# 2. Add the Custom Sensor
sensor:
- platform: ds2438
    one_wire_id: one_wire_bus
    address: 0xd56dc98711646128
    update_interval: 60s
    
    temperature:
      name: "Battery Temperature"
      
    voltage:
      name: "Battery Input Voltage (VAD)"
      unit_of_measurement: "V"
      
    bus_voltage:
      name: "Bus Supply Voltage (VDD)"
      unit_of_measurement: "V"
``` 

🛠 How it works
The DS2438 shares a single A/D converter for both voltage inputs. This component handles the switching logic automatically:

Configures the chip to measure VAD, waits for conversion, and reads the result.

Configures the chip to measure VDD, waits for conversion, and reads the result.

🐛 Troubleshooting
Values stay at 0.00: Check the logs. If you see Raw Data: 00 00 ..., the chip might not be powered correctly (ensure VDD is connected if not using parasitic power) or the A/D conversion time is too short (the code uses safe defaults of 100ms).

Compilation Error reset_: This component uses a C++ access trick to call the protected reset_() method of the OneWireBus, which is required for ESPHome versions >= 2025.12.

📝 License
Open Source / MIT. Feel free to use and modify.