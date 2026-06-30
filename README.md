# Airbus Aircraft Lighting System (Wemos D1 Mini)

This project implements a realistic Airbus-inspired lighting system for RC aircraft or models using a Wemos D1 Mini (ESP8266). It features physical control via a rotary encoder and a modern web interface for remote management.

## 🚀 Features

-   **Airbus Lighting Patterns**:
    -   **Navigation Lights**: Constant ON (Red, Green, White).
    -   **Strobe Lights**: Dual-channel (Wingtip and Tail) with the iconic Airbus double-flash pattern and timing offset.
    -   **Beacon Lights**: Red beacons with toggleable **Pulse** (fade in/out) or **Blink** modes.
-   **Dual Control Modes**:
    -   **Physical**: Rotary encoder for global brightness and power.
    -   **Web App**: Local network dashboard (accessible via `http://lights.local`) for individual channel control.
-   **Intelligent Logic**:
    -   **Master Brightness Scaling**: Turning the encoder dims all channels proportionally based on their individual settings.
    -   **Quick Override**: Double-click the encoder button to instantly set all lights to 100% brightness.
    -   **Persistence**: Settings are saved to the ESP8266's internal flash memory and restored on boot.
-   **Easy Setup**: Includes `WiFiManager` for easy WiFi configuration without hardcoding credentials.

## 🛠 Hardware Requirements

-   Wemos D1 Mini (ESP8266)
-   Rotary Encoder (e.g., KY-040)
-   5mm LEDs (Red, Green, White)
-   Resistors (See [Wiring Documentation](documents/wiring.md) for values)

## 📦 Software Dependencies

Ensure you have the following libraries installed in your Arduino IDE:

-   `ESPAsyncWebServer`
-   `ESPAsyncTCP`
-   `WiFiManager`
-   `ArduinoJson`
-   `OneButton`
-   `AiEsp32RotaryEncoder`

## 🔌 Installation

1.  **Wiring**: Follow the detailed [Wiring Diagram and Resistor Table](documents/wiring.md).
2.  **Flash Filesystem**:
    -   Upload the contents of the `airbus_lights/data` folder to the Wemos D1 Mini using the **LittleFS Data Upload** tool.
3.  **Upload Code**:
    -   Open `airbus_lights/airbus_lights.ino` in the Arduino IDE.
    -   Select "Wemos D1 R2 & mini" as the board.
    -   Upload the sketch.

## 📱 Usage

1.  **WiFi Setup**: On first boot, the ESP will create a hotspot named `Airbus-Lights-Setup`. Connect to it and enter your WiFi credentials.
2.  **Web Control**: Open your browser and go to `http://lights.local`.
3.  **Physical Control**:
    -   **Rotate**: Adjust master brightness.
    -   **Single Click**: Toggle Master Power.
    -   **Double Click**: Force all lights to 100% brightness.

## 📄 Documentation

-   [Detailed Wiring & Pinout](documents/wiring.md)
