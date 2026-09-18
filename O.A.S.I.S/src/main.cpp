/*
  O.A.S.I.S. - ESP32 Wokwi Simulation
  -----------------------------------
  Simulates:
  - pH sensor          -> potentiometer on GPIO34
  - Turbidity sensor   -> potentiometer on GPIO35
  - 3 operating modes  -> MCK / OFF / DRINK (firmware-latched buttons)
  - 20x4 I2C LCD       -> SDA 21, SCL 22
  - 4 relay outputs:
      GPIO18 -> Hose Pump
      GPIO19 -> MCK Valve
      GPIO23 -> RO Pump
      GPIO5  -> Drink Valve

  IMPORTANT:
  The potentiometer conversions below are SIMULATION mappings only.
  They are NOT calibration equations for the real pH or turbidity sensors.
*/

#include <Arduino.h>
#include <DNSServer.h>
#include <Wire.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

#include "dashboard.h"

// =========================================================
// PIN DEFINITIONS
// =========================================================
constexpr uint8_t PIN_PH        = 34;
constexpr uint8_t PIN_TURBIDITY = 35;

constexpr uint8_t PIN_MODE_MCK   = 13;
constexpr uint8_t PIN_MODE_OFF   = 14;
constexpr uint8_t PIN_MODE_DRINK = 27;

constexpr uint8_t PIN_RELAY_HOSE  = 18;
constexpr uint8_t PIN_RELAY_MCK   = 19;
constexpr uint8_t PIN_RELAY_RO    = 23;
constexpr uint8_t PIN_RELAY_DRINK = 5;

constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;
LiquidCrystal_I2C lcd(0x27, 20, 4);

constexpr char AP_SSID[] = "OASIS-Control";
constexpr char AP_PASSWORD[] = "oasis2026";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
DNSServer dnsServer;
WebServer webServer(80);
bool stationAddressReported = false;

// diagram.json uses PNP active-low relay modules (verified in simulation).
constexpr uint8_t RELAY_ON  = LOW;
constexpr uint8_t RELAY_OFF = HIGH;

// =========================================================
// O.A.S.I.S. WATER QUALITY LIMITS
// =========================================================
constexpr float PH_MIN = 6.5f;
constexpr float PH_MAX = 8.5f;

// Team simulation criterion: <= 5 NTU; < 0.3 NTU is the preferred target.
// This classification alone does not establish actual drinking-water safety.
constexpr float DRINK_MAX_NTU = 5.0f;
constexpr float MCK_MAX_NTU = 10.0f;

// =========================================================
// SIMULATION RANGES
// =========================================================
// These are only convenient mappings for the Wokwi potentiometers.
// Replace with real calibration equations on the physical prototype.
constexpr float SIM_PH_MIN  = 0.0f;
constexpr float SIM_PH_MAX  = 14.0f;
constexpr float SIM_NTU_MIN = 0.0f;
constexpr float SIM_NTU_MAX = 50.0f;

// =========================================================
// TYPES
// =========================================================
enum class UserMode {
  MCK,
  OFF,
  DRINK
};

enum class WaterClass {
  UNSAFE,
  MCK_ONLY,
  DRINKABLE
};

struct SensorData {
  int phRaw;
  int turbidityRaw;
  float ph;
  float turbidity;
};

struct Outputs {
  bool hosePump;
  bool mckValve;
  bool roPump;
  bool drinkValve;
};

// =========================================================
// GLOBAL STATE
// =========================================================
UserMode currentMode = UserMode::OFF;
SensorData sensorData{};
WaterClass waterClass = WaterClass::UNSAFE;
Outputs outputs{};

unsigned long lastSensorUpdate = 0;
unsigned long lastSerialUpdate = 0;
unsigned long buttonChangedAt = 0;
uint8_t candidateButtons = 0;
uint8_t stableButtons = 0;

constexpr unsigned long SENSOR_INTERVAL_MS = 250;
constexpr unsigned long SERIAL_INTERVAL_MS = 1000;
// Diagram buttons have bounce disabled; accept even short simulated clicks.
// Enable a suitable debounce interval when using physical contacts.
constexpr unsigned long BUTTON_DEBOUNCE_MS = 0;

// =========================================================
// HELPERS
// =========================================================
float mapFloat(int value, int inMin, int inMax, float outMin, float outMax) {
  return (float)(value - inMin) * (outMax - outMin) /
             (float)(inMax - inMin) + outMin;
}

const char* modeToString(UserMode mode) {
  switch (mode) {
    case UserMode::MCK:   return "MCK";
    case UserMode::OFF:   return "OFF";
    case UserMode::DRINK: return "DRINK";
  }
  return "?";
}

const char* waterClassToString(WaterClass state) {
  switch (state) {
    case WaterClass::DRINKABLE: return "DRINKABLE";
    case WaterClass::MCK_ONLY:  return "MCK ONLY";
    case WaterClass::UNSAFE:    return "UNSAFE";
  }
  return "?";
}

void printLCDLine(uint8_t row, String text) {
  if (text.length() > 20) {
    text = text.substring(0, 20);
  }
  while (text.length() < 20) {
    text += ' ';
  }
  lcd.setCursor(0, row);
  lcd.print(text);
}

// =========================================================
// SENSOR SIMULATION
// =========================================================
void readSensors() {
  sensorData.phRaw        = analogRead(PIN_PH);
  sensorData.turbidityRaw = analogRead(PIN_TURBIDITY);

  sensorData.ph = mapFloat(
      sensorData.phRaw, 0, 4095, SIM_PH_MIN, SIM_PH_MAX);

  sensorData.turbidity = mapFloat(
      sensorData.turbidityRaw, 0, 4095, SIM_NTU_MIN, SIM_NTU_MAX);
}

// =========================================================
// WATER CLASSIFICATION
// =========================================================
WaterClass classifyWater(const SensorData& d) {
  const bool phOK = d.ph >= PH_MIN && d.ph <= PH_MAX;

  const bool drinkable =
      phOK &&
      d.turbidity <= DRINK_MAX_NTU;

  const bool mckAcceptable =
      phOK &&
      d.turbidity < MCK_MAX_NTU;

  if (drinkable) {
    return WaterClass::DRINKABLE;
  }

  if (mckAcceptable) {
    return WaterClass::MCK_ONLY;
  }

  return WaterClass::UNSAFE;
}

// =========================================================
// MODE BUTTONS
// =========================================================
bool checkModeButtons() {
  const uint8_t raw = (digitalRead(PIN_MODE_MCK) == LOW ? 1 : 0)
                    | (digitalRead(PIN_MODE_OFF) == LOW ? 2 : 0)
                    | (digitalRead(PIN_MODE_DRINK) == LOW ? 4 : 0);
  const UserMode previous = currentMode;
  // OFF and conflicting direction requests always take priority.
  if ((raw & 2) || ((raw & 5) == 5)) currentMode = UserMode::OFF;
  if (raw != candidateButtons) {
    candidateButtons = raw;
    buttonChangedAt = millis();
  }
  if (millis() - buttonChangedAt >= BUTTON_DEBOUNCE_MS && raw != stableButtons) {
    const uint8_t pressed = raw & ~stableButtons;
    stableButtons = raw;
    if (!(raw & 2) && (raw & 5) != 5) {
      if (pressed & 1) currentMode = UserMode::MCK;
      else if (pressed & 4) currentMode = UserMode::DRINK;
    }
  }
  if (currentMode != previous) {
    Serial.print("[MODE] ");
    Serial.println(modeToString(currentMode));
    return true;
  }
  return false;
}

// =========================================================
// AUTOMATIC OVERRIDE / ROUTING
// =========================================================
Outputs calculateOutputs(UserMode mode, WaterClass quality) {
  Outputs result{false, false, false, false};

  switch (mode) {
    case UserMode::MCK:
      // MCK mode accepts MCK-quality or drinkable-quality water.
      if (quality != WaterClass::UNSAFE) {
        result.hosePump = true;
        result.mckValve = true;
      }
      break;

    case UserMode::DRINK:
      // Safety override: only enable drinking route when quality passes.
      if (quality == WaterClass::DRINKABLE) {
        result.hosePump   = true;
        result.roPump     = true;
        result.drinkValve = true;
      }
      break;

    case UserMode::OFF:
      // OFF disables every output regardless of water quality.
      break;
  }

  return result;
}

// =========================================================
// RELAY CONTROL
// =========================================================
void writeRelay(uint8_t pin, bool on) {
  digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);
}

void applyOutputs(const Outputs& o) {
  writeRelay(PIN_RELAY_HOSE,  o.hosePump);
  writeRelay(PIN_RELAY_MCK,   o.mckValve);
  writeRelay(PIN_RELAY_RO,    o.roPump);
  writeRelay(PIN_RELAY_DRINK, o.drinkValve);
}

void allOutputsOff() {
  outputs = {false, false, false, false};
  applyOutputs(outputs);
}

// =========================================================
// LCD
// =========================================================
void updateLCD() {
  String row0 = "OASIS " + String(modeToString(currentMode));

  String row1 =
      "pH:" + String(sensorData.ph, 2);

  String row2 =
      "NTU:" + String(sensorData.turbidity, 2);

  String row3 =
      "STATUS: " + String(waterClassToString(waterClass));

  printLCDLine(0, row0);
  printLCDLine(1, row1);
  printLCDLine(2, row2);
  printLCDLine(3, row3);
}

void refreshControlState() {
  readSensors();
  waterClass = classifyWater(sensorData);
  outputs = calculateOutputs(currentMode, waterClass);
  applyOutputs(outputs);
  updateLCD();
}

// =========================================================
// SERIAL MONITOR
// =========================================================
void printSystemStatus() {
  Serial.println();
  Serial.println("========== O.A.S.I.S. ==========");

  Serial.print("Mode        : ");
  Serial.println(modeToString(currentMode));

  Serial.print("pH          : ");
  Serial.println(sensorData.ph, 2);

  Serial.print("Turbidity   : ");
  Serial.print(sensorData.turbidity, 2);
  Serial.println(" NTU");

  Serial.print("Water       : ");
  Serial.println(waterClassToString(waterClass));

  Serial.println("--- Outputs ---");

  Serial.print("Hose Pump   : ");
  Serial.println(outputs.hosePump ? "ON" : "OFF");

  Serial.print("MCK Valve   : ");
  Serial.println(outputs.mckValve ? "OPEN" : "CLOSED");

  Serial.print("RO Pump     : ");
  Serial.println(outputs.roPump ? "ON" : "OFF");

  Serial.print("Drink Valve : ");
  Serial.println(outputs.drinkValve ? "OPEN" : "CLOSED");

  Serial.println("================================");
}

// =========================================================
// LOCAL WEB DASHBOARD
// =========================================================
void sendStatusJson() {
  String payload;
  payload.reserve(420);
  payload += F("{\"mode\":\"");
  payload += modeToString(currentMode);
  payload += F("\",\"waterClass\":\"");
  payload += waterClassToString(waterClass);
  payload += F("\",\"ph\":");
  payload += String(sensorData.ph, 3);
  payload += F(",\"turbidity\":");
  payload += String(sensorData.turbidity, 3);
  payload += F(",\"outputs\":{\"hosePump\":");
  payload += outputs.hosePump ? F("true") : F("false");
  payload += F(",\"mckValve\":");
  payload += outputs.mckValve ? F("true") : F("false");
  payload += F(",\"roPump\":");
  payload += outputs.roPump ? F("true") : F("false");
  payload += F(",\"drinkValve\":");
  payload += outputs.drinkValve ? F("true") : F("false");
  payload += F("},\"apIp\":\"");
  payload += WiFi.softAPIP().toString();
  payload += F("\",\"stationIp\":\"");
  payload += WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("");
  payload += F("\",\"uptimeMs\":");
  payload += String(millis());
  payload += '}';

  webServer.sendHeader(F("Cache-Control"), F("no-store, max-age=0"));
  webServer.send(200, F("application/json"), payload);
}

void handleModeRequest() {
  if (!webServer.hasArg("mode")) {
    webServer.send(400, F("application/json"), F("{\"error\":\"missing mode\"}"));
    return;
  }

  const String requested = webServer.arg("mode");
  UserMode nextMode;
  if (requested == "MCK") nextMode = UserMode::MCK;
  else if (requested == "OFF") nextMode = UserMode::OFF;
  else if (requested == "DRINK") nextMode = UserMode::DRINK;
  else {
    webServer.send(422, F("application/json"), F("{\"error\":\"mode must be MCK, OFF, or DRINK\"}"));
    return;
  }

  if (nextMode != currentMode) {
    allOutputsOff();
    currentMode = nextMode;
    Serial.print(F("[MODE] "));
    Serial.print(modeToString(currentMode));
    Serial.println(F(" selected from web dashboard"));
  }
  refreshControlState();
  lastSensorUpdate = millis();
  sendStatusJson();
}

void sendDashboard() {
  webServer.sendHeader(F("Cache-Control"), F("no-store"));
  webServer.send_P(200, PSTR("text/html; charset=utf-8"), DASHBOARD_HTML);
}

void redirectToDashboard() {
  webServer.sendHeader(F("Location"), F("http://192.168.4.1/"), true);
  webServer.send(302, F("text/plain"), "");
}

void startWebDashboard() {
#ifdef OASIS_WOKWI_STA
  WiFi.mode(WIFI_AP_STA);
#else
  WiFi.mode(WIFI_AP);
#endif
  WiFi.setSleep(false);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, 6, false, 4);

#ifdef OASIS_WOKWI_STA
  // Wokwi's incoming port forwarding requires the simulated ESP32 to join
  // Wokwi-GUEST. The physical build omits this open-network connection.
  WiFi.begin("Wokwi-GUEST", "", 6);
#endif

  dnsServer.start(53, "*", AP_IP);
  webServer.on("/", HTTP_GET, sendDashboard);
  webServer.on("/api/status", HTTP_GET, sendStatusJson);
  webServer.on("/api/mode", HTTP_POST, handleModeRequest);
  webServer.on("/generate_204", HTTP_GET, sendDashboard);
  webServer.on("/hotspot-detect.html", HTTP_GET, sendDashboard);
  webServer.on("/connecttest.txt", HTTP_GET, sendDashboard);
  webServer.on("/ncsi.txt", HTTP_GET, sendDashboard);
  webServer.on("/favicon.ico", HTTP_GET, []() { webServer.send(204); });
  webServer.onNotFound(redirectToDashboard);
  webServer.begin();

  Serial.println();
  Serial.println(F("--- O.A.S.I.S. local dashboard ---"));
  Serial.print(F("SoftAP       : "));
  Serial.println(apStarted ? F("ready") : F("failed"));
  Serial.print(F("Wi-Fi name   : "));
  Serial.println(AP_SSID);
  Serial.print(F("Wi-Fi key    : "));
  Serial.println(AP_PASSWORD);
  Serial.print(F("Dashboard    : http://"));
  Serial.println(WiFi.softAPIP());
#ifdef OASIS_WOKWI_STA
  Serial.println(F("Wokwi URL   : http://localhost:8180"));
#endif
  Serial.println(F("----------------------------------"));
}

// =========================================================
// SETUP
// =========================================================
void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  pinMode(PIN_MODE_MCK, INPUT_PULLUP);
  pinMode(PIN_MODE_OFF, INPUT_PULLUP);
  pinMode(PIN_MODE_DRINK, INPUT_PULLUP);

  pinMode(PIN_RELAY_HOSE, OUTPUT);
  pinMode(PIN_RELAY_MCK, OUTPUT);
  pinMode(PIN_RELAY_RO, OUTPUT);
  pinMode(PIN_RELAY_DRINK, OUTPUT);

  allOutputsOff();

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();

  printLCDLine(0, "O.A.S.I.S.");
  printLCDLine(1, "ESP32 Controller");
  printLCDLine(2, "Initializing...");
  printLCDLine(3, "Mode: OFF");

  refreshControlState();
  startWebDashboard();

  delay(600);

  Serial.println();
  Serial.println("O.A.S.I.S. ESP32 simulation started.");
  Serial.println("MCK   button -> GPIO13");
  Serial.println("OFF   button -> GPIO14");
  Serial.println("DRINK button -> GPIO27");
}

// =========================================================
// MAIN LOOP
// =========================================================
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (!stationAddressReported && WiFi.status() == WL_CONNECTED) {
    stationAddressReported = true;
    Serial.print(F("[WIFI] Station address: "));
    Serial.println(WiFi.localIP());
  }

  const bool modeChanged = checkModeButtons();
  if (modeChanged) allOutputsOff();

  if (modeChanged || millis() - lastSensorUpdate >= SENSOR_INTERVAL_MS) {
    lastSensorUpdate = millis();
    refreshControlState();
  }

  if (millis() - lastSerialUpdate >= SERIAL_INTERVAL_MS) {
    lastSerialUpdate = millis();
    printSystemStatus();
  }
}
