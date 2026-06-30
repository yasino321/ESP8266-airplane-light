#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AiEsp32RotaryEncoder.h>
#include <OneButton.h>

// --- Configuration ---
#define PIN_NAV         5  // D1
#define PIN_STROBE_WING 4  // D2
#define PIN_STROBE_TAIL 14 // D5
#define PIN_BEACON      15 // D8

#define PIN_ENCODER_CLK 12 // D6
#define PIN_ENCODER_DT  13 // D7
#define PIN_ENCODER_SW  0  // D3

#define PWM_RANGE 1023

// --- Embedded Web Interface (HTML/CSS/JS) ---
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Airbus Light Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background-color: #1a1a1a; color: #e0e0e0; display: flex; justify-content: center; padding: 20px; }
        .container { width: 100%; max-width: 400px; background: #2d2d2d; padding: 20px; border-radius: 10px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        h1, h3 { text-align: center; color: #00aaff; }
        .section { border-bottom: 1px solid #444; padding: 15px 0; }
        .section:last-child { border-bottom: none; }
        .control-row { display: flex; align-items: center; justify-content: space-between; margin: 10px 0; }
        input[type=range] { flex-grow: 1; margin-left: 15px; }
        .switch { position: relative; display: inline-block; width: 50px; height: 24px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 24px; }
        .slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .slider { background-color: #00aaff; }
        input:checked + .slider:before { transform: translateX(26px); }
    </style>
</head>
<body>
    <div class="container">
        <h1>Airbus Lights</h1>
        <div class="section">
            <div class="control-row"><span>Master Power</span><label class="switch"><input type="checkbox" id="masterOn" onchange="updateMaster()"><span class="slider"></span></label></div>
            <div class="control-row"><span>Master Brightness</span><input type="range" id="masterBrightness" min="0" max="255" oninput="updateMaster()"></div>
        </div>
        <div class="section">
            <h3>Navigation</h3>
            <div class="control-row"><label class="switch"><input type="checkbox" id="navState" onchange="updateChannel('nav')"><span class="slider"></span></label><input type="range" id="navBrightness" min="0" max="255" oninput="updateChannel('nav')"></div>
        </div>
        <div class="section">
            <h3>Strobes</h3>
            <div class="control-row"><span>Wing</span><label class="switch"><input type="checkbox" id="strobeWingState" onchange="updateChannel('strobeWing')"><span class="slider"></span></label><input type="range" id="strobeWingBrightness" min="0" max="255" oninput="updateChannel('strobeWing')"></div>
            <div class="control-row"><span>Tail</span><label class="switch"><input type="checkbox" id="strobeTailState" onchange="updateChannel('strobeTail')"><span class="slider"></span></label><input type="range" id="strobeTailBrightness" min="0" max="255" oninput="updateChannel('strobeTail')"></div>
        </div>
        <div class="section">
            <h3>Beacon</h3>
            <div class="control-row"><label class="switch"><input type="checkbox" id="beaconState" onchange="updateChannel('beacon')"><span class="slider"></span></label><input type="range" id="beaconBrightness" min="0" max="255" oninput="updateChannel('beacon')"></div>
            <div class="control-row"><span>Mode: <span id="beaconModeText">Pulse</span></span><label class="switch"><input type="checkbox" id="beaconPulseMode" onchange="updateBeaconMode()"><span class="slider"></span></label></div>
        </div>
    </div>
    <script>
        function updateMaster() { sendUpdate('/api/master', { masterOn: document.getElementById('masterOn').checked, masterBrightness: parseInt(document.getElementById('masterBrightness').value) }); }
        function updateChannel(name) { sendUpdate('/api/channel/' + name, { state: document.getElementById(name + 'State').checked, brightness: parseInt(document.getElementById(name + 'Brightness').value) }); }
        function updateBeaconMode() { let isPulse = document.getElementById('beaconPulseMode').checked; document.getElementById('beaconModeText').innerText = isPulse ? 'Pulse' : 'Blink'; sendUpdate('/api/beaconMode', { pulse: isPulse }); }
        function sendUpdate(url, data) { fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(data) }); }
        function fetchState() {
            fetch('/api/state').then(r => r.json()).then(s => {
                document.getElementById('masterOn').checked = s.masterOn; document.getElementById('masterBrightness').value = s.masterBrightness;
                document.getElementById('navState').checked = s.nav.state; document.getElementById('navBrightness').value = s.nav.brightness;
                document.getElementById('strobeWingState').checked = s.strobeWing.state; document.getElementById('strobeWingBrightness').value = s.strobeWing.brightness;
                document.getElementById('strobeTailState').checked = s.strobeTail.state; document.getElementById('strobeTailBrightness').value = s.strobeTail.brightness;
                document.getElementById('beaconState').checked = s.beacon.state; document.getElementById('beaconBrightness').value = s.beacon.brightness;
                document.getElementById('beaconPulseMode').checked = s.beaconPulseMode; document.getElementById('beaconModeText').innerText = s.beaconPulseMode ? 'Pulse' : 'Blink';
            });
        }
        fetchState(); setInterval(fetchState, 2000);
    </script>
</body>
</html>
)=====";

// --- State Variables ---
struct LightChannel {
  bool state;
  int brightness; // 0-255
};

LightChannel nav = {true, 255};
LightChannel strobeWing = {true, 255};
LightChannel strobeTail = {true, 255};
LightChannel beacon = {true, 255};

bool masterOn = true;
int masterBrightness = 255; // 0-255
bool beaconPulseMode = true; // true = pulse, false = blink

unsigned long lastSave = 0;
bool needsSave = false;

// --- Objects ---
ESP8266WebServer server(80);
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(PIN_ENCODER_DT, PIN_ENCODER_CLK, PIN_ENCODER_SW, -1, 4);
OneButton button(PIN_ENCODER_SW, true);

// --- Functions ---

void updatePwm(int pin, bool chState, int chBrightness, int value) {
  if (!masterOn || !chState) {
    analogWrite(pin, 0);
    return;
  }
  int target = (chBrightness * masterBrightness * value) / (255 * 255);
  int pwm = map(target, 0, 255, 0, PWM_RANGE);
  analogWrite(pin, pwm);
}

void handleStrobes() {
  unsigned long now = millis();
  unsigned long wingCycle = now % 1200;
  bool wingOn = ((wingCycle > 0 && wingCycle < 30) || (wingCycle > 150 && wingCycle < 180));
  updatePwm(PIN_STROBE_WING, strobeWing.state, strobeWing.brightness, wingOn ? 255 : 0);

  unsigned long tailCycle = (now + 400) % 1200;
  bool tailOn = ((tailCycle > 0 && tailCycle < 30) || (tailCycle > 150 && tailCycle < 180));
  updatePwm(PIN_STROBE_TAIL, strobeTail.state, strobeTail.brightness, tailOn ? 255 : 0);
}

void handleBeacon() {
  unsigned long now = millis();
  if (beaconPulseMode) {
    float phase = (now % 1500) / 1500.0;
    int val = (sin(phase * 2.0 * PI - PI / 2.0) + 1.0) * 127.5;
    updatePwm(PIN_BEACON, beacon.state, beacon.brightness, val);
  } else {
    bool val = (now % 1000) < 100;
    updatePwm(PIN_BEACON, beacon.state, beacon.brightness, val ? 255 : 0);
  }
}

void handleNav() {
  updatePwm(PIN_NAV, nav.state, nav.brightness, 255);
}

// --- Persistence ---
void saveConfig() {
  StaticJsonDocument<512> doc;
  doc["masterOn"] = masterOn;
  doc["masterBrightness"] = masterBrightness;
  doc["beaconPulseMode"] = beaconPulseMode;
  doc["nav"]["state"] = nav.state;
  doc["nav"]["brightness"] = nav.brightness;
  doc["strobeWing"]["state"] = strobeWing.state;
  doc["strobeWing"]["brightness"] = strobeWing.brightness;
  doc["strobeTail"]["state"] = strobeTail.state;
  doc["strobeTail"]["brightness"] = strobeTail.brightness;
  doc["beacon"]["state"] = beacon.state;
  doc["beacon"]["brightness"] = beacon.brightness;

  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("Config saved.");
  }
}

void loadConfig() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("No config file found. Using defaults.");
    return;
  }
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return;
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (!error) {
    masterOn = doc["masterOn"];
    masterBrightness = doc["masterBrightness"];
    beaconPulseMode = doc["beaconPulseMode"];
    nav.state = doc["nav"]["state"];
    nav.brightness = doc["nav"]["brightness"];
    strobeWing.state = doc["strobeWing"]["state"];
    strobeWing.brightness = doc["strobeWing"]["brightness"];
    strobeTail.state = doc["strobeTail"]["state"];
    strobeTail.brightness = doc["strobeTail"]["brightness"];
    beacon.state = doc["beacon"]["state"];
    beacon.brightness = doc["beacon"]["brightness"];
    rotaryEncoder.setEncoderValue(masterBrightness);
    Serial.println("Config loaded.");
  }
  configFile.close();
}

// --- Encoder Handlers ---
void readEncoder() {
  if (rotaryEncoder.encoderChanged()) {
    masterBrightness = rotaryEncoder.readEncoder();
    needsSave = true;
    lastSave = millis();
  }
}

void handleClick() {
  masterOn = !masterOn;
  needsSave = true;
  lastSave = millis();
}

void handleDoubleClick() {
  masterBrightness = 255;
  rotaryEncoder.setEncoderValue(255);
  nav.brightness = 255;
  strobeWing.brightness = 255;
  strobeTail.brightness = 255;
  beacon.brightness = 255;
  masterOn = true;
  needsSave = true;
  lastSave = millis();
}

// --- Web Server ---
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleGetState() {
  StaticJsonDocument<512> doc;
  doc["masterOn"] = masterOn;
  doc["masterBrightness"] = masterBrightness;
  doc["beaconPulseMode"] = beaconPulseMode;
  doc["nav"]["state"] = nav.state;
  doc["nav"]["brightness"] = nav.brightness;
  doc["strobeWing"]["state"] = strobeWing.state;
  doc["strobeWing"]["brightness"] = strobeWing.brightness;
  doc["strobeTail"]["state"] = strobeTail.state;
  doc["strobeTail"]["brightness"] = strobeTail.brightness;
  doc["beacon"]["state"] = beacon.state;
  doc["beacon"]["brightness"] = beacon.brightness;
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePostMaster() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  masterOn = doc["masterOn"];
  masterBrightness = doc["masterBrightness"];
  rotaryEncoder.setEncoderValue(masterBrightness);
  needsSave = true; lastSave = millis();
  server.send(200);
}

void handlePostNav() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  nav.state = doc["state"];
  nav.brightness = doc["brightness"];
  needsSave = true; lastSave = millis();
  server.send(200);
}

void handlePostStrobeWing() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  strobeWing.state = doc["state"];
  strobeWing.brightness = doc["brightness"];
  needsSave = true; lastSave = millis();
  server.send(200);
}

void handlePostStrobeTail() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  strobeTail.state = doc["state"];
  strobeTail.brightness = doc["brightness"];
  needsSave = true; lastSave = millis();
  server.send(200);
}

void handlePostBeacon() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  beacon.state = doc["state"];
  beacon.brightness = doc["brightness"];
  needsSave = true; lastSave = millis();
  server.send(200);
}

void handlePostBeaconMode() {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  beaconPulseMode = doc["pulse"];
  needsSave = true; lastSave = millis();
  server.send(200);
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleGetState);
  server.on("/api/master", HTTP_POST, handlePostMaster);
  server.on("/api/channel/nav", HTTP_POST, handlePostNav);
  server.on("/api/channel/strobeWing", HTTP_POST, handlePostStrobeWing);
  server.on("/api/channel/strobeTail", HTTP_POST, handlePostStrobeTail);
  server.on("/api/channel/beacon", HTTP_POST, handlePostBeacon);
  server.on("/api/beaconMode", HTTP_POST, handlePostBeaconMode);
  server.begin();
}

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS - automatically format if fresh board
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed. Formatting...");
    LittleFS.format();
    LittleFS.begin();
  }

  pinMode(PIN_NAV, OUTPUT);
  pinMode(PIN_STROBE_WING, OUTPUT);
  pinMode(PIN_STROBE_TAIL, OUTPUT);
  pinMode(PIN_BEACON, OUTPUT);
  analogWriteRange(PWM_RANGE);

  rotaryEncoder.begin();
  rotaryEncoder.setup([] { rotaryEncoder.readEncoder_ISR(); }, [] {  });
  rotaryEncoder.setBoundaries(0, 255, false);
  loadConfig();

  button.attachClick(handleClick);
  button.attachDoubleClick(handleDoubleClick);

  WiFiManager wm;
  wm.autoConnect("Airbus-Lights-Setup");

  if (MDNS.begin("lights")) Serial.println("MDNS started: http://lights.local");
  setupWebServer();
}

void loop() {
  MDNS.update();
  server.handleClient();
  button.tick();
  readEncoder();
  handleNav();
  handleStrobes();
  handleBeacon();
  if (needsSave && millis() - lastSave > 5000) {
    saveConfig();
    needsSave = false;
  }
  delay(10);
}
