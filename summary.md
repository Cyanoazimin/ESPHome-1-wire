### @file ds2438.h
**Role:** Core C++ Logic for DS2438 Custom Component.
**Key Features:**
- **Dual-Voltage Multiplexing:** Implements sequential switching between VDD (Bit AD=1) and VAD (Bit AD=0) using the 0x4E Write Scratchpad command.
- **Current Sensing:** Enables IAD (Bit 0) for continuous current measurement across a shunt resistor.
- **ESPHome 2025.12+ Workaround:** Includes a helper class to access the protected `reset_()` method of `OneWireBus` via pointer casting.
- **Stability:** Implements 100ms delays for A/D conversion and 10ms for memory recalls (0xB8).

### @file sensor.py
**Role:** ESPHome Python Bindings and Schema Definition.
**Key Features:**
- **Dynamic Configuration:** Supports optional `temperature`, `voltage` (VAD), `bus_voltage` (VDD), and `current` sensors.
- **Parameterized Constructor:** Passes the `shunt_resistance` (default 0.1 Ohm) from YAML to the C++ class.
- **Modern Imports:** Updated for ESPHome 2025.x, avoiding deprecated `device_class` imports from `esphome.const`.

### @file warmwasserspeicher.yaml
**Role:** Deployment Configuration.
**Context:** Monitoring a hot water storage tank with an active titanium anode.
- **VDD:** 3.3V stable bus supply.
- **VAD:** ~2.7V fluctuating electrochemical potential (Anode).
- **Current:** Calculated via shunt to monitor corrosion protection efficiency.