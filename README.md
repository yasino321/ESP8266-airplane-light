# Airbus Aircraft Lighting System (Wemos D1 Mini)

This project implements a realistic Airbus-inspired lighting system for RC aircraft or models using a Wemos D1 Mini (ESP8266).

## 🚀 What's New?
The web interface is now **embedded directly into the code**. You no longer need to upload separate files to the flash memory. Just upload the code, and it's ready to go!

## 💾 What is LittleFS?
**LittleFS** is a tiny "hard drive" inside your Wemos D1 Mini. We use it to **persist your settings**.
-   **Why use it?** If you adjust the brightness or change the beacon mode via the web app, LittleFS saves those choices. When you unplug the aircraft and plug it back in, the lights will return exactly to your last settings.
-   **Fresh Boards**: The code is designed for brand-new boards. If LittleFS isn't ready, the ESP8266 will automatically format it on the first boot.

## 📱 Features
-   **Airbus Patterns**: Solid Nav, Double-flash Strobes (with wing/tail offset), and Pulse/Blink Beacon.
-   **Dual Control**: Physical rotary encoder + Local Web App (`http://lights.local`).
-   **Intelligent Dimming**: Master brightness scales individual settings.
-   **100% Override**: Double-click the encoder to set all lights to full power.

## 🛠 Hardware Requirements
-   Wemos D1 Mini
-   Rotary Encoder
-   5mm LEDs (Red, Green, White) + Resistors (See [Wiring Guide](documents/wiring.md))

## 🔌 Quick Installation
1.  **Arduino IDE Setup**:
    -   Install the ESP8266 board support (Preferences > Boards Manager).
    -   Install these libraries: `WiFiManager`, `ArduinoJson`, `OneButton`, `AiEsp32RotaryEncoder`.
2.  **Wiring**: Follow the [Wiring & Resistor Guide](documents/wiring.md).
3.  **Upload**: Open `airbus_lights/airbus_lights.ino`, select **LOLIN(WEMOS) D1 R2 & mini**, and click Upload.

## 📱 Usage
1.  Connect to the WiFi hotspot `Airbus-Lights-Setup` to enter your home WiFi details.
2.  Go to `http://lights.local` on your phone/computer to control the lights!
