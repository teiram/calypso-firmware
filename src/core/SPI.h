#ifndef SPI_H
#define SPI_H
#include <inttypes.h>
#include "hardware/spi.h"

namespace calypso {
    class SPI {
    private:
        static constexpr uint32_t DEFAULT_BAUDRATE = 400 * 1000;

        spi_inst_t *m_spiPeripheral;
        uint8_t m_sck;
        uint8_t m_miso;
        uint8_t m_mosi;
        uint32_t m_baudrate;

    public:
        SPI(spi_inst_t* spiPeripheral, uint8_t sck, uint8_t miso, uint8_t mosi);
        bool init();
        void setBaudrate(uint32_t baudrate);
        uint32_t baudrate() const;
        uint8_t byte(uint8_t value);
        size_t send(const uint8_t *buffer, size_t len);
        size_t recv(uint8_t *buffer, size_t len);
        size_t sendrecv(const uint8_t *sendBuffer, uint8_t *recvBuffer, size_t len);
        bool isBusy();
    };
}

#endif //SPI_H