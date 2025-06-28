#include <PS2X_lib.h>  //for v1.6
#include <Servo.h>
PS2X ps2x;
int PS2 = 0; 
Servo LXservo;


const int in1 = 2;    // direction pin 1
const int in2 = 4;    // direction pin 2
const int ena = 3;    // PWM pin to change speed

int fspeed;           // forward speed 

void setup(){
  LXservo.attach(6);
  PS2 = ps2x.config_gamepad(13,11,10,12, true, true);   //Настройка выводов: (clock, command, attention, data, true, true)
  pinMode(in1, OUTPUT);      // connection to L298n
  pinMode(in2, OUTPUT);      // connection to L298n
  pinMode(ena, OUTPUT);      // connection to L298n
  pinMode(8, OUTPUT);   // Фары
  pinMode(9, OUTPUT);   // Стопаки
  pinMode(5, OUTPUT);   // Стопаки от кнопки
  pinMode(1, OUTPUT);   // Поворот на лево
  pinMode(7, OUTPUT);   // Поворот на право
}



void loop(){
  ps2x.read_gamepad();
  
  if(ps2x.ButtonPressed(PSB_CIRCLE)) digitalWrite(8, HIGH);
  if(ps2x.ButtonPressed(PSB_TRIANGLE)) digitalWrite(8, LOW);
  
  
  if(ps2x.ButtonPressed(PSB_SQUARE)) digitalWrite(5, HIGH);
  if(ps2x.ButtonPressed(PSB_CROSS)) digitalWrite(5, LOW);
  
 // if(ps2x.ButtonPressed(PSB_R1)) digitalWrite(7, HIGH);
  // if(ps2x.ButtonPressed(PSB_R2)) digitalWrite(7, LOW);
 
  
  if (ps2x.Analog(PSS_LY) == 128) analogWrite(ena, 0);
  

  if (ps2x.Analog(PSS_LY) > 128){
    fspeed = map(ps2x.Analog(PSS_LY), 129, 255, 0, 255);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(ena, fspeed);
    digitalWrite(9, HIGH);
  }
  
  if (ps2x.Analog(PSS_LY) < 128){
    fspeed = map(ps2x.Analog(PSS_LY), 0, 127, 255, 0);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(ena, fspeed);
    digitalWrite(9, LOW);
  }
  
  LXservo.write(map(ps2x.Analog(PSS_RX), 0, 255, 65, 105)); 
   
    if (ps2x.Analog(PSS_RX) < 120){
    digitalWrite(1, HIGH);
    digitalWrite(7, LOW);
  }
  
    if (ps2x.Analog(PSS_RX) > 150){
    digitalWrite(7, HIGH);
    digitalWrite(1, LOW);
  }  
  
      if (ps2x.Analog(PSS_RX) > 120 && ps2x.Analog(PSS_RX) < 150){
    digitalWrite(1, LOW);
    digitalWrite(7, LOW);
  }
  
  delay(50);

}    
