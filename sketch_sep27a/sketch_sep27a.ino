#include <DHT.h>
#include <ESP8266WiFi.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define SOIL_PIN A0
#define RAIN_PIN 3
#define RELAY_PIN 4

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA";

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(SOIL_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  WiFi.begin(ssid, password);
}

void loop() {
  float temp = dht.readTemperature();
  int soilMoisture = analogRead(SOIL_PIN);
  int rain = digitalRead(RAIN_PIN);

  if (soilMoisture < 300 && rain == HIGH) { // Solo seco e sem chuva
    digitalWrite(RELAY_PIN, HIGH); // Liga irrigação
    delay(600000); // 10 minutos
    digitalWrite(RELAY_PIN, LOW);
  }

  // Enviar dados para ThingSpeak (exemplo simplificado)
  Serial.print("Temp: "); Serial.println(temp);
  Serial.print("Umidade Solo: "); Serial.println(soilMoisture);

  delay(3600000); // Verifica a cada hora
}