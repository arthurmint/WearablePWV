#include "Arduino.h"
#include "Wire.h"


const uint8_t SDA_PIN = 6; // D4
const uint8_t SCL_PIN = 7; // D5

const uint8_t GPIO0 = 2; // D0
const uint8_t GPIO1 = 3; // D1

class ADPD1080 {
    private:
        const uint8_t addr = 0x64;
        const uint32_t clock_speed = 100E+3;

        const uint8_t SLOTA_LED_OFFSET = 0x30;
        const uint8_t SLOTB_LED_OFFSET = 0x35;
        const uint8_t SLOTA_PERIOD = 0x31;
        const uint8_t SLOTB_PERIOD = 0x36;
        const uint8_t
        const uint8_t

    public:
        ADPD1080(int SDA, int SCL, int GPIO0, int GPIO1) {
            Wire.setPins(SDA, SCL);

            Wire.begin();

            Wire.setClock(clock_speed);

            #ifdef DEBUG_OUTPUT
            printf("ADPD1080 Initialised\n");
            #endif
        };





}