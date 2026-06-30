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

// --- Embedded Web Interface ---
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><title>Airbus Light Control</title><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial,sans-serif;background-color:#1a1a1a;color:#e0e0e0;display:flex;justify-content:center;padding:20px}
.container{width:100%;max-width:400px;background:#2d2d2d;padding:20px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,.5)}
h1,h3{text-align:center;color:#00aaff}.section{border-bottom:1px solid #444;padding:15px 0}.section:last-child{border-bottom:none}
.control-row{display:flex;align-items:center;justify-content:space-between;margin:10px 0}input[type=range]{flex-grow:1;margin-left:15px}
.switch{position:relative;display:inline-block;width:50px;height:24px}.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;border-radius:24px}
.slider:before{position:absolute;content:"";height:16px;width:16px;left:4px;bottom:4px;background-color:#fff;transition:.4s;border-radius:50%}
input:checked+.slider{background-color:#00aaff}input:checked+.slider:before{transform:translateX(26px)}
</style></head><body><div class="container"><h1>Airbus Lights</h1><div class="section">
<div class="control-row"><span>Master Power</span><label class="switch"><input type="checkbox" id="masterOn" onchange="updateMaster()"><span class="slider"></span></label></div>
<div class="control-row"><span>Master Brightness</span><input type="range" id="masterBrightness" min="0" max="255" oninput="updateMaster()"></div></div>
<div class="section"><h3>Navigation</h3><div class="control-row"><label class="switch"><input type="checkbox" id="navState" onchange="updateChannel('nav')"><span class="slider"></span></label>
<input type="range" id="navBrightness" min="0" max="255" oninput="updateChannel('nav')"></div></div>
<div class="section"><h3>Strobes</h3><div class="control-row"><span>Wing</span><label class="switch"><input type="checkbox" id="strobeWingState" onchange="updateChannel('strobeWing')"><span class="slider"></span></label>
<input type="range" id="strobeWingBrightness" min="0" max="255" oninput="updateChannel('strobeWing')"></div>
<div class="control-row"><span>Tail</span><label class="switch"><input type="checkbox" id="strobeTailState" onchange="updateChannel('strobeTail')"><span class="slider"></span></label>
<input type="range" id="strobeTailBrightness" min="0" max="255" oninput="updateChannel('strobeTail')"></div></div>
<div class="section"><h3>Beacon</h3><div class="control-row"><label class="switch"><input type="checkbox" id="beaconState" onchange="updateChannel('beacon')"><span class="slider"></span></label>
<input type="range" id="beaconBrightness" min="0" max="255" oninput="updateChannel('beacon')"></div>
<div class="control-row"><span>Mode: <span id="beaconModeText">Pulse</span></span><label class="switch"><input type="checkbox" id="beaconPulseMode" onchange="updateBeaconMode()"><span class="slider"></span></label></div></div></div>
<script>
function updateMaster(){sendUpdate('/api/master',{masterOn:document.getElementById('masterOn').checked,masterBrightness:parseInt(document.getElementById('masterBrightness').value)})}
function updateChannel(n){sendUpdate('/api/channel/'+n,{state:document.getElementById(n+'State').checked,brightness:parseInt(document.getElementById(n+'Brightness').value)})}
function updateBeaconMode(){let p=document.getElementById('beaconPulseMode').checked;document.getElementById('beaconModeText').innerText=p?'Pulse':'Blink';sendUpdate('/api/beaconMode',{pulse:p})}
function sendUpdate(u,d){fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})}
function fetchState(){fetch('/api/state').then(r=>r.json()).then(s=>{
document.getElementById('masterOn').checked=s.masterOn;document.getElementById('masterBrightness').value=s.masterBrightness;
document.getElementById('navState').checked=s.nav.state;document.getElementById('navBrightness').value=s.nav.brightness;
document.getElementById('strobeWingState').checked=s.strobeWing.state;document.getElementById('strobeWingBrightness').value=s.strobeWing.brightness;
document.getElementById('strobeTailState').checked=s.strobeTail.state;document.getElementById('strobeTailBrightness').value=s.strobeTail.brightness;
document.getElementById('beaconState').checked=s.beacon.state;document.getElementById('beaconBrightness').value=s.beacon.brightness;
document.getElementById('beaconPulseMode').checked=s.beaconPulseMode;document.getElementById('beaconModeText').innerText=s.beaconPulseMode?'Pulse':'Blink';})}
fetchState();setInterval(fetchState,2000);
</script></body></html>
)=====";

// --- State Variables ---
struct LightChannel { bool state; int brightness; };
LightChannel nav={true,255}, strobeWing={true,255}, strobeTail={true,255}, beacon={true,255};
bool masterOn=true; int masterBrightness=255; bool beaconPulseMode=true;
unsigned long lastSave=0; bool needsSave=false;

// --- Objects ---
ESP8266WebServer server(80);
AiEsp32RotaryEncoder rotaryEncoder(PIN_ENCODER_DT, PIN_ENCODER_CLK, -1, -1, 4); // Removed SW from encoder object
OneButton button(PIN_ENCODER_SW, true);

// --- Functions ---
void IRAM_ATTR readEncoderISR() { rotaryEncoder.readEncoder_ISR(); }

void updatePwm(int pin, bool chState, int chBrightness, int value) {
  if (!masterOn || !chState) { analogWrite(pin, 0); return; }
  int target = (chBrightness * masterBrightness * value) / (255 * 255);
  analogWrite(pin, map(target, 0, 255, 0, PWM_RANGE));
}

void handleStrobes() {
  unsigned long wingCycle = millis() % 1200;
  bool wingOn = ((wingCycle > 0 && wingCycle < 30) || (wingCycle > 150 && wingCycle < 180));
  updatePwm(PIN_STROBE_WING, strobeWing.state, strobeWing.brightness, wingOn ? 255 : 0);
  unsigned long tailCycle = (millis() + 400) % 1200;
  bool tailOn = ((tailCycle > 0 && tailCycle < 30) || (tailCycle > 150 && tailCycle < 180));
  updatePwm(PIN_STROBE_TAIL, strobeTail.state, strobeTail.brightness, tailOn ? 255 : 0);
}

void handleBeacon() {
  if (beaconPulseMode) {
    float phase = (millis() % 1500) / 1500.0;
    updatePwm(PIN_BEACON, beacon.state, beacon.brightness, (sin(phase * 2.0 * PI - PI / 2.0) + 1.0) * 127.5);
  } else {
    updatePwm(PIN_BEACON, beacon.state, beacon.brightness, (millis() % 1000 < 100) ? 255 : 0);
  }
}

void saveConfig() {
  File f = LittleFS.open("/config.json", "w");
  if (f) {
    StaticJsonDocument<512> doc;
    doc["masterOn"]=masterOn; doc["masterBrightness"]=masterBrightness; doc["beaconPulseMode"]=beaconPulseMode;
    doc["nav"]["state"]=nav.state; doc["nav"]["brightness"]=nav.brightness;
    doc["strobeWing"]["state"]=strobeWing.state; doc["strobeWing"]["brightness"]=strobeWing.brightness;
    doc["strobeTail"]["state"]=strobeTail.state; doc["strobeTail"]["brightness"]=strobeTail.brightness;
    doc["beacon"]["state"]=beacon.state; doc["beacon"]["brightness"]=beacon.brightness;
    serializeJson(doc, f); f.close();
    Serial.println("Config Saved");
  }
}

void loadConfig() {
  if (!LittleFS.exists("/config.json")) return;
  File f = LittleFS.open("/config.json", "r");
  if (!f) return;
  StaticJsonDocument<512> doc;
  if (!deserializeJson(doc, f)) {
    masterOn=doc["masterOn"]; masterBrightness=doc["masterBrightness"]; beaconPulseMode=doc["beaconPulseMode"];
    nav.state=doc["nav"]["state"]; nav.brightness=doc["nav"]["brightness"];
    strobeWing.state=doc["strobeWing"]["state"]; strobeWing.brightness=doc["strobeWing"]["brightness"];
    strobeTail.state=doc["strobeTail"]["state"]; strobeTail.brightness=doc["strobeTail"]["brightness"];
    beacon.state=doc["beacon"]["state"]; beacon.brightness=doc["beacon"]["brightness"];
    rotaryEncoder.setEncoderValue(masterBrightness);
  }
  f.close();
}

// --- Handlers ---
void handleClick() { masterOn=!masterOn; needsSave=true; lastSave=millis(); Serial.println("Toggle Power"); }
void handleDoubleClick() { masterBrightness=255; rotaryEncoder.setEncoderValue(255); nav.brightness=255; strobeWing.brightness=255; strobeTail.brightness=255; beacon.brightness=255; masterOn=true; needsSave=true; lastSave=millis(); Serial.println("Reset 100%"); }

void handleState() {
  StaticJsonDocument<512> doc;
  doc["masterOn"]=masterOn; doc["masterBrightness"]=masterBrightness; doc["beaconPulseMode"]=beaconPulseMode;
  doc["nav"]["state"]=nav.state; doc["nav"]["brightness"]=nav.brightness;
  doc["strobeWing"]["state"]=strobeWing.state; doc["strobeWing"]["brightness"]=strobeWing.brightness;
  doc["strobeTail"]["state"]=strobeTail.state; doc["strobeTail"]["brightness"]=strobeTail.brightness;
  doc["beacon"]["state"]=beacon.state; doc["beacon"]["brightness"]=beacon.brightness;
  String s; serializeJson(doc, s); server.send(200, "application/json", s);
}

void handlePost(void (*updateFunc)(StaticJsonDocument<200>&)) {
  StaticJsonDocument<200> doc;
  if (!deserializeJson(doc, server.arg("plain"))) {
    updateFunc(doc); needsSave=true; lastSave=millis(); server.send(200);
  } else { server.send(400); }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- Airbus Lights Booting ---");

  pinMode(PIN_NAV, OUTPUT); pinMode(PIN_STROBE_WING, OUTPUT); pinMode(PIN_STROBE_TAIL, OUTPUT); pinMode(PIN_BEACON, OUTPUT);
  analogWriteRange(PWM_RANGE);

  if (!LittleFS.begin()) { Serial.println("Formatting FS..."); LittleFS.format(); LittleFS.begin(); }
  loadConfig();

  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR, []{});
  rotaryEncoder.setBoundaries(0, 255, false);
  rotaryEncoder.setEncoderValue(masterBrightness);

  button.attachClick(handleClick);
  button.attachDoubleClick(handleDoubleClick);

  WiFiManager wm;
  wm.setDebugOutput(true);
  Serial.println("Starting WiFiManager...");
  if (!wm.autoConnect("Airbus-Lights-Setup")) { Serial.println("Failed to connect"); ESP.restart(); }
  Serial.println("WiFi Connected!");

  MDNS.begin("lights");
  server.on("/", [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/state", handleState);
  server.on("/api/master", [](){ handlePost([](StaticJsonDocument<200>& d){ masterOn=d["masterOn"]; masterBrightness=d["masterBrightness"]; rotaryEncoder.setEncoderValue(masterBrightness); }); });
  server.on("/api/channel/nav", [](){ handlePost([](StaticJsonDocument<200>& d){ nav.state=d["state"]; nav.brightness=d["brightness"]; }); });
  server.on("/api/channel/strobeWing", [](){ handlePost([](StaticJsonDocument<200>& d){ strobeWing.state=d["state"]; strobeWing.brightness=d["brightness"]; }); });
  server.on("/api/channel/strobeTail", [](){ handlePost([](StaticJsonDocument<200>& d){ strobeTail.state=d["state"]; strobeTail.brightness=d["brightness"]; }); });
  server.on("/api/channel/beacon", [](){ handlePost([](StaticJsonDocument<200>& d){ beacon.state=d["state"]; beacon.brightness=d["brightness"]; }); });
  server.on("/api/beaconMode", [](){ handlePost([](StaticJsonDocument<200>& d){ beaconPulseMode=d["pulse"]; }); });
  server.begin();
}

void loop() {
  MDNS.update(); server.handleClient(); button.tick();
  if (rotaryEncoder.encoderChanged()) { masterBrightness = rotaryEncoder.readEncoder(); needsSave=true; lastSave=millis(); }
  updatePwm(PIN_NAV, nav.state, nav.brightness, 255); handleStrobes(); handleBeacon();
  if (needsSave && millis() - lastSave > 5000) { saveConfig(); needsSave=false; }
  delay(5);
}
