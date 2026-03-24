#include <Arduino.h>

#include "../include/pinout.h"
#include "../include/adpd1080.h"

ADPD1080 AFE;

/* Project specific functions -- define measurement procedure */

bool startMeasurement() {
  if (AFE.getMode() == AFE.STANDBY) {
    if (!AFE.clk32k_en(1)) {
      #ifdef DEBUG_OUTPUT
      Serial.println("CLK ENABLED");
      #endif
      AFE.setMode(AFE.PROGRAM);
      #ifdef DEBUG_OUTPUT
      Serial.println("Set mode to PROGRAM");
      #endif
    }
    return 0;
  } else if (AFE.getMode() == AFE.PROGRAM) {
    AFE.setLED();
    AFE.setMode(AFE.OPERATION);
    #ifdef DEBUG_OUTPUT
    Serial.println("Set LED current");
    #endif
  } else {
    #ifdef DEBUG_OUTPUT
    Serial.println("Unable to start measurement while the device is not in STANDBY mode");
    #endif
    return 1;
  }
 
}



void setup() { 
  Serial.begin(115200);
  delay(1000);
  AFE.begin(PIN_SDA, PIN_SCL, PIN_GPIO0, PIN_GPIO1);
  delay(1000);
  Serial.println("Initialised ADPD1080");
}
void loop() {
  startMeasurement();
  Serial.println("Waiting...");
  delay(1000);
  if (AFE.getMode() == AFE.OPERATION) {
    AFE.reset();
  }
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

