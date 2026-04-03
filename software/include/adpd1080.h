#include "Arduino.h"
#include "Wire.h"
#include "register_listings.h"

// Datasheet for reference https://www.analog.com/media/en/technical-documentation/data-sheets/adpd1080-1081.pdf


bool interrupt = false;

void ARDUINO_ISR_ATTR int_return() {
    interrupt = true;
}

class ADPD1080 {
    private:
        // i2c 
        uint8_t i2c_saddr = 0x64; // default 7-bit slave address
        uint8_t i2c_waddr = 0xC8; // default write address
        uint8_t i2c_raddr = 0xC9; // default read address 
        const uint32_t clock_speed = 100E+3;

        // ints
        uint8_t gpio0, gpio1;

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

        bool multiword_read(uint8_t addr, uint8_t *reg, uint8_t n_words) {
            uint8_t n_bytes, i = 0;

            Wire.beginTransmission(i2c_saddr);
            
            Wire.write(addr);
            
            Wire.endTransmission();

            n_bytes = Wire.requestFrom(i2c_saddr, n_words);

            if (n_bytes == n_words) {
                for (i=0; i<n_bytes; i++) {
                    reg[n_bytes - 1 - i] = Wire.read();
                    #ifdef DEBUG_OUTPUT
                    Serial.print("Read: ");
                    Serial.print(reg[n_bytes - 1 - i], BIN);
                    #endif
                }
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

            this->gpio0 = GPIO0;
            this->gpio1 = GPIO1;

            pinMode(gpio0, INPUT_PULLDOWN);
            attachInterrupt(gpio0, int_return, CHANGE);
            pinMode(gpio1, INPUT_PULLDOWN);
            attachInterrupt(gpio1, int_return, CHANGE);
            reset();

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
            if (((check[0] >> 7) & 0b1) == value) {
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

        uint16_t getStatus() {
            uint8_t status[2];
            read_register(STATUS, status);
            return (uint16_t) (status[1] << 8) | status[0];
        }

        void setLED() {
            uint16_t data;

            data = 0xE & 0xF;
            data += (0b000 << 4) & 0b1110000;
            data += (0b00000 << 7) & 0b111110000000;
            data += (0b1 << 12) & 0b1000000000000;
            data += (0b1 << 13) & 0b10000000000000;
            data += (0b00 << 14) & 0b1100000000000000;

            write_register(ILED2_COARSE, data);
        }

        void configurePPG(bool interrupt_mode) {
            uint8_t optical_select, rbias_select;
            uint8_t num_averages = 0b000; // 2^num_averages = number of averages
            uint16_t sample_frequency = 100; // data rate = sample_frequency / num_averages

            setMode(PROGRAM);

            // Set SLOTA_EN at bit 0, SLOTA_FIFO_MODE = 1 at bits [4:2]
            write_register(SLOT_EN, 0b101);

            // Sample frequency = 32e+3 / (FSAMPLE_VALUE * 4)
            write_register(FSAMPLE, 32e+3 / (sample_frequency * 4));

            /* 
            LED X2 -> [1:0] 0x2
            PD1-4 -> [7:4] 0x5
            PD1-4 -> [11:8] 0x5 
            */
            optical_select = 0x3;
            optical_select += (0x5 << 4) & 0x00F0;
            optical_select += (0x5 << 8) & 0x0F00;
            write_register(PD_LED_SELECT, optical_select);

            write_register(NUM_AVG, (num_averages << 4) & 0xF0);

            // LED pulse settings (Default values)
            write_register(SLOTA_LED_PULSE, 0x0300);
            write_register(SLOTA_NUMPULSES, 0x0818); 

            // AFE window (integration time)
            write_register(SLOTA_AFE_WINDOW, 0x0020);

            /* 
                TIA Feedback resistor
                0: 200kΩ
                1: 100kΩ
                2: 50kΩ
                3: 25kΩ
            */
            write_register(SLOTA_TIA_CFG, ((7 << 10) & 0b1111110000000000) | ((3 << 4) & 0b110000) | ((2<<2) & 0b1100));

            /* 
                AFE connection in Time Slot A.
                0xADA5: analog full path mode (TIA_BPF_INT_ADC).
                0xAE65: TIA ADC mode (must set Register 0x42, Bit 7 = 1
                and Register 0x58, Bit 7 = 1).
                0xB065: TIA ADC mode (if Register 0x42, Bit 7 = 0).
                Others: reserved.
            */
            write_register(SLOTA_AFE_CFG, 0xADA5);

            /*
                To apply a reverse bias to the photodiode:
                Bit 7 = 1
                Bits[11:10] - SLOTB_V_CATHODE
                Bits[9:8] - SLOTA_V_CATHODE
                Set these to 0x2 for 250mV Vbias
            */

            rbias_select = 0x20;
            rbias_select += 0b10000000;
            rbias_select += (0x2 << 8) & 0b1100000000;
            rbias_select += (0x2 << 10) & 0b110000000000;
            write_register(AFE_PWR_CFG2, rbias_select);

             // Enable data access
            write_register(DATA_ACCESS_CTL, 0x0000);


            if (interrupt_mode) {
                // TODO Register 0x11 selects the kind of data from each time slot to be
                // written to the FIFO

                /*
                enum fifo_mode : uint8_t {
                    NO_DATA = 0,
                    SUM_16BIT = 1,
                    SUM_32BIT = 2,
                    QUAD_16BIT = 4,
                    QUAD_32BIT = 6
                }
                */

                // Set the interrupt bytes threshold
                write_register(FIFO_THRESH, 0x0F00);

                // Configure GPIO0 to act as int and active high
                write_register(GPIO_DRV, 0b0000000100);

                // Enable the sample interrupt by writing a 0 to the appropriate bit in register 0x01
                write_register(INT_MASK, ((0b1101 << 5) & 0b111100000) | 0x1F);

            }
        
            setLED();
            setMode(OPERATION);
        }

        void readFIFO(uint32_t *out, uint8_t len) {
            uint8_t reg[len], i=0;
            /*
            read_register(FIFO_ACCESS, reg);

            *out = (reg[1] << 8) | reg[0];
            */
            *out = 0;
            if (!multiword_read(FIFO_ACCESS, reg, len)) {
                for (i=0; i<len; i++) {
                    *out |= reg[i] << (8 * i);
                }
            } else {
                Serial.println("Error reading FIFO");
            }

        }


        bool readPPG(uint16_t *pd1, uint16_t *pd2, uint16_t *pd3, uint16_t *pd4) {
            uint8_t reg[2];

            if (read_register(SLOTA_PD1_16BIT, reg)) return true;
            *pd1 = (reg[1] << 8) | reg[0];

            if (read_register(SLOTA_PD2_16BIT, reg)) return true;
            *pd2 = (reg[1] << 8) | reg[0];

            if (read_register(SLOTA_PD3_16BIT, reg)) return true;
            *pd3 = (reg[1] << 8) | reg[0];

            if (read_register(SLOTA_PD4_16BIT, reg)) return true;
            *pd4 = (reg[1] << 8) | reg[0];

            return false;
        }
};