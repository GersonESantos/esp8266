/********************************************************
 * PROJETO MESCLADO: Sensor de Umidade com Relé e Sinric Pro
 * Lógica do Relé Corrigida para Módulos "Active LOW"
 ********************************************************/

// Bibliotecas
#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include "SensordeUmidadeDoSolo.h" // Garanta que este arquivo .h está na mesma pasta

// --- Definições de Hardware e Lógica ---
// Valores de calibração do sensor (AJUSTE CONFORME SEU SENSOR)
const int SENSOR_SECO = 686;    // Valor ADC lido com o sensor no ar
const int SENSOR_MOLHADO = 360; // Valor ADC lido com o sensor na água

// Pinos de hardware
const int PINO_SENSOR_UMIDADE = 34;
const int PINO_RELE = 12;

// Limite para acionar a irrigação
const int LIMITE_UMIDADE_MINIMA = 70; // Em %. A bomba liga se a umidade for MENOR que isso.

// --- Configurações Sinric Pro e Wi-Fi ---
#define APP_KEY    "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define DEVICE_ID  "68daf7175918d860c09da1fb"
#define SSID       "BRUGER_2G"
#define PASS       "Gersones68"
#define BAUD_RATE  115200

// --- Variáveis Globais ---
unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 2000; // Ler a cada 2 segundos

SensordeUmidadeDoSolo &sensordeUmidadeDoSolo = SinricPro[DEVICE_ID];

// --- Callbacks e Eventos Sinric Pro (sem alterações) ---
bool onRangeValue(const String &deviceId, const String& instance, int &rangeValue) { return true; }
bool onAdjustRangeValue(const String &deviceId, const String& instance, int &valueDelta) { return true; }
bool onSetMode(const String& deviceId, const String& instance, String &mode) { return true; }
void updateRangeValue(String instance, int value) { sensordeUmidadeDoSolo.sendRangeValueEvent(instance, value); }
void updateMode(String instance, String mode) { sensordeUmidadeDoSolo.sendModeEvent(instance, mode, "PHYSICAL_INTERACTION"); }

// --- Funções de Configuração ---
void setupSinricPro() {
  sensordeUmidadeDoSolo.onRangeValue("rangeInstance1", onRangeValue);
  sensordeUmidadeDoSolo.onAdjustRangeValue("rangeInstance1", onAdjustRangeValue);
  sensordeUmidadeDoSolo.onSetMode("modeInstance1", onSetMode);
  SinricPro.onConnected([]{ Serial.printf("[SinricPro]: Conectado\r\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("[SinricPro]: Desconectado\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
};

void setupWiFi() {
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Conectando a %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf(" conectado, IP: %s\r\n", WiFi.localIP().toString().c_str());
}

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(PINO_RELE, OUTPUT);
  // Garante que a bomba comece desligada (enviando sinal HIGH para um relé Active LOW)
  digitalWrite(PINO_RELE, HIGH); 
  
  setupWiFi();
  setupSinricPro();
}

// --- Loop Principal ---
void loop() {
  SinricPro.handle();

  if (millis() - ultimaLeitura >= intervaloLeitura) {
    int valorSensor = analogRead(PINO_SENSOR_UMIDADE);
    int perct = map(valorSensor, SENSOR_SECO, SENSOR_MOLHADO, 0, 100);
    perct = constrain(perct, 0, 100);

    Serial.print("Umidade do solo: ");
    Serial.print(perct);
    Serial.println("%");

    // Lógica de controle do relé (CORRIGIDA PARA ACTIVE LOW)
    if (perct >= LIMITE_UMIDADE_MINIMA) {
      // Se o solo está seco, LIGA o relé (enviando sinal LOW)
      digitalWrite(PINO_RELE, HIGH);
      Serial.println("Status: Solo seco. Bomba LIGADA.");
    } else {
      // Se o solo está úmido, DESLIGA o relé (enviando sinal HIGH)
      digitalWrite(PINO_RELE, LOW);
      Serial.println("Status: Solo úmido. Bomba DESLIGADA.");
    }

    // Envia os dados atualizados para o Sinric Pro / Alexa
    updateRangeValue("rangeInstance1", perct);
    updateMode("modeInstance1", (perct < LIMITE_UMIDADE_MINIMA) ? "Dry" : "Wet");
    
    ultimaLeitura = millis();
  }
}