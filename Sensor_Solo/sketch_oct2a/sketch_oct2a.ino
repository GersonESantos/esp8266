/*
 * Sensor de Umidade do Solo Capacitivo com Bomba
 * Corrige discrepância entre valores no dashboard e monitor serial
 */

// Uncomment the following line to enable serial debug output
#define ENABLE_DEBUG

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

#define APP_KEY    "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define DEVICE_ID  "68daf7175918d860c09da1fb"
#define SSID       "BRUGER_2G"
#define PASS       "Gersones68"
#define BAUD_RATE  115200


#define SLEEP_DELAY_IN_SECONDS  60   // 15 minutos

// Definição do pino ADC conforme a plataforma
#if defined(ESP8266)
  const int adcPin = A0;  
#elif defined(ESP32) 
  const int adcPin = 34;  
#elif defined(ARDUINO_ARCH_RP2040)
  const int adcPin = 26;  
#endif

// Definição do pino da bomba (ajuste conforme sua configuração)
const int bombaPin = 5;

// Calibração do sensor (AJUSTE CONFORME SEU SENSOR)
const int VERY_DRY = 720;    // Valor quando solo está seco
const int VERY_WET = 560;    // Valor quando solo está molhado
const int NEITHER_DRY_OR_WET = (VERY_DRY + VERY_WET) / 2;  // Valor médio

// Limite para ativar/desativar bomba (ajuste conforme necessário)
const int LIMITE_BOMBA_LIGAR = 30;   // Abaixo de 30% liga a bomba
const int LIMITE_BOMBA_DESLIGAR = 60; // Acima de 60% desliga a bomba

CapacitiveSoilMoistureSensor &capacitiveSoilMoistureSensor = SinricPro[DEVICE_ID];

bool bombaEstado = false;

void atualizarEstadoSolo(String modo) {
  capacitiveSoilMoistureSensor.sendModeEvent("modeInstance1", modo, "PHYSICAL_INTERACTION");
}

void atualizarNivelUmidade(int valor) {
  capacitiveSoilMoistureSensor.sendRangeValueEvent("rangeInstance1", valor);
}

void controlarBomba(int porcentagem) {
  bool bombaDeveLigar = (porcentagem < LIMITE_BOMBA_LIGAR);
  bool bombaDeveDesligar = (porcentagem > LIMITE_BOMBA_DESLIGAR);
  
  if (bombaDeveLigar && !bombaEstado) {
    digitalWrite(bombaPin, HIGH);
    bombaEstado = true;
    Serial.println("Bomba LIGADA.");
  } else if (bombaDeveDesligar && bombaEstado) {
    digitalWrite(bombaPin, LOW);
    bombaEstado = false;
    Serial.println("Bomba DESLIGADA.");
  }
}

void enviarUmidadeSolo() {
  int leituraADC = analogRead(adcPin);
  
  // Converter ADC para porcentagem (0-100%)
  int porcentagem = map(leituraADC, VERY_WET, VERY_DRY, 100, 0);
  porcentagem = constrain(porcentagem, 0, 100);  // Garantir que está entre 0-100

  // Determinar estado do solo
  String estadoSolo = "";
  if(leituraADC > NEITHER_DRY_OR_WET) {
    estadoSolo = "Dry";  // Solo seco
  } else {
    estadoSolo = "Wet";  // Solo molhado
  }

  // Controlar bomba automaticamente
  controlarBomba(porcentagem);

  // Debug no monitor serial
  Serial.printf("Leitura ADC: %d -> Umidade: %d%%\r\n", leituraADC, porcentagem);
  Serial.printf("Status: Solo %s. Bomba %s.\r\n", 
                estadoSolo.c_str(), 
                bombaEstado ? "LIGADA" : "DESLIGADA");
  
  // Debug do que será enviado para SinricPro
  Serial.printf("[SinricPro]: Enviando umidade (%d%%) e estado (%s) para o dashboard...\r\n", 
                porcentagem, estadoSolo.c_str());

  // Enviar dados para SinricPro (CORREÇÃO CRÍTICA)
  atualizarEstadoSolo(estadoSolo);           // Enviar estado "Dry" ou "Wet"
  atualizarNivelUmidade(porcentagem);        // Enviar PORCENTAGEM, não valor ADC
  
  // Aguardar confirmação dos eventos
  SinricPro.handle();
  delay(1000);
}

void configurarSensor() {
  pinMode(adcPin, INPUT);
  pinMode(bombaPin, OUTPUT);
  digitalWrite(bombaPin, LOW);  // Iniciar com bomba desligada
}

void configurarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Conectando a %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("conectado\r\n");
}

// Conectar ao servidor SinricPro de forma síncrona
void aguardarConexaoSinricPro() {
  SinricPro.begin(APP_KEY, APP_SECRET);  
  
  while (SinricPro.isConnected() == false) {
    SinricPro.handle();
    yield();
  }
  delay(100);
  Serial.printf("Conectado ao SinricPro!\r\n");
}

// Desconectar do servidor SinricPro
void pararSinricPro() {
  SinricPro.handle(); 
  SinricPro.stop();
  
  while (SinricPro.isConnected()) {
    SinricPro.handle();
    yield();
  }
}

void pararWiFi(){
  WiFi.disconnect();
}

void reportarEstado() {
  configurarSensor();
  configurarWiFi();
  aguardarConexaoSinricPro();
  enviarUmidadeSolo();
  pararSinricPro();
  pararWiFi();
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.setTimeout(2000);
  while(!Serial) { }
  
  Serial.println("=== Sensor de Umidade do Solo com Bomba Automática ===");
  Serial.println("Iniciando leitura...");
  
  reportarEstado();
  
  Serial.printf("Indo dormir por %d segundos...\r\n", SLEEP_DELAY_IN_SECONDS);
  ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000);
}

void loop() {
  // O código principal está no setup() devido ao deep sleep
}