#include "Arduino.h"
#include "Wire.h"
#include "register_listings.h" // contains register listings from the ADPD1080 data sheet

// Datasheet for reference regarding measurement, register reading and addressing info
// https://www.analog.com/media/en/technical-documentation/data-sheets/adpd1080-1081.pdf



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


        enum led_select : uint8_t {
                IR = 0x1,
                RED = 0x2,
                GREEN = 0X3
        };

        /*
        @brief Initialises the ADPD1080 with input pin numbers, attaches interrupt pins and tests I2C communication
        @param SDA I2C SDA
        @param SCL I2C SCL
        @param GPIO0 Interrupt pin 
        @param GPIO1 Additional interrupt pin
        */
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

        /*
        @brief Handles the state of ADPD1080 as described in the datasheet
        @param mode can be either STANDBY, PROGRAM or NORMAL
        */
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

        /*
        @brief Performs a software reset on the ADPD1080
        */
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

        /*
        @brief Configures the 32kHz clock to be the input value, returning 0 if successful
        @param value Boolean enables/disables the clock
        */
        bool clk32k_en(bool value) {
            uint8_t check[2];
            write_register(SAMPLE_CLK, (value << 7) & 0b10000000);

            read_register(SAMPLE_CLK, check);
            if (((check[0] >> 7) & 0b1) == value) {
                return 0; // Succesfully changed value
            } else return 1;
        }

        /*
        @brief Quickly returns the current mode of the device via register access
        */
        uint8_t getMode() {
            uint8_t reg[2];
            bool error = 1;

            while (error) {
                error = read_register(MODE, reg);
            }
            return reg[0];
        }

        /*
        @brief Reads the STATUS register (0x00), [15:8] corresponds to the number of available bytes of FIFO data to be read
        */
        uint16_t getStatus() {
            uint8_t status[2];
            read_register(STATUS, status);
            return (uint16_t) (status[1] << 8) | status[0];
        }

        // TODO make this dynamic -_-
        /*
        @brief Sets the LED Coarse current to a 16-bit value, hardcoded for now 
        @param LEDxx corresponds to the LED pin which can be 1, 2 or 3
        @param led_coarse LEDXX coarse current setting. Coarse current sink target value of LEDXX in standard operation. 0x0: lowest coarse setting … 0xF: highest coarse setting
        @param led_slew LEDXX driver slew rate control from 0 to 7, the slower the slew rate, the safer the performance in terms of reducing the risk of overvoltage of LED driver
        @param led_scale LEDXX current scale factor, 0 (40%) or 1 (100%)
        */
        void setLED(uint8_t LEDxx, uint16_t led_coarse, uint16_t led_slew, uint16_t led_scale) {
            uint16_t data;
        
            data = led_coarse | ((led_slew << 4) & 0b1110000) | 4096 | ((led_scale << 13) & 8192);

            switch (LEDxx) {
                case 1:
                    write_register(ILED1_COARSE, data);
                    break;
                case 2:
                    write_register(ILED2_COARSE, data);
                    break;
                case 3:
                    write_register(ILED3_COARSE, data);
                    break;
                default:
                    break;
            }

            
        }

        // TODO make this function more readable
        /*
        @brief Handles the initial configuration of the ADPD1080 in either interrupt mode or polling mode. Put this in setup()
        @param interrupt_mode Determines how the FIFO will be read (on interrupt or via polling)
        */
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


            // Refer to Table 16 of the datasheet
            enum pd_select {
                NC = 0,
                CH1_3_4_CH2_1_2 = 1,
                CH1_7_8_CH2_5_6 = 2,
                CH1_1_TO_4 = 3,
                CH_ALL_5_TO_8 = 4,
                CH_ALL_1_TO_4 = 5,
                CH1_3_4_CH2_5_6 = 6,
                CH1_5_TO_8 = 7
            };
            
            optical_select = GREEN | ((CH_ALL_1_TO_4 << 4) & 0x00F0) | ((CH_ALL_1_TO_4 << 8) & 0x0F00);

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
            write_register(TIA_INDEP_GAIN, 0x0000);


            /*
                Must set bit 6 = 1
                Must set bit 7 = 1 for TIA ADC mode
            */

            write_register(SLOTA_TIA_CFG, (0x07 << 10) | 0b0011111000);
            /* 
                AFE connection in Time Slot A.
                0xADA5: analog full path mode (TIA_BPF_INT_ADC).
                0xAE65: TIA ADC mode (must set Register 0x42, Bit 7 = 1
                and Register 0x58, Bit 7 = 1).
                0xB065: TIA ADC mode (if Register 0x42, Bit 7 = 0).
                Others: reserved.
            */
            write_register(SLOTA_AFE_CFG, 0xAE65);

            // Write 1 to bit 7 to enable the integrator as a buffer.
            write_register(MATH, 0b10000000);

            /*
                To apply a reverse bias to the photodiode:
                Bit 7 = 1
                Bits[11:10] - SLOTB_V_CATHODE
                Bits[9:8] - SLOTA_V_CATHODE
                Set these to 0x2 for 250mV Vbias
            */
            rbias_select = 0x20 | 0b10000000 | ((0x2 << 8) & 0b1100000000) | ((0x2 << 10) & 0b110000000000);
            
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
        
            setLED(GREEN, 0xE, 0, 1);
            setMode(OPERATION);
        }

        /*
        @brief Reads the 16-bit FIFO via multi-word read (as detailed by the datasheet)
        @param *out points to the data's destination 
        @param len the number of bytes to be read, must be at least 2
        */
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

        /*
        @brief Reads the 16-bit value of each PD Channel 
        */
        bool readPPG(uint16_t *pd1, uint16_t *pd2, uint16_t *pd3, uint16_t *pd4) {
            uint8_t reg[2];
            
            setMode(OPERATION);
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