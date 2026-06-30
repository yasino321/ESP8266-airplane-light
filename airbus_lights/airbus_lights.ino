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
  }
}

void loadConfig() {
  if (!LittleFS.exists("/config.json")) return;
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
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleStyle() {
  File file = LittleFS.open("/style.css", "r");
  server.streamFile(file, "text/css");
  file.close();
}

void handleScript() {
  File file = LittleFS.open("/script.js", "r");
  server.streamFile(file, "application/javascript");
  file.close();
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
  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/script.js", HTTP_GET, handleScript);
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
  if (!LittleFS.begin()) Serial.println("LittleFS Mount Failed");

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
