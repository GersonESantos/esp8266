/***********************************************************************
 * PROJETO: Estação de Irrigação Híbrida (Painel Web + SinricPro)
 * DESCRIÇÃO: Controla uma bomba e monitora sensores através de um 
 * painel web local e da plataforma de nuvem SinricPro (Alexa/Google).
 * VERSÃO FINAL CORRIGIDA
 ***********************************************************************/

// --- Bibliotecas ---
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <SinricProTemperaturesensor.h> // CORREÇÃO: Biblioteca correta para o sensor

// --- Credenciais e IDs ---
#define WIFI_SSID       "BRUGER_2G"
#define WIFI_PASS       "Gersones68"
#define APP_KEY         "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET      "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define PUMP_DEVICE_ID  "68df4f4a5918d860c09f0b00"
#define SENSOR_DEVICE_ID "68df50c05918d860c09f0b6c"

// --- Configurações de Hardware ---
const int RELAY_PIN = 12;
const int SOIL_PIN  = 34;
const int DHT_PIN   = 27;
#define DHTTYPE     DHT22

// --- Calibração do Sensor de Solo (Ajuste conforme seu sensor) ---
const int VERY_DRY = 1910;
const int VERY_WET = 625;

// --- Objetos e Variáveis Globais ---
DHT dht(DHT_PIN, DHTTYPE);
AsyncWebServer server(80);

// Dispositivos SinricPro
SinricProSwitch &pumpSwitch = SinricPro[PUMP_DEVICE_ID];
// CORREÇÃO: Usando a classe correta da biblioteca SinricPro
SinricProTemperaturesensor &sensorDevice = SinricPro[SENSOR_DEVICE_ID];

// Variáveis de estado
bool  pumpState   = false;
int   soilMoistureRaw = 0;
float temperature = 0.0;
float humidity    = 0.0;

const unsigned long SENSOR_UPDATE_INTERVAL = 15000; // 15 segundos
unsigned long lastSensorUpdate = 0;

// =================================================================
// --- PÁGINA WEB (HTML, CSS, JAVASCRIPT) ---
// =================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt-br"><head><meta charset="UTF-8" /><meta name="viewport" content="width=device-width, initial-scale=1.0" /><title>Painel de Irrigação IoT</title><style>:root{--cor-primaria:#0b9dac;--cor-fundo:#001f21;--cor-card:#012526;--cor-texto:#e4e4e4;--cor-gauge-fundo:#084e55}body{font-family:sans-serif;background:var(--cor-fundo);color:var(--cor-texto);display:flex;justify-content:center;align-items:flex-start;min-height:100vh;margin:0;padding:20px;box-sizing:border-box}.container{max-width:800px;width:100%}h1{color:#fff;text-align:center}.card{background:var(--cor-card);padding:25px;border-radius:8px;border-top:3px solid var(--cor-primaria)}.content-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));justify-items:center;align-items:center;gap:40px}.control-container{display:flex;flex-direction:column;align-items:center;gap:15px}.pump-button{width:100px;height:100px;border-radius:50%;border:2px solid rgba(255,255,255,0.1);cursor:pointer;display:flex;align-items:center;justify-content:center;transition:all .3s;background-color:transparent}.pump-button svg{width:50px;height:50px;stroke:currentColor}.pump-off{color:var(--cor-gauge-fundo)}.pump-on{color:var(--cor-primaria);filter:drop-shadow(0 0 12px var(--cor-primaria))}.gauge-wrapper{display:flex;flex-direction:column;align-items:center}.gauge{position:relative;width:150px;height:150px}.gauge svg{width:100%;height:100%;transform:rotate(-90deg)}.gauge .circle-bg{fill:none;stroke:var(--cor-gauge-fundo);stroke-width:4}.gauge .circle{fill:none;stroke:var(--cor-primaria);stroke-width:4;stroke-linecap:round;transition:stroke-dasharray .5s ease}.gauge .gauge-value{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);font-size:1.8rem;font-weight:700}.gauge-label{margin-top:10px;font-weight:700;font-size:1.1rem;text-align:center}</style></head><body><div class="container"><header><h1>Painel de Irrigação IoT</h1></header><main><div class="card"><div class="content-grid"><div class="control-container"><button id="pumpButton" class="pump-button pump-off" onclick="togglePump()"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg></button><div class="gauge-label">Bomba d'Água</div></div><div class="gauge-wrapper"><div class="gauge"><svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleSoil" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg><div class="gauge-value" id="soilValue">--%</div></div><div class="gauge-label">Umidade do Solo</div></div><div class="gauge-wrapper"><div class="gauge"><svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleTemp" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg><div class="gauge-value" id="tempValue">--°C</div></div><div class="gauge-label">Temperatura</div></div><div class="gauge-wrapper"><div class="gauge"><svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleHumidity" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg><div class="gauge-value" id="humidityValue">--%</div></div><div class="gauge-label">Umidade do Ar</div></div></div></div></main></div><script>function togglePump(){fetch("/toggle_pump")}function updateSensorData(){fetch("/data").then(response=>response.json()).then(data=>{const pumpBtn=document.getElementById("pumpButton");pumpBtn.classList.toggle("pump-on",data.pumpState);pumpBtn.classList.toggle("pump-off",!data.pumpState);const moisturePercent=data.soilPercent;document.getElementById("soilValue").textContent=moisturePercent.toFixed(0)+"%";document.getElementById("circleSoil").setAttribute("stroke-dasharray",moisturePercent.toFixed(0)+", 100");const tempPercent=data.temperature/50*100;document.getElementById("tempValue").textContent=data.temperature.toFixed(1)+"°C";document.getElementById("circleTemp").setAttribute("stroke-dasharray",Math.min(100,Math.max(0,tempPercent)).toFixed(0)+", 100");document.getElementById("humidityValue").textContent=data.humidity.toFixed(1)+"%";document.getElementById("circleHumidity").setAttribute("stroke-dasharray",data.humidity.toFixed(0)+", 100")}).catch(error=>console.error("Erro ao buscar dados:",error))}setInterval(updateSensorData,2e3);window.onload=updateSensorData;</script></body></html>
)rawliteral";

// --- Funções de Callback do SinricPro ---
bool onPowerState(const String &deviceId, bool &state) {
    Serial.printf("[SinricPro] Comando de energia recebido: %s\n", state ? "ON" : "OFF");
    pumpState = state;
    digitalWrite(RELAY_PIN, pumpState);
    return true;
}

// --- Funções do Servidor Web Local ---
void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
}

void handleData(AsyncWebServerRequest *request) {
    int soilPercent = map(soilMoistureRaw, VERY_DRY, VERY_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    String json = "{";
    json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
    json += "\"soilPercent\":" + String(soilPercent) + ",";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1);
    json += "}";
    request->send(200, "application/json", json);
}

void handleTogglePump(AsyncWebServerRequest *request) {
    pumpState = !pumpState;
    digitalWrite(RELAY_PIN, pumpState);
    Serial.printf("[Web] Bomba alterada para: %s\n", pumpState ? "ON" : "OFF");
    pumpSwitch.sendPowerStateEvent(pumpState);
    request->send(200, "text/plain", "OK");
}

// --- Funções de Leitura e Envio de Dados para Sinric ---
void readAndSendSensorData() {
    soilMoistureRaw = analogRead(SOIL_PIN);
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (isnan(newTemp) || isnan(newHum)) {
        Serial.println("[Sensor] Falha ao ler do sensor DHT!");
        return;
    }
    
    temperature = newTemp;
    humidity = newHum;

    Serial.printf("[Sensor] Leitura: Temp=%.1f C, Umid Ar=%.1f %%, Umid Solo(raw)=%d\n", temperature, humidity, soilMoistureRaw);

    // Envia temperatura e umidade do ar
    sensorDevice.sendTemperatureEvent(temperature, humidity);

    int soilPercent = map(soilMoistureRaw, VERY_DRY, VERY_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);
    // Envia a umidade do solo usando o evento de umidade separado
    sensorDevice.sendHumidityEvent(soilPercent);
}

// --- Funções Principais de SETUP ---
void setupWiFi() {
    Serial.printf("\n[WiFi] Conectando a %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\n[WiFi] Conectado!");
    Serial.print("[IP] Endereço: ");
    Serial.println(WiFi.localIP());
}

void setupSinricPro() {
    pumpSwitch.onPowerState(onPowerState);
    
    SinricPro.onConnected([](){ Serial.println("[SinricPro] Conectado"); });
    SinricPro.onDisconnected([](){ Serial.println("[SinricPro] Desconectado"); });
    SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, pumpState);

    setupWiFi();
    setupSinricPro();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/toggle_pump", HTTP_GET, handleTogglePump);
    server.begin();
    
    Serial.println("[Server] Servidor Web local iniciado.");
    Serial.println("Setup finalizado. Sistema pronto.");
}

// --- Função Principal de LOOP ---
void loop() {
    SinricPro.handle();

    if (millis() - lastSensorUpdate > SENSOR_UPDATE_INTERVAL) {
        readAndSendSensorData();
        lastSensorUpdate = millis();
    }
}