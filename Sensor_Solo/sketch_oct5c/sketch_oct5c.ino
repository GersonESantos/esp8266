/***********************************************************************
 * PROJETO: Estação de Irrigação e Monitoramento Ambiental
 * DESCRIÇÃO: Controla uma bomba e monitora a umidade do solo, 
 * temperatura e umidade do ar com uma interface web moderna.
 ***********************************************************************/

// --- Bibliotecas ---
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h> // NOVO: Biblioteca para o sensor DHT

// --- Configurações de Rede ---
#define SSID "BRUGER_2G"
#define PASS "Gersones68"

// --- Configurações de Hardware ---
const int RELAY_PIN = 12; // Pino do relé para a bomba
const int SOIL_PIN  = 34; // Pino do sensor de umidade do solo
const int DHT_PIN   = 27; // NOVO: Pino para o sensor DHT22

// --- Objetos e Variáveis Globais ---
DHT dht(DHT_PIN, DHT22);   // NOVO: Objeto do sensor DHT

bool  pumpState = false; // Estado atual da bomba (false = desligada)
int   soilMoistureValue = 0; // Valor bruto do sensor de solo
float temperature = 0.0; // NOVO: Variável para temperatura
float humidity    = 0.0; // NOVO: Variável para umidade do ar

AsyncWebServer server(80);

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
        .container { max-width: 800px; width: 100%; }
        h1 { color: #fff; text-align: center; }
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
            border: 2px solid rgba(255,255,255,0.1); 
            cursor: pointer; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            transition: all 0.3s; 
            background-color: transparent;
        }
        .pump-button svg { width: 50px; height: 50px; stroke: currentColor; }
        .pump-off { color: var(--cor-gauge-fundo); }
        .pump-on { color: var(--cor-primaria); filter: drop-shadow(0 0 12px var(--cor-primaria)); }
        
        .gauge-wrapper { display: flex; flex-direction: column; align-items: center; }
        .gauge { position: relative; width: 150px; height: 150px; }
        .gauge svg { width: 100%; height: 100%; transform: rotate(-90deg); }
        .gauge .circle-bg { fill: none; stroke: var(--cor-gauge-fundo); stroke-width: 4; }
        .gauge .circle { fill: none; stroke: var(--cor-primaria); stroke-width: 4; stroke-linecap: round; transition: stroke-dasharray 0.5s ease; }
        .gauge .gauge-value { 
            position: absolute; 
            top: 50%; 
            left: 50%; 
            transform: translate(-50%, -50%); 
            font-size: 1.8rem; 
            font-weight: 700; 
        }
        .gauge-label { margin-top: 10px; font-weight: 700; font-size: 1.1rem; text-align: center; }
    </style>
</head>
<body>
    <div class="container">
        <header><h1>Painel de Irrigação IoT</h1></header>
        <main>
            <div class="card">
                <div class="content-grid">
                    
                    <div class="control-container">
                        <button id="pumpButton" class="pump-button pump-off" onclick="togglePump()">
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
                        </button>
                        <div class="gauge-label">Bomba d'Água</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleSoil" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg>
                            <div class="gauge-value" id="soilValue">--%</div>
                        </div>
                        <div class="gauge-label">Umidade do Solo</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleTemp" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg>
                            <div class="gauge-value" id="tempValue">--°C</div>
                        </div>
                        <div class="gauge-label">Temperatura</div>
                    </div>

                    <div class="gauge-wrapper">
                        <div class="gauge">
                            <svg viewBox="0 0 36 36"><path class="circle-bg" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/><path class="circle" id="circleHumidity" stroke-dasharray="0, 100" d="M18 2.08a15.92 15.92 0 0 1 0 31.84a15.92 15.92 0 0 1 0-31.84"/></svg>
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
            fetch('/toggle_pump');
        }

        function updateSensorData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    // Atualiza o botão da bomba
                    const pumpBtn = document.getElementById('pumpButton');
                    pumpBtn.classList.toggle('pump-on', data.pumpState);
                    pumpBtn.classList.toggle('pump-off', !data.pumpState);

                    // Atualiza o medidor de umidade do solo
                    const moisturePercent = 100 - ((data.soilMoisture / 4095) * 100);
                    document.getElementById('soilValue').textContent = moisturePercent.toFixed(0) + '%';
                    document.getElementById('circleSoil').setAttribute('stroke-dasharray', moisturePercent.toFixed(0) + ', 100');
                    
                    // NOVO: Atualiza o medidor de temperatura
                    const tempPercent = (data.temperature / 50) * 100; // Mapeia 0-50°C para 0-100%
                    document.getElementById('tempValue').textContent = data.temperature.toFixed(1) + '°C';
                    document.getElementById('circleTemp').setAttribute('stroke-dasharray', tempPercent.toFixed(0) + ', 100');

                    // NOVO: Atualiza o medidor de umidade do ar
                    document.getElementById('humidityValue').textContent = data.humidity.toFixed(1) + '%';
                    document.getElementById('circleHumidity').setAttribute('stroke-dasharray', data.humidity.toFixed(0) + ', 100');
                })
                .catch(error => console.error('Erro ao buscar dados:', error));
        }
        
        setInterval(updateSensorData, 2000);
        window.onload = updateSensorData;
    </script>
</body>
</html>
)rawliteral";


// --- Funções do Servidor Web ---

// Envia a página web principal para o cliente
void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
}

// Envia os dados de estado e sensores em formato JSON
void handleData(AsyncWebServerRequest *request) {
    // Lê os sensores
    soilMoistureValue = analogRead(SOIL_PIN);
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    // Checa por falhas de leitura no DHT e mantém os valores antigos se falhar
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Falha ao ler do sensor DHT!");
    }

    String json = "{";
    json += "\"pumpState\":" + String(pumpState ? "true" : "false") + ",";
    json += "\"soilMoisture\":" + String(soilMoistureValue) + ",";
    json += "\"temperature\":" + String(temperature) + ","; // NOVO
    json += "\"humidity\":" + String(humidity);             // NOVO
    json += "}";
    request->send(200, "application/json", json);
}

// Alterna o estado do relé (bomba)
void handleTogglePump(AsyncWebServerRequest *request) {
    pumpState = !pumpState;
    digitalWrite(RELAY_PIN, pumpState);
    request->send(200, "text/plain", "OK");
}


// --- Função Principal de SETUP ---
void setup() {
    Serial.begin(115200);
    dht.begin(); // NOVO: Inicia o sensor DHT

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, pumpState); 

    // Conecta ao Wi-Fi
    WiFi.begin(SSID, PASS);
    Serial.printf("\n[WiFi] Conectando a %s", SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\n[WiFi] Conectado!");
    Serial.print("[IP] Endereço: ");
    Serial.println(WiFi.localIP());

    // Configura as rotas do servidor web
    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.on("/toggle_pump", HTTP_GET, handleTogglePump);
    
    // Inicia o servidor
    server.begin();
    Serial.println("[SERVER] Servidor Web iniciado.");
}

// --- Função Principal de LOOP ---
void loop() {
    // O loop pode ficar vazio.
}