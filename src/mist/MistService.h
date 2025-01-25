#ifndef MIST_SERVICE_H
#define MIST_SERVICE_H

#include "Service.h"
#include "SPIDevice.h"
#include "JTAG.h"
#include "Joystick.h"
#include <cinttypes>

namespace calypso {
    class MistService: public Service {
    private:
        constexpr static const char NAME[] = {"MistService"};
        uint8_t m_userIOGPIO;
        uint8_t m_dataIOGPIO;
        uint8_t m_osdGPIO;
        uint8_t m_directModeGPIO;

        inline void initializeSelectGPIO(uint8_t gpio);
    public:
        MistService(uint8_t userIOGPIO, uint8_t dataIOGPIO, uint8_t osdGPIO, uint8_t directModeGPIO);
        const char* name();
        bool init();
        bool needsAttention();
        void attention();
        void cleanup();
    };
}

#endif //MIST_SERVICE_H