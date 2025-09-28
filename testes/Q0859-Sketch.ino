#include <Servo.h>

#define pinLeitura 7
#define pinServo 9
#define pinPWM 6

#define pinPot A0

Servo meuServo;

void setup() {
  meuServo.attach(pinServo);
  
  Serial.begin(9600);
  pinMode(pinLeitura, INPUT_PULLUP);
  pinMode(pinPWM, OUTPUT);
}

void loop() {
  int valorLido = analogRead(pinPot);
  meuServo.write(map(valorLido, 0, 1023, 0, 180));
  analogWrite(pinPWM, map(valorLido, 0, 1023, 0, 255));
  
  unsigned long duracaoLOW = pulseIn(pinLeitura, LOW, 50000);
  Serial.print("L:");
  Serial.print(duracaoLOW);
  Serial.print(" ");

  unsigned long duracaoHIGH = pulseIn(pinLeitura, HIGH, 50000);
  Serial.print("H:");
  Serial.print(duracaoHIGH);
  Serial.print(" ");

  Serial.print("L+H:");
  Serial.println(duracaoLOW + duracaoHIGH);  
}
