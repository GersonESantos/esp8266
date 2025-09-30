// Define os valores de calibração do sensor
const int seco = 686;    // Valor lido com o sensor no ar (seco)
const int molhado = 360; // Valor lido com o sensor na água (molhado)

// Define os pinos de hardware
const int PINO_SENSOR_UMIDADE = 34;
const int PINO_RELE = 12;

// Define o limite para acionar a irrigação
const int LIMITE_UMIDADE_MINIMA = 30; // Em porcentagem (%). O relé liga se a umidade for menor que isso.

void setup() {
  // Inicia a comunicação serial para vermos os resultados no Monitor Serial
  Serial.begin(115200);

  // Configura o pino do relé como saída
  pinMode(PINO_RELE, OUTPUT);
  // Garante que o relé comece desligado
  digitalWrite(PINO_RELE, LOW); 
}

void loop() {
  // 1. Lê o valor bruto (0-4095) do pino do sensor
  int leituraBruta = analogRead(PINO_SENSOR_UMIDADE);
  
  // 2. Mapeia o valor lido para uma porcentagem de 0 a 100
  int perct = map(leituraBruta, seco, molhado, 0, 100);

  // 3. Garante que o resultado final fique sempre entre 0 e 100
  perct = constrain(perct, 0, 100);

  // 4. Imprime o valor da umidade no Monitor Serial
  Serial.print("Umidade do solo: ");
  Serial.print(perct);
  Serial.println("%");

  // 5. Lógica de controle do relé
  if (perct < LIMITE_UMIDADE_MINIMA) {
    // Se o solo está seco, liga o relé (bomba)
    digitalWrite(PINO_RELE, HIGH);
    Serial.println("Status: Solo seco. Bomba LIGADA.");
  } else {
    // Se o solo está úmido o suficiente, desliga o relé (bomba)
    digitalWrite(PINO_RELE, LOW);
    Serial.println("Status: Solo úmido. Bomba DESLIGADA.");
  }
  
  // Espera 2 segundos antes da próxima leitura para não oscilar o relé
  delay(2000);
}