/****************************************************************
 * PROJETO FINAL MESCLADO: Sensor de Umidade com Relé e Sinric Pro
 * Combina a leitura de hardware com a estrutura de comunicação
 * do exemplo Sinric Pro.
 ****************************************************************/

// Bibliotecas
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
#endif
#include <SinricPro.h>
#include "SensordeUmidadeDoSolo.h" // Garanta que este arquivo .h está na mesma pasta

// --- Definições de Hardware e Lógica (DO SEU PRIMEIRO CÓDIGO) ---
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

// --- Variáveis Globais para o Sensor (DO SEU PRIMEIRO CÓDIGO) ---
unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 5000; // Ler a cada 5 segundos para estabilidade

SensordeUmidadeDoSolo &sensordeUmidadeDoSolo = SinricPro[DEVICE_ID];

// --- Callbacks e Eventos Sinric Pro (Estrutura do segundo código) ---
// Callbacks são chamados quando você envia um comando PELO APP/ALEXA
bool onRangeValue(const String &deviceId, const String& instance, int &rangeValue) {
  Serial.printf("[SinricPro]: Recebido comando para definir RangeValue para %d (não utilizado neste projeto).\n", rangeValue);
  return true;
}

bool onAdjustRangeValue(const String &deviceId, const String& instance, int &valueDelta) {
  Serial.printf("[SinricPro]: Recebido comando para ajustar RangeValue em %d (não utilizado neste projeto).\n", valueDelta);
  return true;
}

bool onSetMode(const String& deviceId, const String& instance, String &mode) {
  Serial.printf("[SinricPro]: Recebido comando para definir o Modo para %s (não utilizado neste projeto).\n", mode.c_str());
  return true;
}

// --- Funções de Configuração ---
void setupSinricPro() {
  sensordeUmidadeDoSolo.onRangeValue("rangeInstance1", onRangeValue);
  sensordeUmidadeDoSolo.onAdjustRangeValue("rangeInstance1", onAdjustRangeValue);
  sensordeUmidadeDoSolo.onSetMode("modeInstance1", onSetMode);

  SinricPro.onConnected([]{ Serial.printf("[SinricPro]: Conectado ao servidor.\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("[SinricPro]: Desconectado do servidor.\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
};

void setupWiFi() {
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Conectando a %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf(" conectado, IP: %s\n", WiFi.localIP().toString().c_str());
}

void setup() {
  Serial.begin(BAUD_RATE);
  
  // Configuração do pino do relé (do seu primeiro código)
  pinMode(PINO_RELE, OUTPUT);
  // Garante que a bomba comece desligada (sinal HIGH para um relé Active LOW)
  digitalWrite(PINO_RELE, HIGH); 
  
  setupWiFi();
  setupSinricPro();
}


// --- Loop Principal (LÓGICA DO SENSOR INSERIDA AQUI) ---
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








