#include "Arduino.h"
#include "Wire.h"
#include "register_listings.h"

#define DEBUG_OUTPUT
// Datasheet for reference https://www.analog.com/media/en/technical-documentation/data-sheets/adpd1080-1081.pdf
class ADPD1080 {
    private:
        // i2c 
        uint8_t i2c_saddr = 0x64; // default 7-bit slave address
        uint8_t i2c_waddr = 0xC8; // default write address
        uint8_t i2c_raddr = 0xC9; // default read address 
        const uint32_t clock_speed = 100E+3;

        bool read_register(uint8_t addr, uint8_t *reg) {
            uint8_t n_bytes;

            Wire.beginTransmission(i2c_saddr);
            
            Wire.write(addr);
            
            Wire.endTransmission();

            n_bytes = Wire.requestFrom(i2c_saddr, (uint8_t) 2);

            if (n_bytes == 2) {
                reg[1] = Wire.read();
                reg[0] = Wire.read();

                #ifdef DEBUG_OUTPUT
                Serial.print("Read: ");
                Serial.print(reg[1], BIN);
                Serial.print(" ");
                Serial.print(reg[0], BIN);
                Serial.print(" at address ");
                Serial.println(addr, HEX);
                #endif
                return false;
            }
            
            return true;
        }

        bool write_register(uint8_t addr, uint16_t data) {
            uint8_t b1, b2, error;
            b1 = data >> 8;
            b2 = data & 0b11111111;
            Wire.beginTransmission(i2c_saddr);
            
            Wire.write(addr);
            Wire.write(b1);
            Wire.write(b2);
            
            error = Wire.endTransmission();

            #ifdef DEBUG_OUTPUT
            if (!error) {
                Serial.print("Wrote ");
                Serial.print(b1, BIN);
                Serial.print(b2, BIN);
                Serial.print(" to address ");
                Serial.println(addr, HEX);
            } else Serial.println("An error occured when trying to write data");
            #endif 

            return error;
        }
    public:
        // Values of Register 0x10 which determine the state
        enum device_mode : uint16_t {
            STANDBY = 0x0000, // Ultralow power mode, no data collection, all register values are retained
            PROGRAM = 0x0001, // Safe mode for programming registers, no data collection, device is fully powered
            OPERATION = 0x0002 // Normal operation, LEDs and PDs are sampled, power is cycled by internal FSM
        };

        void begin(int SDA, int SCL, int GPIO0, int GPIO1) {
            uint8_t error;

            Wire.begin(SDA, SCL, clock_speed);

            Wire.beginTransmission(i2c_saddr);
            error = Wire.endTransmission();

            #ifdef DEBUG_OUTPUT
            if (error == 0) Serial.println("ADPD1080 Initialised without error");
            else Serial.println("An error occured when trying to initialise ADPD1080");
            #endif
        };

        void setMode(device_mode mode) {
            write_register(MODE, mode);
            #ifdef DEBUG_OUTPUT
            switch (mode) {
                case STANDBY:
                    Serial.println("Set ADPD1080 to Standby");
                    break;
                case PROGRAM:
                    Serial.println("Set ADPD1080 to Program");
                    break;
                case OPERATION:
                    Serial.println("Set ADPD1080 to Normal operation");
                    break;
            }
            #endif
        }

        void reset() {
            if (write_register(SW_RESET, 0x1)) {
                #ifdef DEBUG_OUTPUT
                Serial.println("Error resetting ADPD1080");
                #endif
            } else {
                #ifdef DEBUG_OUTPUT
                Serial.println("Successfully reset ADPD1080");
                #endif
            }
        }

        bool clk32k_en(bool value) {
            uint8_t check[2];
            write_register(SAMPLE_CLK, (value << 7) & 0b10000000);

            read_register(SAMPLE_CLK, check);
            if ((check[0] >> 7) & 0b1 == value) {
                return 0; // Succesfully changed value
            } else return 1;
        }

        uint8_t getMode() {
            uint8_t reg[2];
            bool error = 1;

            while (error) {
                error = read_register(MODE, reg);
            }
            return reg[0];
        }
};