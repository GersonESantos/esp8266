#include <Arduino.h>
#include <WiFi.h>


#include <ESPAsyncWebServer.h>
#define SSID       "BRUGER_2G"
#define PASS       "Gersones68"


const int RELAY_PIN = 12;  // Relay for pump (ASSUMINDO agora Active HIGH para a correção desejada)
const int  SOIL_PIN  = 34;   // Soil sensor ADC pin SOIL_PIN = SENSOR_PIN


int sensorValue = 0;


AsyncWebServer server(80);

const char index_html[] PROGMEN = R"rawliteral(
 <!DOCTYPE html>
<html lang="en">
<head>
    <title>ESP32 Wi-Fi Web Server </title>
     <script>
        function sendCommand(command){
            fetch(`/led?syayr=` + command);
        }
        function updateSensor() {
          fetch(`/sensor`)
          .then(response => response.text())
          .then(data => {
            document.getElementById("sensorValue").innerText = data;
          });
          }
          setInterval(updateSensor, 2000);
    </script>
</head>
<body>
  <h1>ESP32 Wi-Fi Web Server</h1>
  <p>Click Para Controlar LED: </p>
  <button onclick="sendComand(`on`)">Turn On</button>
  <button onclick="sendComand(`off`)">Turn Off</button>
  <p>Sensor Value: <span id="sensorValue">Loading...</span></p>
</body>
</html> 
)rawliteral";


String getSemsorData(){
  sensorValue = analogRead(SOIL_PIN);
  return String(sensorValue);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  WiFi.begin(SSID, PASS);

  Serial.printf("[WiFi]: Conectando a %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nconectado! to WiFi");
  Serial.println(WiFi.localIP());

server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html);
});



server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request){
  if (request->hasParam("state")) {
    String state = request->getParam("state")->Value();
    if (state == "on"){
      digitalWrite(RELAY_PIN HIGH); 
    } else {
      digitalWrite(RELAY_PIN LOW); 
    }
  }
  request->send(200, "text/pkain", "LED updated");
});


server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
 request->send(200, "text/plain", getSemsorData().c_str());
});


server.begin();
}


void loop() {
  // put your main code here, to run repeatedly:

}
