#include <Arduino.h>

#include "../include/pinout.h"
#include "../include/adpd1080.h"
#define TEST_FOR_I2C
ADPD1080 AFE;

void setup() { 
  Serial.begin(115200);
  delay(1000);
  AFE.begin(PIN_SDA, PIN_SCL, PIN_GPIO0, PIN_GPIO1);
  delay(1000);
  Serial.println("Initialised ADPD1080");
}
void loop() {
  #ifndef TEST_FOR_I2C
  Serial.println("Testing ADPD1080");
  
  AFE.begin(PIN_SDA, PIN_SCL, PIN_GPIO0, PIN_GPIO1);
  delay(1000);
  #endif

  #ifdef TEST_FOR_I2C
  byte error, address;
  int nDevices = 0;

  delay(5000);

  Serial.println("Scanning for I2C devices ...");
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("I2C device found at address 0x%02X\n", address);
      nDevices++;
    } else if (error != 2) {
      Serial.printf("Error %d at address 0x%02X\n", error, address);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
  }
  #endif
}

