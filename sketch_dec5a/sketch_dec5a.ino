// Alterar estes valores baseando-se no seu projeto e nos seus testes ;)
const int rele = 5;
const int seco = 686;
const int molhado = 360;
const int tempoDeRega = 2400; // Equivale a 4min
// ------------------------------------------------
 
void setup() {
   Serial.begin(9600);
   pinMode(rele, OUTPUT);
}

void loop() {
  //Lendo valor do sensor
  int valorSensor = analogRead(A0);
  
  //Mapeando o valor do sensor para novos valores mais compreensiveis
  int porcentagem = map(valorSensor, molhado, seco, 100, 0);
  Serial.println(porcentagem);

  if(porcentagem >= 70){ // Nao rega
    digitalWrite(rele, HIGH);
    delay(1000);
   }
   else{
    digitalWrite(rele, LOW); // Inicia a rega e espera 4 min
    delay(tempoDeRega);
   }
   
}
