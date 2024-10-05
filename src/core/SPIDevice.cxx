#include "SPIDevice.h"
#include "hardware/gpio.h"
#include <cstdio>
#include "calypso-debug.h"

using namespace calypso;

SPIDevice::SPIDevice(spi_inst_t* spiPeripheral, uint8_t sck, uint8_t miso, uint8_t mosi):
    m_spiPeripheral(spiPeripheral),
    m_sck(sck),
    m_miso(miso),
    m_mosi(mosi),
    m_baudrate(0) {}

bool SPIDevice::init() {

    gpio_init(m_sck);
    gpio_set_function(m_sck, GPIO_FUNC_SPI);
    gpio_init(m_miso);
    gpio_set_function(m_miso, GPIO_FUNC_SPI);
    gpio_init(m_mosi);
    gpio_set_function(m_mosi, GPIO_FUNC_SPI);
    gpio_set_drive_strength(m_sck, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(m_miso, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(m_mosi, GPIO_DRIVE_STRENGTH_12MA);

    spi_init(m_spiPeripheral, DEFAULT_BAUDRATE);
    spi_set_format(m_spiPeripheral, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    m_baudrate = DEFAULT_BAUDRATE;
    return true;
}

void SPIDevice::setBaudrate(uint32_t speed) {
    SPI_DEBUG_LOG(L_INFO, "SPIDevice::setBaudrate(%ld)\n", speed);
    spi_set_baudrate(m_spiPeripheral, speed);
    m_baudrate = speed;
}

uint32_t SPIDevice::baudrate() const {
    return m_baudrate;
}

uint8_t SPIDevice::byte(uint8_t value) {
    uint8_t readValue;
    spi_write_read_blocking(m_spiPeripheral, &value, &readValue, 1);
    return readValue;
}

size_t SPIDevice::send(const uint8_t *buffer, size_t len) {
    return spi_write_blocking(m_spiPeripheral, buffer, len);
}

size_t SPIDevice::recv(uint8_t *buffer, size_t len) {
    return spi_read_blocking(m_spiPeripheral, 0xff, buffer, len);
}

size_t SPIDevice::sendrecv(const uint8_t *sendBuffer, uint8_t *recvBuffer, size_t len) {
    return spi_write_read_blocking(m_spiPeripheral, sendBuffer, recvBuffer, len);
}

bool SPIDevice::isBusy() {
    return spi_is_busy(m_spiPeripheral);
}