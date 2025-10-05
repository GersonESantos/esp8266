/**********************************************************************************
 * TITLE: Plant Watering system using ESP32 Sinric Pro, Moisture Sensor (For Active-LOW Relay module)
 * Click on the following links to learn more. 
 * YouTube Video: https://youtu.be/MmbmNIKxfEI
 * Related Blog : https://iotcircuithub.com/esp32-projects/
 * * This code is provided free for project purpose and fair use only.
 * Please do mail us to techstudycell@gmail.com if you want to use it commercially.
 * Copyrighted © by Tech StudyCell
 * * Preferences--> Aditional boards Manager URLs : 
 * https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * * Download Board ESP32 (2.0.3) : https://github.com/espressif/arduino-esp32
 *
 * Download the libraries 
 * SinricPro Library (3.5.2): https://github.com/sinricpro/esp8266-esp32-sdk/
 * ArduinoJson by Benoit Blanchon (minimum Version 7.0.3)
 * WebSockets by Markus Sattler (minimum Version 2.4.0)
 **********************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include "CapacitiveSoilMoistureSensor.h" // Sua biblioteca customizada para o sensor de solo

// --- Credenciais e IDs ---
#define WIFI_SSID       "BRUGER_2G"
#define WIFI_PASS       "Gersones68"
#define APP_KEY         "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET      "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define PUMP_DEVICE_ID  "68df4f4a5918d860c09f0b00"
#define SOIL_DEVICE_ID "68df50c05918d860c09f0b6c"

// ---- Hardware Pins ----
// CORREÇÃO: Comentário ajustado e lógica de controle invertida para Active-HIGH
const int RELAY_PIN = 12;   // Relay for pump (active HIGH)
const int SOIL_PIN  = 34;   // Soil sensor ADC pin

// ---- Calibration values (adjust for your sensor) ----
const int VERY_DRY  = 2910;
const int NEITHER_DRY_OR_WET = 2098;
const int VERY_WET  = 925;
const int DRY_PUSH_NOTIFICATION_THRESHHOLD = 2850;
const int UNPLUGGED = 3000;

// ---- Globals ----
int lastSoilMoisture = 0;
String lastSoilState = "";

// Assumimos que 'CapacitiveSoilMoistureSensor' é uma classe SinricPro válida ou um wrapper.
CapacitiveSoilMoistureSensor &soilSensor = SinricPro[SOIL_DEVICE_ID];
SinricProSwitch &pumpSwitch = SinricPro[PUMP_DEVICE_ID];

// ---- Pump Control ----
bool onPowerState(const String& deviceId, bool &state) {
  if (deviceId == PUMP_DEVICE_ID) {
    // CORREÇÃO: Lógica invertida para que 'state=true' (ON) envie HIGH.
    digitalWrite(RELAY_PIN, state ? HIGH : LOW); // Agora HIGH liga a bomba, LOW desliga
    Serial.printf("Pump %s\r\n", state ? "ON" : "OFF");
  }
  return true;
}

// ---- Soil Sensor Handler ----
void handleSoilMoisture() {
  if (!SinricPro.isConnected()) return;

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis < 60000) return;   // every 60 sec 
  lastMillis = millis();

  int rawValue = analogRead(SOIL_PIN);
  int percentage = map(rawValue, VERY_DRY, VERY_WET, 0, 100); // Mapeamento: VERY_DRY (alto ADC) -> 0%, VERY_WET (baixo ADC) -> 100%
  percentage = constrain(percentage, 1, 100);

  Serial.printf("Soil ADC: %d | Moisture: %d%%\r\n", rawValue, percentage);

  if (rawValue == lastSoilMoisture) {
    Serial.println("No change in soil moisture, skipping update...");
    return;
  }

  // CORREÇÃO: Inverte os rótulos "Dry" e "Wet"
  String soilState = (rawValue > NEITHER_DRY_OR_WET) ? "Wet" : "Dry"; // Se rawValue alto (mais seco), agora é "Wet", senão "Dry"
  if (soilState != lastSoilState) {
    soilSensor.sendModeEvent("modeInstance1", soilState, "PHYSICAL_INTERACTION");
    lastSoilState = soilState;
  }

  // Update Range: % value
  soilSensor.sendRangeValueEvent("rangeInstance1", percentage);

  // Push Notifications
  if (rawValue > DRY_PUSH_NOTIFICATION_THRESHHOLD) {
    soilSensor.sendPushNotification("Plants are too dry. Please water them!");
  }
  if (rawValue > UNPLUGGED) {
    soilSensor.sendPushNotification("Soil sensor may be unplugged!");
  }

  lastSoilMoisture = rawValue;
}

// ---- Setup WiFi ----
void setupWiFi() {
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi]: Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println(" connected!");
}

// ---- Setup Sinric Pro ----
void setupSinricPro() {
  pumpSwitch.onPowerState(onPowerState);

  SinricPro.onConnected([] { Serial.println("[SinricPro]: Connected"); });
  SinricPro.onDisconnected([] { Serial.println("[SinricPro]: Disconnected"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// ---- Arduino Setup ----
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  // CORREÇÃO: Inicia com o relé em LOW (desligado para active-HIGH)
  digitalWrite(RELAY_PIN, LOW); // Pump OFF at start for active HIGH relay
  pinMode(SOIL_PIN, INPUT);
  
  setupWiFi();
  setupSinricPro();
}

// ---- Arduino Loop ----
void loop() {
  SinricPro.handle();
  handleSoilMoisture();
}