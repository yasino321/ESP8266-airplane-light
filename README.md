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

## 🔌 Installation Guide

### 1. Prepare Arduino IDE
- **Install ESP8266 Board Support**:
    - Go to `File > Preferences`.
    - In "Additional Boards Manager URLs", paste: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
    - Go to `Tools > Board > Boards Manager`, search for `esp8266`, and install the latest version.
- **Install Required Libraries**:
    - Go to `Sketch > Include Library > Manage Libraries...`.
    - Search for and install:
        - `ESPAsyncWebServer` (and its dependency `ESPAsyncTCP`)
        - `WiFiManager` (by tzapu)
        - `ArduinoJson` (by Benoit Blanchon)
        - `OneButton` (by Matthias Hertel)
        - `AiEsp32RotaryEncoder` (by Igor Antolic)

### 2. Install LittleFS Upload Tool
The web files must be uploaded separately to the ESP8266's flash memory.
- Download the [ESP8266 LittleFS Filesystem Uploader](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases).
- Place the `.jar` file in your Arduino `tools` directory (e.g., `Documents/Arduino/tools/ESP8266LittleFS/tool/esp8266littlefs.jar`).
- Restart Arduino IDE. You should see "ESP8266 LittleFS Data Upload" under the `Tools` menu.

### 3. Wiring
- Connect your LEDs and Rotary Encoder according to the [Detailed Wiring & Pinout Guide](documents/wiring.md).
- **Crucial**: Ensure you use the resistors specified in the guide to avoid damaging your Wemos D1 Mini.

### 4. Upload Files and Code
1.  **Upload Web Assets**:
    - Connect your Wemos D1 Mini via USB.
    - Open `airbus_lights/airbus_lights.ino`.
    - Go to `Tools > ESP8266 LittleFS Data Upload`. **Wait for it to finish.**
2.  **Upload Sketch**:
    - Go to `Tools > Board` and select `LOLIN(WEMOS) D1 R2 & mini`.
    - Select the correct `Port`.
    - Click the **Upload** button (arrow icon).

## 📱 Usage

1.  **WiFi Setup**: On first boot, the ESP will create a hotspot named `Airbus-Lights-Setup`. Connect to it and enter your WiFi credentials.
2.  **Web Control**: Open your browser and go to `http://lights.local`.
3.  **Physical Control**:
    -   **Rotate**: Adjust master brightness.
    -   **Single Click**: Toggle Master Power.
    -   **Double Click**: Force all lights to 100% brightness.

## 📄 Documentation

-   [Detailed Wiring & Pinout](documents/wiring.md)
