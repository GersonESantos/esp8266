/***********************************************************************
 * TITLE: Plant Watering system using ESP32 Sinric Pro, Moisture Sensor
 * (For Active-LOW Relay module with Active-HIGH logic override)
 * * Este código mescla a funcionalidade original do SinricPro para controle
 * de bomba e leitura de sensor de umidade do solo, com uma interface
 * web local moderna para visualização e controle.
 *
 * Créditos originais do SinricPro: Tech StudyCell (links abaixo)
 * YouTube Video: https://youtu.be/MmbmNIKxfEI
 * Related Blog : https://iotcircuithub.com/esp32-projects/
 *
 * Bibliotecas necessárias:
 * - SinricPro Library (instale via Gerenciador de Bibliotecas)
 * - ESPAsyncWebServer (instale via Gerenciador de Bibliotecas)
 * - AsyncTCP (dependência do ESPAsyncWebServer, instale via Gerenciador de Bibliotecas)
 * - CapacitiveSoilMoistureSensor.h (sua biblioteca customizada, deve estar na pasta do sketch)
 ***********************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <ESPAsyncWebServer.h> // NOVO: Para o servidor web local
#include "CapacitiveSoilMoistureSensor.h" // Sua biblioteca customizada para o sensor de solo

// --- Credenciais e IDs ---
#define WIFI_SSID       "BRUGER_2G"
#define WIFI_PASS       "Gersones68"
#define APP_KEY         "89cda427-c430-4127-9a60-afd89f2364d7"
#define APP_SECRET      "a4993c6e-91d9-4b84-97f6-373c1d78789b-72975cde-8f6e-4ff5-b9a1-b67a298291b5"
#define PUMP_DEVICE_ID  "68df4f4a5918d860c09f0b00"
#define SOIL_DEVICE_ID  "68df50c05918d860c09f0b6c"

// ---- Hardware Pins ----
const int RELAY_PIN = 12;   // Relay for pump (active HIGH)
const int SOIL_PIN  = 34;   // Soil sensor ADC pin

// ---- Calibration values (adjust for your sensor) ----
const int VERY_DRY  = 2910;
const int NEITHER_DRY_OR_WET = 2098;
const int VERY_WET  = 925;
const int DRY_PUSH_NOTIFICATION_THRESHHOLD = 2850;
const int UNPLUGGED = 3000;

// ---- Globals (compartilhadas entre SinricPro e Web Server) ----
int lastSoilMoisture = 0; // Usado pelo SinricPro para detectar mudanças
String lastSoilState = ""; // Usado pelo SinricPro para detectar mudanças

// Variáveis para a interface web
bool  pumpState   = false; // Estado atual da bomba
// soilMoistureRaw será o valor de 'lastSoilMoisture' para o web server
float temperature = 0.0;   // Sem DHT, será 0.0 para a interface web
float humidity    = 0.0;   // Sem DHT, será 0.0 para a interface web

// Objetos SinricPro (DO NOT ALTERAR)
CapacitiveSoilMoistureSensor &soilSensor = SinricPro[SOIL_DEVICE_ID];
SinricProSwitch &pumpSwitch = SinricPro[PUMP_DEVICE_ID];

// Objeto do Servidor Web
AsyncWebServer server(80); // NOVO: Objeto do servidor web assíncrono

// ---- Pump Control (DO NOT ALTERAR) ----
bool onPowerState(const String& deviceId, bool &state) {
  if (deviceId == PUMP_DEVICE_ID) {
    // Lógica invertida para que 'state=true' (ON) envie HIGH.
    digitalWrite(RELAY_PIN, state ? HIGH : LOW); // Agora HIGH liga a bomba, LOW desliga
    pumpState = state; // Sincroniza o estado global da bomba
    Serial.printf("Pump %s\r\n", state ? "ON" : "OFF");
  }
  return true;
}

// ---- Soil Sensor Handler (DO NOT ALTERAR) ----
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

  // Inverte os rótulos "Dry" e "Wet"
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

  lastSoilMoisture = rawValue; // Atualiza o último valor de umidade do solo processado
}

// ---- Setup WiFi (DO NOT ALTERAR) ----
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

// ---- Setup Sinric Pro (DO NOT ALTERAR) ----
void setupSinricPro() {
  pumpSwitch.onPowerState(onPowerState);

  SinricPro.onConnected([] { Serial.println("[SinricPro]: Connected"); });
  SinricPro.onDisconnected([] { Serial.println("[SinricPro]: Disconnected"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// =================================================================
// --- PÁGINA WEB (HTML, CSS, JAVASCRIPT) ---
// =================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">

<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Painel de Irrigação IoT</title>

    <style>
        :root {
            --cor-primaria: #0b9dac;
            --cor-fundo: #001f21;
            --cor-card: #012526;
            --cor-texto: #e4e4e4;
            --cor-gauge-fundo: #084e55;
        }

        body {
            font-family: sans-serif;
            background: var(--cor-fundo);
            color: var(--cor-texto);
            display: flex;
            justify-content: center;
            align-items: flex-start;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }

        .container {
            max-width: 800px;
            width: 100%;
        }

        h1 {
            color: #fff;
            text-align: center;
        }

        .card {
            background: var(--cor-card);
            padding: 25px;
            border-radius: 8px;
            border-top: 3px solid var(--cor-primaria);
        }

        .content-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            justify-items: center;
            align-items: center;
            gap: 40px;
        }

        .control-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: 15px;
        }

        .pump-button {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            border: 2px solid rgba(255, 255, 255, 0.1);
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all .3s;
            background-color: transparent;
        }

        .pump-button svg {
            width: 50px;
            height: 50px;
            stroke: currentColor;
        }

        .pump-off {
            color: var(--cor-gauge-fundo);
        }

        .pump-on {
            color: var(--cor-primaria);
            filter: drop-shadow(0 0 12px var(--cor-primaria));
        }

        .gauge-wrapper {
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .gauge {
            position: relative;
            width: 150px;
            height: 150px;
        }

        .gauge svg {
            width: 100%;
            height: 100%;
            transform: rotate(-90deg);
        }

        .gauge .circle-bg {
            fill: none;
            stroke: var(--cor-gauge-fundo);
            stroke-width: 4;
        }

        .gauge .circle {
            fill: none;
            stroke: var(--cor-primaria);
            stroke-width: 4;
            stroke-linecap: round;
            transition: stroke-dasharray .5s ease;
        }

        .gauge .gauge-value {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 1.8rem;
            font-weight: 700;
        }

        .gauge-label {
            margin-top: 10px;
            font-weight: 700;
            font-size: 1.1rem;
            text-align: center;
        }
    </style>
</head>

<body>
    <div class="container">
        <header>
            <h1>Painel de Irrigação IoT</h1>
        </header>
        <main>
            <div class="card">
                <div class="content-grid">

                    <div class="control-container">
                        <button id="pumpButton" class="pump-button pump-off" onclick="togglePump()">
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="2">
                                <path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path>
                                <line x1="12" y1="2" x2="12" y2="12"></line>
                            </svg>
                        </button>
                        <div class="gauge-label">Bomba d'Água</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36">
                                <path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                                <path class="circle" id="circleSoil" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                            </svg>
                            <div class="gauge-value" id="soilValue">--%</div>
                        </div>
                        <div class="gauge-label">Umidade do Solo</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36">
                                <path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                                <path class="circle" id="circleTemp" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                            </svg>
                            <div class="gauge-value" id="tempValue">--°C</div>
                        </div>
                        <div class="gauge-label">Temperatura</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36">
                                <path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                                <path class="circle" id="circleHumidity" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84" />
                            </svg>
                            <div class="gauge-value" id="humidityValue">--%</div>
                        </div>
                        <div class="gauge-label">Umidade do Ar</div>
                    </div>
                    
                </div>
            </div>
        </main>
    </div>

    <script>
        function togglePump() {
            fetch("/toggle_pump");
        }

        function updateSensorData() {
            fetch("/data")
                .then(response => response.json())
                .then(data => {
                    // Atualiza o botão da bomba
                    const pumpBtn = document.getElementById("pumpButton");
                    pumpBtn.classList.toggle("pump-on", data.pumpState);
                    pumpBtn.classList.toggle("pump-off", !data.pumpState);
                    
                    // Atualiza o medidor de umidade do solo
                    const moisturePercent = data.soilPercent;
                    document.getElementById("soilValue").textContent = moisturePercent.toFixed(0) + "%";
                    document.getElementById("circleSoil").setAttribute("stroke-dasharray", moisturePercent.toFixed(0) + ", 100");
                    
                    // Atualiza o medidor de temperatura
                    // Como não há DHT, a temperatura será 0.0
                    const tempPercent = data.temperature / 50 * 100; 
                    document.getElementById("tempValue").textContent = data.temperature.toFixed(1) + "°C";
                    document.getElementById("circleTemp").setAttribute("stroke-dasharray", Math.min(100, Math.max(0, tempPercent)).toFixed(0) + ", 100");
                    
                    // Atualiza o medidor de umidade do ar
                    // Como não há DHT, a umidade do ar será 0.0
                    document.getElementById("humidityValue").textContent = data.humidity.toFixed(1) + "%";
                    document.getElementById("circleHumidity").setAttribute("stroke-dasharray", data.humidity.toFixed(0) + ", 100");
                })
                .catch(error => console.error("Erro ao buscar dados:", error));
        }
        
        setInterval(updateSensorData, 2000); // 2000 ms = 2 segundos
        window.onload = updateSensorData;
    </script>
</body>

</html>
)rawliteral";


// --- Funções do Servidor Web Local (NOVO) ---
void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
}

void handleData(AsyncWebServerRequest *request) {
    // Usa o valor de umidade do solo atualizado pela função SinricPro
    int soilPercent = map(lastSoilMoisture, VERY_DRY, VERY_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    String json = "{";
    json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
    json += "\"soilPercent\":" + String(soilPercent) + ",";
    json += "\"temperature\":" + String(temperature, 1) + ","; // Valor 0.0 pois DHT foi desconsiderado
    json += "\"humidity\":" + String(humidity, 1);             // Valor 0.0 pois DHT foi desconsiderado
    json += "}";
    request->send(200, "application/json", json);
}

void handleTogglePump(AsyncWebServerRequest *request) {
    pumpState = !pumpState; // Alterna o estado global da bomba
    digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW); // Controla o relé com base no novo estado
    Serial.printf("[Web] Bomba alterada para: %s\n", pumpState ? "ON" : "OFF");
    
    // Sincroniza a mudança com o SinricPro (DO NOT ALTERAR)
    pumpSwitch.sendPowerStateEvent(pumpState); 
    
    request->send(200, "text/plain", "OK");
}


// ---- Arduino Setup ----
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Pump OFF at start for active HIGH relay
  pinMode(SOIL_PIN, INPUT);
  
  setupWiFi();
  setupSinricPro();

  // Configura as rotas do servidor web local (NOVO)
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/toggle_pump", HTTP_GET, handleTogglePump);
  server.begin(); // Inicia o servidor web
}

// ---- Arduino Loop ----
void loop() {
  SinricPro.handle();       // Mantém a conexão SinricPro (DO NOT ALTERAR)
  handleSoilMoisture();     // Lida com o sensor de solo e envia para SinricPro (DO NOT ALTERAR)
  // server.handleClient(); // Não é necessário para ESPAsyncWebServer
}