#define PS4_R2 4
#define PS4_L2 5
#define PS4_JOYSTICK_LEFT_X 3
#define MOTOR_ENABLE_PIN 6
#define FRAM_PIN 4
#define BAK_PIN 5
#define SERVO_PIN 9
#include "SoftwareSerial.h"
#include <stdio.h>
#include <stdlib.h>
#include <Servo.h>

Servo jobbigSakAttLimmaFast;
SoftwareSerial bt = SoftwareSerial(2,3);
// bör sända detta 2 gånger i rad för att starta upp första gången
#define magicPacket 'z'
bool lastPacketWasMagic = false;
void setup() {
  // put your setup code here, to run once:
  bt.begin(9600);
  Serial.begin(9600);
  Serial.println("Startar...");
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  pinMode(FRAM_PIN, OUTPUT);
  pinMode(BAK_PIN, OUTPUT);
  jobbigSakAttLimmaFast.attach(SERVO_PIN);
}
bool readingButton = true;
bool ready = false;
struct Data {
  uint8_t knapp;
  uint8_t value;
};
struct Data data;
void setHastighet(int16_t hastighet) {
  Serial.print("Hastighet: ");
  Serial.println(hastighet);
  if(hastighet < 0) {
    digitalWrite(BAK_PIN, HIGH);
    digitalWrite(FRAM_PIN, LOW);
    analogWrite(MOTOR_ENABLE_PIN,-hastighet);
  } else if(hastighet == 0) {
    analogWrite(MOTOR_ENABLE_PIN,0);

  } else {
    digitalWrite(FRAM_PIN, HIGH);
    digitalWrite(BAK_PIN, LOW);
    analogWrite(MOTOR_ENABLE_PIN,hastighet);
  }
}
void handlePacket() {
  char* msg = (char*)malloc(100);
  snprintf(msg, 99, "Knapp: %X. Value: %X", data.knapp, data.value);
  Serial.println(msg);
  free(msg);
  switch(data.knapp) {
    case PS4_R2:
      setHastighet(data.value);
      break;
    case PS4_L2:
      setHastighet(-data.value);
      break;
    case PS4_JOYSTICK_LEFT_X:
      int grader = (data.value/3)+100;
      jobbigSakAttLimmaFast.write(grader);
      delay(20);
      break;
  }
}
void loop() {
  // put your main code here, to run repeatedly:
  if(bt.available() > 0) {
    char val = bt.read();
    Serial.print(val);
    if(val == magicPacket) {
      if(lastPacketWasMagic) {
        ready = true;
        lastPacketWasMagic = false;
        readingButton = true;
      } else {
        lastPacketWasMagic = true;
      }
      return;
    }
    if(ready) {
      if(readingButton) {
        data.knapp = val;
        readingButton = false;
      }
      else {
        data.value = val;
        handlePacket();
        readingButton = true;
      }
    }
  }
}
