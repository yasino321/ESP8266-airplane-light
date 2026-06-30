# Airbus Lighting System Wiring Diagram

## Pin Mapping (Wemos D1 Mini)

| Pin | GPIO | Function | Description |
| :--- | :--- | :--- | :--- |
| **D1** | 5 | **Nav Lights** | PWM Output for Red, Green, White Nav LEDs |
| **D2** | 4 | **Strobe Wing** | PWM Output for Wingtip Strobes |
| **D5** | 14 | **Strobe Tail** | PWM Output for Tail Strobe |
| **D8** | 15 | **Beacon** | PWM Output for Red Beacons |
| **D6** | 12 | **Encoder CLK** | Rotary Encoder Clock |
| **D7** | 13 | **Encoder DT** | Rotary Encoder Data |
| **D3** | 0 | **Encoder SW** | Rotary Encoder Button (Switch) |
| **GND** | - | **Ground** | Common Ground for all components |
| **5V/3.3V** | - | **VCC** | Encoder VCC (use 3.3V) |

## Resistor Table (for 3.3V Logic)

All LEDs are connected in **parallel** to their respective pins. Each LED **must** have its own resistor to ensure even brightness and protect the ESP8266.

| Channel | LED Color | $ (Assumed) | Resistor (per LED) |
| :--- | :--- | :--- | :--- |
| **Nav (D1)** | Red | 2.0V | 330 $\Omega$ |
| **Nav (D1)** | Green | 3.2V | 27 $\Omega$ |
| **Nav (D1)** | White | 3.2V | 27 $\Omega$ |
| **Strobe Wing (D2)** | White | 3.2V | 22 $\Omega$ |
| **Strobe Tail (D5)** | White | 3.2V | 10 $\Omega$ |
| **Beacon (D8)** | Red | 2.0V | 220 $\Omega$ |

*Note: For White/Green LEDs, 27 $\Omega$ is very low. If they are too dim, you may try 10 $\Omega$. If they are too bright or the Wemos gets hot, increase to 47 $\Omega$.*

## Circuit Diagram (ASCII)

```
          Wemos D1 Mini
         +---------------+
         |            5V |--- (Not used for LEDs)
         |            G  |--- Common GND
         |            D4 |
         |            D3 |--- Encoder SW
         |            D2 |--- [Resistor] ---+---(LED Wing Strobe 1)--- GND
         |               |                  +---(LED Wing Strobe 2)--- GND
         |            D1 |--- [Resistor] ---+---(LED Nav Red)--------- GND
         |               |                  +---(LED Nav Green)------- GND
         |               |                  +---(LED Nav White)------- GND
         |            RX |
         |            TX |
         |               |
         |            D0 |
         |            D5 |--- [Resistor] -------(LED Tail Strobe)---- GND
         |            D6 |--- Encoder CLK
         |            D7 |--- Encoder DT
         |            D8 |--- [Resistor] ---+---(LED Beacon 1)------- GND
         |           3V3 |--- Encoder VCC   +---(LED Beacon 2)------- GND
         +---------------+
```
