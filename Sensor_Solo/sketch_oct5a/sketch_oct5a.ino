#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// --- Suas credenciais de Wi-Fi ---
#define SSID "BRUGER_2G"
#define PASS "Gersones68"

// --- Pinos ---
const int RELAY_PIN = 12; // Pino do relé para a bomba (Active HIGH)
const int SOIL_PIN  = 34; // Pino do sensor de umidade do solo

// --- Variáveis Globais ---
int sensorValue = 0;
AsyncWebServer server(80);

// --- Página HTML ---
// O código HTML é armazenado na memória de programa para economizar RAM
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Web Server</title>
    <script>
        // Função para enviar comandos para o relé (ligar/desligar)
        function sendCommand(command) {
            // <-- CORREÇÃO: O nome do parâmetro agora é "state" para corresponder ao código C++.
            fetch(`/relay?state=` + command);
        }

        // Função para buscar e atualizar o valor do sensor na página
        function updateSensor() {
            fetch(`/sensor`)
                .then(response => response.text())
                .then(data => {
                    document.getElementById("sensorValue").innerText = data;
                });
        }

        // Atualiza o valor do sensor a cada 2 segundos
        setInterval(updateSensor, 2000);
    </script>
</head>
<body>
    <h1>ESP32 Wi-Fi Web Server</h1>
    <p>Clique para controlar a bomba:</p>
    <button onclick="sendCommand('on')">Ligar Bomba</button>
    <button onclick="sendCommand('off')">Desligar Bomba</button>
    <p>Valor do Sensor de Umidade: <span id="sensorValue">Loading...</span></p>
</body>
</html>
)rawliteral";

// --- Funções ---

// <-- CORREÇÃO: Nome da função corrigido de "getSemsorData" para "getSensorData"
String getSensorData() {
    sensorValue = analogRead(SOIL_PIN);
    return String(sensorValue);
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Garante que o relé comece desligado

    // Conecta ao Wi-Fi
    WiFi.begin(SSID, PASS);
    Serial.printf("[WiFi]: Conectando a %s", SSID);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nConectado ao WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // --- Rotas do Servidor Web ---

    // <-- CORREÇÃO: A rota principal "/" agora serve a página HTML.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    // <-- CORREÇÃO: Rota renomeada para "/relay" para clareza.
    server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("state")) {
            String state = request->getParam("state")->value(); // <-- CORREÇÃO: "value()" em minúsculas.
            if (state == "on") {
                digitalWrite(RELAY_PIN, HIGH); // <-- CORREÇÃO: Sintaxe correta com vírgula.
            } else {
                digitalWrite(RELAY_PIN, LOW); // <-- CORREÇÃO: Sintaxe correta com vírgula.
            }
            // <-- CORREÇÃO: MIME type corrigido de "pkain" para "plain"
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });

    // Rota para fornecer os dados do sensor
    server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
        // <-- CORREÇÃO: Chamando a função com o nome corrigido.
        request->send(200, "text/plain", getSensorData().c_str());
    });

    // Inicia o servidor
    server.begin();
    Serial.println("Servidor HTTP iniciado.");
}

void loop() {
    // O loop pode ficar vazio, pois o servidor assíncrono trabalha em background.
}