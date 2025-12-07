# ADS1115
C++ I2C driver for the Texas Instruments ADS1115. 

## Prerequisites
* [cmake](https://cmake.org/)

## Connection

Connect your ADS1115 to your Raspberry Pi using the following pin connections:

### Power and I2C Connections
| ADS1115 Pin | Raspberry Pi Pin | Description |
|-------------|------------------|-------------|
| VDD         | Pin 1 (3.3V)     | Power supply |
| GND         | Pin 6 (GND)      | Ground |
| SCL         | Pin 5 (GPIO 3)   | I2C Clock |
| SDA         | Pin 3 (GPIO 2)   | I2C Data |

### I2C Address Configuration
The ADR pin determines the I2C address of the ADS1115:

| ADR Pin Connection | I2C Address |
|-------------------|-------------|
| ADR → GND         | 0x48 (default) |
| ADR → VDD         | 0x49 |
| ADR → SDA         | 0x4A |
| ADR → SCL         | 0x4B |

**Note:** Most breakout boards have ADR connected to GND by default, giving address 0x48.

### Analog Input Channels
The ADS1115 has 4 analog input channels:
- **AIN0** - Channel 0
- **AIN1** - Channel 1
- **AIN2** - Channel 2
- **AIN3** - Channel 3

Connect your analog signals to these pins. The ADC can measure:
- Single-ended: AIN0, AIN1, AIN2, or AIN3 (relative to GND)
- Differential: AIN0-AIN1, AIN0-AIN3, AIN1-AIN3, or AIN2-AIN3

### Amplifying Loadcell Signals

Loadcells typically produce very small differential signals (millivolts) that benefit from amplification before being read by the ADS1115. Here are recommended solutions:

#### Option 1: Instrumentation Amplifier (Recommended)

**INA125P** or **INA128** are excellent choices for loadcell amplification:

**INA125P Wiring:**
```
Loadcell E+ → INA125 Pin 2 (IN+)
Loadcell E- → INA125 Pin 3 (IN-)
Loadcell V+ → INA125 Pin 7 (V+)
Loadcell V- → INA125 Pin 4 (V-)
INA125 Pin 1 → Gain resistor (RG) → INA125 Pin 8
INA125 Pin 6 (OUT) → ADS1115 AIN0
INA125 Pin 5 (REF) → ADS1115 AIN1 (or GND for single-ended)
INA125 Pin 7 (V+) → 5V or 12V (check datasheet)
INA125 Pin 4 (V-) → GND
```

**Gain Calculation:**
- Gain = 4 + (60kΩ / RG)
- Common gains: 100 (RG = 620Ω), 200 (RG = 316Ω), 500 (RG = 123Ω), 1000 (RG = 60.4Ω)
- Higher gain = better resolution but smaller input range

**Alternative ICs:**
- **INA826** - Lower power, rail-to-rail output
- **INA128** - Similar to INA125, different package
- **AD620** - Lower cost alternative

#### Option 2: Op-Amp Differential Amplifier

For simpler applications, use an op-amp configured as a differential amplifier:

**LM358 or TL072 Configuration:**
```
Loadcell E+ → Op-Amp Non-inverting input (+)
Loadcell E- → Op-Amp Inverting input (-)
Output → ADS1115 AIN0
Gain = 1 + (R2/R1) where R1 and R2 are feedback resistors
```

**Note:** Op-amp solutions require careful resistor matching for good common-mode rejection.

#### Option 3: HX711 Module (Alternative)

If you prefer a complete solution with built-in amplification:
- **HX711** - 24-bit ADC with programmable gain amplifier (PGA)
- Gain options: 128x (Channel A) or 64x (Channel B)
- Uses GPIO pins instead of I2C
- See your existing `read_loadcell.py` for HX711 implementation

#### Recommended Setup for ADS1115:

1. **Loadcell → INA125P → ADS1115**
   - INA125P gain: 100-500 (adjust RG resistor)
   - ADS1115 FSR: ±2.048V or ±4.096V
   - ADS1115 channel: AIN0-AIN1 (differential)

2. **Typical Loadcell Signal:**
   - Full-scale output: 2-3mV per volt excitation
   - With 5V excitation: ~10-15mV full-scale
   - With gain of 200: ~2-3V full-scale (perfect for ADS1115 ±4.096V range)

3. **Power Supply:**
   - Use stable, low-noise power supply for loadcell excitation
   - Consider using ADS1115's internal reference or external voltage reference

### Enable I2C on Raspberry Pi
Make sure I2C is enabled on your Raspberry Pi:
```bash
sudo raspi-config
# Navigate to: Interface Options → I2C → Enable
```

Verify I2C is working and detect your ADS1115:
```bash
sudo apt-get install -y i2c-tools
i2cdetect -y 1
# You should see your ADS1115 address (e.g., 0x48) in the output
```

## Build
```
mkdir build
cd build
cmake ..
make
```
## Example
The repository comes with an example application *bin/ADS1115_example* to run on a Raspberry Pi with the address configured to 0x48 simply do:
```
./build/bin/ADS1115_example "/dev/i2c-1" 0x48
```

## Currently Supported ##
* Linux
* STM HAL (blocking mode)

## TODOs ##
* Add support for continous read mode. 

## Acknowledgments
See the list of [contributors](https://github.com/lokraszewski/ADS1115/contributors) who participated in this project.
