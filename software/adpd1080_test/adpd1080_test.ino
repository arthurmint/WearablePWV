//working code
#include <Arduino.h>

#include "../include/pinout.h"
#include "../include/adpd1080.h"

ADPD1080 AFE;

/* Project specific functions -- define measurement procedure */

bool startMeasurement() {
  if (AFE.getMode() == AFE.STANDBY) {
    if (!AFE.clk32k_en(1)) {
      AFE.configurePPG(false); // configures and sets OPERATION internally
      // Now flush the FIFO after configuration
      AFE.setMode(AFE.PROGRAM);
      delay(10);
      AFE.setMode(AFE.OPERATION);
    }
    return false;

  } else if (AFE.getMode() == AFE.PROGRAM) {
    AFE.setMode(AFE.OPERATION);
    return false;

  } else {
    #ifdef DEBUG_OUTPUT
    Serial.println("Device not in STANDBY, cannot start measurement");
    #endif
    return true;
  }
}

void setup() { 
    Serial.begin(9600);
    delay(1000);
    
    AFE.begin(PIN_SDA, PIN_SCL, PIN_GPIO0, PIN_GPIO1);
    delay(500);
    
    AFE.printDevID(); // must be after begin()
    
    startMeasurement();
    AFE.readbackConfig(); 
  Serial.println("Initialised ADPD1080");
  //delay(10000);
}

void loop() {
  uint32_t data;
  uint16_t pd1, pd2, pd3, pd4;

  uint16_t available_data = (AFE.getStatus() >> 8) & 0xFF;
  //Serial.print("Available data: ");
  //Serial.print(available_data);

  if (available_data >= 4) {
    AFE.readFIFO(&data, 4);
    
    Serial.println(data);
    /*
    if (!AFE.readPPG(&pd1, &pd2, &pd3, &pd4)) {
      Serial.print(" PD1: "); Serial.print(pd1);
      Serial.print(" PD2: "); Serial.print(pd2);
      Serial.print(" PD3: "); Serial.print(pd3);
      Serial.print(" PD4: "); Serial.println(pd4);
    } else {
      Serial.println("Read error");
    }*/
  } else {
    //Serial.println(" - Not enough data");
  }

  delay(100);
}

