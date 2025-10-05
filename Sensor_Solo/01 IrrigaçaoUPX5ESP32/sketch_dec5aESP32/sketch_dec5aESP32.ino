// Alterar estes valores baseando-se no seu projeto e nos seus testes ;)
const int rele = 12;
const int seco = 686;
const int molhado = 360;
const int tempoDeRega = 1000; // Equivale a 4min
// ------------------------------------------------
 
void setup() {
   Serial.begin(115200);
   pinMode(rele, OUTPUT);
}

void loop() {
  //Lendo valor do sensor
  int valorSensor = analogRead(34);
  
  //Mapeando o valor do sensor para novos valores mais compreensiveis
  int porcentagem = map(valorSensor, molhado, seco, 100, 0);
  Serial.println(porcentagem);
  String estadoSolo = (porcentagem < 70) ? "WET" : "DRY";
  Serial.print(" | Estado: ");
  Serial.println(estadoSolo);
  if(porcentagem >= 70){ // Nao rega
    digitalWrite(rele, HIGH);
    Serial.println("Status: Solo seco. Bomba LIGADA.");
    delay(1000);
   }
   else{
    digitalWrite(rele, LOW); // Inicia a rega e espera 4 min
    Serial.println("Status: Solo úmido. Bomba DESLIGADA.");
    delay(tempoDeRega);
   }
   
}
