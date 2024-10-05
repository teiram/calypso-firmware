#ifndef SPISDCARD_H
#define SPISDCARD_H

#include "SPIDevice.h"
#include <inttypes.h>

namespace calypso {
    class SPISDCard {
    private:
        static constexpr uint8_t DEFAULT_RETRIES = 10;
        static constexpr uint32_t DEFAULT_TIMEOUT = 2000;
        SPIDevice& m_spi;
        uint8_t m_cs;
        uint8_t m_directModeGPIO;
        bool m_isSDHC;
        uint8_t spin();
        uint8_t reset();
        bool isReady(uint32_t timeout = DEFAULT_TIMEOUT);
        void setSPIMode();
        void toggleSelect(bool value);
        bool initTry();
        bool initNoSel();
        bool readResponse(uint8_t *buffer, size_t len);
        uint8_t cmd(const uint8_t request[], int requestlen, uint8_t recv[], int recvlen,
            uint8_t expected, bool release, uint8_t retries = DEFAULT_RETRIES);
        bool cxd(uint8_t request[], uint8_t recv[]);
        uint8_t cmd1();
        uint8_t cmd8();
        uint8_t cmd41();
        uint8_t cmd55();
        uint8_t cmd58();
        uint8_t cmd59();
        bool readSectorTry(uint32_t lba, uint8_t *data);
    public:
        SPISDCard(SPIDevice& spi, uint8_t cs, uint8_t directModeGpio);
        bool isSDHC();
        bool init();
        bool writeSector(uint32_t lba, const uint8_t *data);
        bool readSector(uint32_t lba, uint8_t *data);
        uint8_t cmd9(uint8_t buffer[]);
        uint8_t cmd10(uint8_t buffer[]);
    };
}
#endif //SPISDCARD_H