#ifndef JTAG_H
#define JTAG_H

#include <inttypes.h>

namespace calypso {
    class JTAG {
    private:
        typedef struct {    
            uint8_t onebit:1;     
            uint16_t manuf:11; 
            uint16_t size:9;  
            uint8_t family:7; 
            uint8_t rev:4; 
        } jtag_code_t;

        typedef union {
            uint32_t code;
            jtag_code_t b;
        } idcode_t;

        static constexpr uint8_t MAX_IR_CHAINLENGTH = 100; 
        uint8_t m_tdo;
        uint8_t m_tdi;
        uint8_t m_tck;
        uint8_t m_tms;

        void enable();
        void disable();
        void tck();
        void tdiPush(bool value);
        void tmsPush(bool value);
        void reset();
        int getChainLength();
        void enterSelectDr();
        void enterShiftIr();
        void enterShiftDr();
        void enterRti();
        void enterSelectIr();
        void exitShift();
        void readDr(int length);
        void read(idcode_t* value, int length);

    public:
        JTAG(uint8_t tdo, uint8_t tdi, uint8_t tck, uint8_t tms);

        void startProgram();
        void endProgram();
        void startProgramXilinx();
        void endProgramXilinx();
        void programPostamble();
        void programChunk(uint8_t *data, uint16_t length);
        int scan();
    };
}
#endif //JTAG_H