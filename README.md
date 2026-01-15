# ESPHome-1-wire

Connecting Maxim's 1-wire devices like **DS2438**, **DS2408**, and **DS2423** via ESPHome to Home Assistant.

A robust collection of custom components for **ESPHome** to read temperature, voltages, currents, counters, and control digital I/O via the 1-Wire bus.

### 🔋 DS2438 (Smart Battery Monitor)
* **Fully Compatible with ESPHome 2025.12+**: Resolves Python class path issues and protected member access (`reset_`) introduced in recent ESPHome updates.
* **Dual Voltage Support**: Measures both **Input Voltage (VAD)** and **Bus Voltage (VDD)** by dynamically reconfiguring the chip's control register (0x4E Write Scratchpad).
* **Current Sensing**: Supports current measurement via shunt resistor (IAD), automatically calculating amperage based on the configured shunt resistance.
* **Reliable Data**: Implements the "Recall Memory" sequence to fetch fresh data from internal latches.

### 🔌 DS2408 (8-Channel Addressable Switch)
* **8 Digital I/O Channels**: Read input states or control outputs via Home Assistant.
* **Binary Sensor Integration**: Maps PIO pins to ESPHome binary sensors.
* **Efficient Polling**: Reads all channels in a single transaction using `Read PIO Registers`.

### 🔢 DS2423 (Dual 32-bit Counter)
* **Dual Counters**: Supports both Counter A and Counter B inputs.
* **Battery Backed**: Reads from non-volatile memory pages (0x01C0 / 0x01E0).
* **High Precision**: 32-bit resolution for rain gauges, anemometers, or energy meters.

## 📋 Prerequisites

Before using this component, ensure you have:
- **ESP32 or ESP8266** microcontroller board
- **Maxim DS2438 Smart Battery Monitor** IC
- **1-Wire pull-up resistor** (typically 4.7kΩ)
- Basic soldering skills and familiarity with ESPHome
- [ESPHome documentation](https://esphome.io/)
- [DS2438 Datasheet](https://www.maximintegrated.com/en/products/ibutton/ibutton-products/DS2438.html)

## 🛠 Installation
1.  Create a folder named `my_components` in your ESPHome configuration directory (if it doesn't exist).
2.  Create subfolders `ds2438`, `ds2408`, and `ds2423` inside `my_components` as needed.
3.  Copy the respective component files (`.h`, `sensor.py` / `binary_sensor.py`, `__init__.py`) into their folders.

Directory structure:
```text
/config/esphome/
├── my_components/
│   └── ds2438/
│       ├── __init__.py
│       ├── sensor.py
│       └── ds2438.h
│   └── ds2408/ ...
│   └── ds2423/ ...
└── your_device.yaml
```

## 🔌 Hardware Wiring

Connect your DS2438 to your ESP board as follows:

### DS2438
| DS2438 Pin | ESP Pin | Notes |
|-----------|---------|-------|
| DQ (1)    | GPIO4   | Data line (1-Wire bus) with 4.7kΩ pull-up to VDD |
| GND (2)   | GND     | Ground |
| VDD (3)   | 3.3V    | Power supply |
| VAD (4)   | Sensor  | Analog Input (0-10V) |
| IAD/ISENS | Shunt   | Current Sense Inputs |

### DS2408
| DS2408 Pin | Note |
|------------|------|
| P0-P7      | Digital I/O |
| RSTZ       | Reset Input |

### DS2423
| DS2423 Pin | Note |
|------------|------|
| A/B        | Counter Inputs |

**Finding your DS2438 address**: After configuration, check your ESPHome logs during the first boot to find your device's 1-Wire address. You can also use ESPHome's 1-Wire component debug mode to scan for devices.

## ⚙️ Configuration

Add the following to your ESPHome YAML configuration file (more comprehensive example "warmwasserspeicher.yaml" in the esphome folder):

```yaml
# 1. Define the 1-Wire Bus
one_wire:
  - platform: gpio
    pin: GPIO4
    id: one_wire_bus

# 2. DS2438 Sensor
sensor:
  - platform: ds2438
    one_wire_id: one_wire_bus
    address: 0xd56dc98711646128
    update_interval: 60s
    shunt_resistance: 0.1 # resistance in Ohms
    temperature:
      name: "Battery Temperature"
    voltage:
      name: "Battery Input Voltage (VAD)"
      unit_of_measurement: "V"
    bus_voltage:
      name: "Bus Supply Voltage (VDD)"
      unit_of_measurement: "V"
    current:
      name: "Battery Current"
      unit_of_measurement: "A"

# 3. DS2408 Binary Sensor
binary_sensor:
  - platform: ds2408
    one_wire_id: one_wire_bus
    address: 0x2900000000000000
    channels:
      - pin: 0
        name: "Motion Sensor"
      - pin: 1
        name: "Door Sensor"

# 4. DS2423 Counter
sensor:
  - platform: ds2423
    one_wire_id: one_wire_bus
    address: 0x1D00000000000000
    counter_a:
      name: "Rain Gauge"
    counter_b:
      name: "Anemometer"
```

**Configuration Parameters**:
- `shunt_resistance` (DS2438): Resistor value for current calculation (default: 0.1 Ohm).
- `channels` (DS2408): List of pins (0-7) to map to binary sensors.
- `counter_a` / `counter_b` (DS2423): Specific configuration for the two counters.

## 📖 Usage in Home Assistant

Once configured and deployed:

1. The three sensors of the DS2438 will appear automatically in Home Assistant
2. Navigate to **Settings → Devices & Services → Entities**
3. Search for your device name (e.g., "Battery Temperature")
4. Add them to your dashboards, automations, or scripts
5. The sensors update at your configured interval

Example Home Assistant automation:
```yaml
automation:
  - trigger:
      platform: numeric_state
      entity_id: sensor.battery_temperature
      above: 50
    action:
      service: persistent_notification.create
      data:
        message: "Battery temperature is critically high!"
```

## 🔧 Technical Details

### DS2438
The DS2438 shares a single A/D converter for both voltage inputs. This component handles the switching logic automatically, measuring VAD, waiting for conversion, and continuing with VDD (Control Register 0x4E). Resistor voltage (current) is measured by enabling the IAD bit.

### DS2408
Communicates via Command 0xF0 (Read PIO Registers). The state of all 8 pins is read in one go.

### DS2423
Counters are stored in memory pages 14 (Counter A) and 15 (Counter B). Data is retrieved by "Read Memory & Counter" commands.

## 🐛 Troubleshooting

**Values stay at 0.00**: 
Check the ESPHome logs. If you see `Raw Data: 00 00 ...`, the chip might not be powered correctly:
- Ensure **VDD is properly connected** to 3.3V (not just relying on parasitic power)
- Verify the **1-Wire bus pull-up resistor** is correctly installed (4.7kΩ)
- Check that the **A/D conversion time is sufficient** (code uses safe defaults of 100ms)
- Confirm the **device address** is correct in your configuration

**Device not found on bus**:
- Verify GPIO pin number in your `one_wire` configuration
- Check for loose connections or cold solder joints
- Enable debug logging in ESPHome to see 1-Wire bus activity

**Compilation Error: `reset_`**: 
This component uses a C++ access trick to call the protected `reset_()` method of the OneWireBus, which is required for ESPHome versions >= 2025.12. Ensure you're using ESPHome 2025.12 or later.

## ⚠️ Known Limitations

- The component requires **ESPHome 2025.12 or newer** due to API changes
- Only one DS2438 address can be configured per instance (create multiple sensors for multiple devices)
- Requires a stable power supply; parasitic power mode is not recommended for reliable operation
- A/D conversion takes ~100ms; very high update intervals may cause delays

## 📊 Example Output

Once working, your Home Assistant will show:
```
Battery Temperature: 24.5 °C
Battery Input Voltage (VAD): 4.15 V
Bus Supply Voltage (VDD): 3.28 V
```

Example tracking of electrode and bus voltage without Home Assistant integration (only via MQTT, InfluxDB and Grafana)

![Voltages imported to Grafana via MQTT](DS2438voltages.jpeg)

**Analysis of Current Data during Heating Cycle**: The graph visualizes the electrochemical parameters of a hot water storage tank protected by a titanium impressed current anode. It plots three key metrics over time: the 1-wire Bus Supply Voltage (VDD), the Anode Potential (VAD) measured against the tank ground, and the Protection Current (I) flowing through the shunt (The shunt has a delibarately low resistance, so as not to impede the corrosion protection. Thus, the current is at the limit of what is directly measurable with the DS2438).
Bus Voltage Stability (VDD): The top line (VDD) remains flat and stable (approx. 3.3V). This confirms that the ESP32 and the sensor power supply are regulated correctly and are unaffected by the load changes during the heating process. It serves as a reliable reference baseline.
The Heating Event: A distinct event is visible where the water temperature rises (heating phase). This thermal change correlates directly with significant shifts in both the Anode Voltage (VAD) and the Protection Current. Water conductivity increases with temperature. Warmer water lowers the internal electrical resistance between the central titanium anode and the tank wall (cathode). Depending on the potentiostat's regulation logic, this typically results in an increase in Protection Current (to maintain the potential) or a shift in Anode Voltage (as the voltage drop across the water medium decreases).

Tracking this temperature dependency allows for alarms, when the corrosion inhibition device is not functional any more. Looking at the power (voltage x current) consumed, the dependency becomes even more apparent:

![Power imported to Grafana via MQTT](DS2438power.jpeg)

### Corrosion Protection Monitoring
**The Problem:** The electrical conductivity of water increases with temperature, causing the protection current (and thus power) to rise as the water heats up. This makes it difficult to detect failures (like a disconnected cable) just by looking at a static threshold, because the "normal" power at 20°C might be close to 0, while at 60°C it should be much higher.

**The Solution:** By analyzing data points (Temperature vs. Power), we established a quadratic regression model for the specific Titanium Electrode setup:
> **P µW⁻¹ ≈ 0.052 T² °C⁻² - 3.2 T °C⁻¹ + 55.8**

An ESPHome `binary_sensor` now continuously compares the **actual measured power** ($V_{AD} \cdot I_{AD}$) against this **expected power**. If the difference exceeds a safety threshold (e.g., 10 µW) for more than **12 hours**, an alarm is triggered. This reliably detects:
*   **Cable breaks / Potentiostat failure**: Power drops to 0, deviation becomes high.
*   **Electrode degradation**: Power drifts significantly from the expected curve.

See `warmwasserspeicher.yaml` for the implementation using lambda functions.

## 📝 License

This project is released under the **MIT License**. You are free to use, modify, and distribute this code, provided that you include the original license notice in any distribution.

For more details, see the [LICENSE](LICENSE) file in this repository.

## 🤝 Contributing

Found a bug or have an improvement? Feel free to open an issue or pull request!