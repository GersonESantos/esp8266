

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);

}

void loop() {
  
    int PINO_SENSOR_UMIDADE = analogRead(34);
    Serial.println(PINO_SENSOR_UMIDADE);
    delay(1000);
}
