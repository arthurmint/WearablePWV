// pwvdissertation.ino
#include <WiFi.h>
#include <Arduino.h>
#include "../include/pinout.h"


void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_LED, OUTPUT);
  Serial.println();
}
void loop() {
  digitalWrite(PIN_LED, HIGH);
  Serial.println("LED ON!");
  delay(1000);
  digitalWrite(PIN_LED, LOW);
  Serial.println("LED OFF!");
  delay(1000);
  
}
