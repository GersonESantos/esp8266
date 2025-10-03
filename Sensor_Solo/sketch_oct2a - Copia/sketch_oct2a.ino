/*
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

// Custom devices requires SinricPro ESP8266/ESP32 SDK 2.9.6 or later

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
#endif
 
#include <SinricPro.h>
#include "CapacitiveSoilMoistureSensor.h"

// DECLARAÇÕES DAS FUNÇÕES
void updateSoilState(String mode);
void updateMoistureLevel(int value);


#define APP_KEY    "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define DEVICE_ID  "68daf7175918d860c09da1fb"
#define SSID       "BRUGER_2G"
#define PASS       "Gersones68"


#define BAUD_RATE               115200    // Change baudrate to your need (used for serial monitor)
#define SLEEP_DELAY_IN_SECONDS  60 * 15   // sleep duration. 15 mins.

#if defined(ESP8266)
  const int adcPin = A0;  
#elif defined(ESP32) 
  const int adcPin = 34;  
#elif defined(ARDUINO_ARCH_RP2040)
  const int adcPin = 26;  
#endif

const int VERY_DRY = 720; // TODO: This is when soil is dry. Adjust according to your sensor
const int VERY_WET = 560; // TODO: This is when soil is wet. Adjust according to your sensor
const int NEITHER_DRY_OR_WET = (VERY_DRY + VERY_WET) / 2; // Adicione esta linha

//ger01CapacitiveSoilMoistureSensor &capacitiveSoilMoistureSensor = SinricPro[DEVICE_ID];

// DEFINIÇÕES DAS FUNÇÕES
void updateSoilState(String mode) {
  //ger02capacitiveSoilMoistureSensor.sendModeEvent("modeInstance1", mode, "PHYSICAL_INTERACTION");
}

void updateMoistureLevel(int value) {
  //ger03capacitiveSoilMoistureSensor.sendRangeValueEvent("rangeInstance1", value);
}

void sendSoilMoisture() {
  int soilMoisture = analogRead(adcPin);
  int percentage = map(soilMoisture, VERY_WET, VERY_DRY, 100, 0); 

  if (percentage < 0) {
    percentage = 0;
  }
  if (percentage > 100) {
    percentage = 100;
  }

  Serial.printf("Soil Moisture: %d. as a percentage: %d%%\r\n", soilMoisture, percentage); 
  
  // Determine Wet or Dry.
  String soilMoistureStr = "";
  if(soilMoisture > NEITHER_DRY_OR_WET) {
    soilMoistureStr = "Dry"; // NOTE: must be Wet, Dry. This is from modes list in template.
  } else {
    soilMoistureStr = "Wet";
  }
  
  // Update Wet or Dry.
  updateSoilState(soilMoistureStr);

  // Update Moisture level label
  updateMoistureLevel(percentage); // Corrigido: enviar percentage, não soilMoisture
}
 
void setupSensor() {
  pinMode(adcPin, INPUT);
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected\r\n");
}

// Connect to Sinric Pro server synchronously.
void waitForSinricProConnect() {
  SinricPro.begin(APP_KEY, APP_SECRET);  
  
  while (SinricPro.isConnected() == false) { // wait for connect
    SinricPro.handle();
    yield();
  }
  delay(100);
  Serial.printf("waitForSinricProConnect(): Connected to SinricPro ..\n"); 
}

// Disconnect from Sinric Pro server.
void stopSinricPro() {
  SinricPro.handle(); 
  SinricPro.stop();
  
  while (SinricPro.isConnected()) { // wait for disconnect
    SinricPro.handle();
    yield();
  }
}

void stopWiFi(){
  WiFi.disconnect();
}

void reportState() {
  setupSensor();
  setupWiFi();
  waitForSinricProConnect();
  sendSoilMoisture();
  stopSinricPro();
  stopWiFi();
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.setTimeout(2000);
  while(!Serial) { }
  
  reportState(); // Report the status to server
  
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000); // Goto sleep.
}
 
void loop() {
  SinricPro.handle(); // Essencial para manter a comunicação

  // Lógica de leitura do sensor a cada X segundos (do seu primeiro código)
  if (millis() - ultimaLeitura >= intervaloLeitura) {
    ultimaLeitura = millis();

    int valorSensor = analogRead(PINO_SENSOR_UMIDADE);
    
    // LÓGICA DE MAPEAMENTO CORRIGIDA: Seco=0%, Molhado=100%
    int perct = map(valorSensor, SENSOR_SECO, SENSOR_MOLHADO, 0, 100);
    perct = constrain(perct, 0, 100); // Garante que o valor esteja sempre entre 0 e 100

    Serial.println("-------------------------");
    Serial.printf("Leitura ADC: %d -> Umidade: %d%%\n", valorSensor, perct);

    // Lógica de controle do relé
    if (perct < LIMITE_UMIDADE_MINIMA) {
      digitalWrite(PINO_RELE, LOW); // LIGA a bomba (relé Active LOW)
      Serial.println("Status: Solo seco. Bomba LIGADA.");
    } else {
      digitalWrite(PINO_RELE, HIGH); // DESLIGA a bomba (relé Active LOW)
      Serial.println("Status: Solo úmido. Bomba DESLIGADA.");
    }

    // Envia os dados atualizados para o Sinric Pro / Alexa
    Serial.printf("[SinricPro]: Enviando umidade (%d%%) para o dashboard...\n", perct);
    sensordeUmidadeDoSolo.sendRangeValueEvent("rangeInstance1", perct);

    // AQUI ESTÁ A CORREÇÃO: Adicionado o terceiro parâmetro "PHYSICAL_INTERACTION"
    sensordeUmidadeDoSolo.sendModeEvent("modeInstance1", (perct < LIMITE_UMIDADE_MINIMA) ? "Dry" : "Wet", "PHYSICAL_INTERACTION");
  }
}