#include "Arduino.h"
#include "Wire.h"

#define DEBUG_OUTPUT
// Datasheet for reference https://www.analog.com/media/en/technical-documentation/data-sheets/adpd1080-1081.pdf
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

        bool read_register(uint8_t addr, uint8_t *reg) {

            Wire.beginTransmission(i2c_saddr + 0);
            
            Wire.write(addr);
            Wire.write(i2c_saddr + 1);

            reg[1] = Wire.read();
            reg[0] = Wire.read();
        
            return (Wire.endTransmission() == 0) ? 0 : 0;
        }

    public:
        
        void begin(int SDA, int SCL, int GPIO0, int GPIO1) {
            uint8_t error;

            Wire.begin(SDA, SCL, clock_speed);

            Wire.beginTransmission(i2c_saddr);
            error = Wire.endTransmission();

            if (error == 0) Serial.println("ADPD1080 Initialised");
        };

        void test() {
            uint8_t reg[2];

            if (read_register(0x10, reg)) {
                Serial.println("Error checking the register");
            } else {
                Serial.print("Register state: ");
                Serial.print(reg[1], BIN);
                Serial.print(" ");
                Serial.println(reg[0], BIN);
            }
            return;
        };
};