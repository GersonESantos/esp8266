// Define o pino onde o LED está conectado
const int ledPin = 4;  // GPIO 4 corresponde ao pino D2 na ESP8266

void setup() {
  // Configura o pino do LED como saída
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Liga o LED
  digitalWrite(ledPin, HIGH);
  // Aguarda 0,5 segundos
  delay(500);
  // Desliga o LED
  digitalWrite(ledPin, LOW);
  // Aguarda mais 0,5 segundos
  delay(5000);
}
