#include "SoftwareSerial.h"
#include <stdio.h>
#include <stdlib.h>

SoftwareSerial bt = SoftwareSerial(2,3);
// bör sända detta 2 gånger i rad för att starta upp första gången
#define magicPacket 'z'
bool lastPacketWasMagic = false;
void setup() {
  // put your setup code here, to run once:
  bt.begin(9600);
  Serial.begin(9600);
  Serial.println("Startar...");
}
bool readingButton = true;
bool ready = false;
struct Data {
  uint8_t knapp;
  uint8_t value;
};
struct Data data;
void handlePacket() {
  char* msg = (char*)malloc(100);
  snprintf(msg, 99, "Knapp: %X. Value: %X", data.knapp, data.value);
  Serial.println(msg);
  free(msg);
}
void loop() {
  // put your main code here, to run repeatedly:
  if(bt.available() > 0) {
    char val = bt.read();
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
