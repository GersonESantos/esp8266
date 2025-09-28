#include <DHT.h>            // Biblioteca para DHT22
#include <ESP8266WiFi.h>    // Biblioteca para ESP8266
#include <ThingSpeak.h>     // Biblioteca para ThingSpeak

// Pinos
#define DHTPIN 2            // Pino do DHT22
#define DHTTYPE DHT22       // Tipo do sensor
#define SOIL_PIN A0         // Pino do sensor de umidade do solo
#define RAIN_PIN 3          // Pino do sensor de chuva
#define RELAY_PIN 4         // Pino do relé

// Configurações Wi-Fi e ThingSpeak
const char* ssid = "BRUGER_2G";         // Substitua pelo nome da rede
const char* password = "Gersones68";    // Substitua pela senha
const char* apiKey = "SUA_API_KEY";         // API Key do ThingSpeak
const long channelID = SEU_CHANNEL_ID;      // ID do canal no ThingSpeak

// Inicializações
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void setup() {
  Serial.begin(9600);          // Inicia comunicação serial
  dht.begin();                 // Inicia o DHT22
  pinMode(SOIL_PIN, INPUT);    // Configura sensor de solo como entrada
  pinMode(RAIN_PIN, INPUT);    // Configura sensor de chuva como entrada
  pinMode(RELAY_PIN, OUTPUT);  // Configura relé como saída
  digitalWrite(RELAY_PIN, LOW); // Relé inicia desligado

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao Wi-Fi!");
  
  ThingSpeak.begin(client);    // Inicia ThingSpeak
}

void loop() {
  // Leitura dos sensores
  float temp = dht.readTemperature();         // Temperatura em °C
  float humidity = dht.readHumidity();        // Umidade do ar em %
  int soilMoisture = analogRead(SOIL_PIN);    // Umidade do solo (0-1023)
  int rain = digitalRead(RAIN_PIN);           // Chuva (0 = chuva, 1 = sem chuva)

  // Verifica se as leituras são válidas
  if (isnan(temp) || isnan(humidity)) {
    Serial.println("Erro ao ler o DHT22!");
    return;
  }

  // Lógica de irrigação
  if (soilMoisture < 300 && rain == 1) { // Solo seco (ajuste o valor conforme o sensor) e sem chuva
    digitalWrite(RELAY_PIN, HIGH);       // Liga a irrigação
    Serial.println("Irrigação ligada!");
    delay(600000);                       // 10 minutos (600000 ms)
    digitalWrite(RELAY_PIN, LOW);        // Desliga a irrigação
    Serial.println("Irrigação desligada!");
  } else {
    Serial.println("Irrigação não necessária.");
  }

  // Envio de dados ao ThingSpeak
  ThingSpeak.setField(1, temp);          // Campo 1: Temperatura
  ThingSpeak.setField(2, humidity);      // Campo 2: Umidade do ar
  ThingSpeak.setField(3, soilMoisture);  // Campo 3: Umidade do solo
  ThingSpeak.setField(4, rain);          // Campo 4: Estado da chuva

  int response = ThingSpeak.writeFields(channelID, apiKey);
  if (response == 200) {
    Serial.println("Dados enviados ao ThingSpeak com sucesso!");
  } else {
    Serial.println("Erro ao enviar dados: " + String(response));
  }

  // Exibe dados no monitor serial
  Serial.print("Temperatura: "); Serial.print(temp); Serial.println("°C");
  Serial.print("Umidade Ar: "); Serial.print(humidity); Serial.println("%");
  Serial.print("Umidade Solo: "); Serial.println(soilMoisture);
  Serial.print("Chuva: "); Serial.println(rain == 0 ? "Sim" : "Não");

  delay(3600000); // Aguarda 1 hora (3600000 ms) para próxima leitura
}