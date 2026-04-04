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
      AFE.configurePPG(false);
      #ifdef DEBUG_OUTPUT
      Serial.println("PPG Configuration Compeleted!");
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
  delay(10000);
  startMeasurement();
}
void loop() {
  uint32_t data;
  uint16_t available_data;
  uint16_t pd1, pd2, pd3, pd4;

  available_data = ((AFE.getStatus() >> 8) & 0xFF);
  Serial.print("Available data: ");
  Serial.print(available_data);
  if (available_data >= 4) {
    AFE.readFIFO(&data, 4);

    Serial.print(" DATA: ");
    Serial.print(data);

    if (!AFE.readPPG(&pd1, &pd2, &pd3, &pd4)) {
        Serial.print(" PD1: "); Serial.print(pd1);
        Serial.print(" PD2: "); Serial.print(pd2);
        Serial.print(" PD3: "); Serial.print(pd3);
        Serial.print(" PD4: "); Serial.println(pd4);
    } else {
        Serial.println("Read error");
    }
    
  } else {
    Serial.println("Not enough data to read");
  }

  if (interrupt) {
    AFE.readFIFO(&data, 4);
    
    Serial.print(" DATA: ");
    Serial.println(data);
    
    interrupt = false;
  }
  delay(10);


  
  
  
  
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

