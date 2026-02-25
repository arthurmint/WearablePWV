#include "Arduino.h"
#include "Wire.h"

// Datasheet for reference https://www.analog.com/media/en/technical-documentation/data-sheets/adpd1080-1081.pdf

const uint8_t SDA_PIN = 6; // D4
const uint8_t SCL_PIN = 7; // D5

const uint8_t GPIO0 = 2; // D0
const uint8_t GPIO1 = 3; // D1

class ADPD1080 {
    private:
        // i2c 
        uint8_t i2c_saddr = 0x64; // default 7-bit slave address
        uint8_t i2c_waddr = 0xC8; // default write address
        uint8_t i2c_raddr = 0xC9; // default read address 
        const uint32_t clock_speed = 100E+3;
    

        // register addresses
        const uint8_t MODE = 0x10; // read/write to update state stored in [1:0]
        const uint8_t SLOTA_LED_OFFSET = 0x30;
        const uint8_t SLOTB_LED_OFFSET = 0x35;
        const uint8_t SLOTA_PERIOD = 0x31;
        const uint8_t SLOTB_PERIOD = 0x36;
        const uint8_t ADDR_CONFIG = 0x09; // used to reconfigure slave address
        const uint8_t KEY_SEL = 0x0D; // can be used to select a key to enable address changes in speciifc devices
        const uint8_t FIFO_ACCESS = 0x60;


        read_register(uint8_t reg_addr, uint) 

    public:
        ADPD1080(int SDA, int SCL, int GPIO0, int GPIO1) {
            Wire.setPins(SDA, SCL);

            Wire.begin();

            Wire.setClock(clock_speed);

            #ifdef DEBUG_OUTPUT
            printf("ADPD1080 Initialised\n");
            #endif
        };

    

        test() {
            Wire.
        }

};