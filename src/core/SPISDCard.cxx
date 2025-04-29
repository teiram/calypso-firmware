#include "SPISDCard.h"
#include "CRC.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <cstdio>
#include "calypso-debug.h"

using namespace calypso;

SPISDCard::SPISDCard(SPIDevice& spi, uint8_t cs, uint8_t directModeGPIO):
    m_spi(spi),
    m_cs(cs),
    m_directModeGPIO(directModeGPIO),
    m_isSDHC(false) {}

bool SPISDCard::isSDHC() {
    return m_isSDHC;
}

bool SPISDCard::initNoSel() {
    int8_t timeout = 10;
    m_spi.setBaudrate(400 * 1000);
    while (!initTry() && --timeout) tight_loop_contents();
  
    if (timeout > 0) {
        m_spi.setBaudrate(25 * 1000000);
    }
    return timeout > 0;
}

bool SPISDCard::init() {
    gpio_init(m_cs);
    gpio_set_dir(m_cs, GPIO_OUT);

    bool success;
	gpio_put(m_cs, 0);
    success = initNoSel();
	gpio_put(m_cs, 1);

    return success;
}

uint8_t SPISDCard::spin() {
  uint8_t out = 0xff;
  return m_spi.byte(0xff);
}

void SPISDCard::toggleSelect(bool value) {
    uint8_t val = spin();
    gpio_put(m_cs, value);
    val = spin();
}

bool SPISDCard::readResponse(uint8_t *buffer, size_t len) {
    *buffer = 0xff;
    uint16_t timeout = 512;
    while ((*buffer) == 0xff && --timeout) {
        *buffer = m_spi.byte(0xff);
    }
    if ((*buffer) == 0xff) {
        SD_DEBUG_LOG(L_ERROR, "SPI timeout in readResponse\n");
        return false;
    }
    if (len > 1) {
        m_spi.recv(buffer + 1, len - 1);
    }
    return true;
}

uint8_t SPISDCard::cmd(const uint8_t request[], int requestlen, uint8_t recv[], int recvlen,
    uint8_t expected, bool release, uint8_t retries) {
  
    while (retries--) {
        toggleSelect(0);    
        if (!isReady()) {
            gpio_put(m_cs, 1);
            continue;
        }
        m_spi.send(request, requestlen);
        readResponse(recv, recvlen);
        if (recv[0] == 0xff || recv[0] == expected) break;
    }
    if (release) {
        toggleSelect(1);
    }
    return recv[0];
}

bool SPISDCard::cxd(uint8_t request[], uint8_t recv[]) {
    uint8_t buffer[1];
	toggleSelect(0);
    if (cmd(request, 6, buffer, 1, 0x00, false) != 0x00) {
        toggleSelect(1);
        return false;
    }

    uint8_t timeout = 20;
    while (--timeout) {
        uint8_t status = m_spi.byte(0xff);
        if (status == 0xfe) {
            break;
        }
        sleep_us(100);
    }

    if (!timeout) {
        toggleSelect(1);
        SD_DEBUG_LOG(L_ERROR, "Timeout in cxd\n");
        return false;
    }

    m_spi.recv(recv, 16);

    timeout = 20;
    while (--timeout) {
        uint8_t status = m_spi.byte(0xff);
        if (status == 0xff) {
            break;
        }
        sleep_us(100);
    }
    if (!timeout) {
        SD_DEBUG_LOG(L_WARN, "Timeout after cxd\n");
    }
    toggleSelect(1);
    return true;
}

uint8_t SPISDCard::cmd1() {
    uint8_t request[] = {0x50, 0x00, 0x00, 0x02, 0x00, 0xff};
    uint8_t recv[1];
    return cmd(request, sizeof request, recv, 1, 0x00, true);
}

uint8_t SPISDCard::cmd8() {
  uint8_t request[] = {0xff, 0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
  uint8_t recv[5];
  return cmd(request, sizeof request, recv, sizeof recv, 0x01, true);
}

uint8_t SPISDCard::cmd55() {
  uint8_t request[] = {0xff, 0x77, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t recv[1];
  return cmd(request, sizeof request, recv, sizeof recv, 0x00, true);
}

uint8_t SPISDCard::cmd41() {
  uint8_t request[] = {0xff, 0x69, 0x40, 0x00, 0x00, 0x00, 0x87};
  uint8_t recv[1];
  return cmd(request, sizeof request, recv, sizeof recv, 0x00, true, 100);
}

uint8_t SPISDCard::cmd58() {
  uint8_t request[] = {0xff, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x75};
  uint8_t recv[5];
  if (cmd(request, sizeof request, recv, sizeof recv, 0x00, true) == 0x00) {
    return recv[1];
  }
  return 0xff;
}

uint8_t SPISDCard::cmd59() {
  uint8_t request[] = {0x7b, 0x00, 0x00, 0x00, 0x00, 0xff};
  uint8_t recv[8];
  return cmd(request, sizeof request, recv, sizeof recv, 0x01, true);
}


uint8_t SPISDCard::cmd9(uint8_t buffer[]) {
  uint8_t request[] = {0x49, 0x00, 0x00, 0x00, 0x00, 0xaf};
  return cxd(request, buffer);
}

uint8_t SPISDCard::cmd10(uint8_t buffer[]) {
  uint8_t request[] = {0x4a, 0x00, 0x00, 0x00, 0x00, 0x1b};
  return cxd(request, buffer);
}


uint8_t SPISDCard::reset() {
  uint8_t request[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
  uint8_t recv[1];
  return cmd(request, sizeof request, recv, sizeof recv, 0x01, true);
}

bool SPISDCard::isReady(uint32_t timeout) {
    uint8_t value;
    do {
        value = m_spi.byte(0xff);
        timeout --;
    } while (value != 0xff && timeout);
  return value == 0xff;
}

void SPISDCard::setSPIMode() {
    uint8_t sd_presignal_data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    gpio_put(m_cs, 1);
    m_spi.send(sd_presignal_data, sizeof sd_presignal_data);
}

bool SPISDCard::initTry() {
    SD_DEBUG_LOG(L_DEBUG, "SPISDCard::initTry\n");
    setSPIMode();
    if (!isReady()) {
        SD_DEBUG_LOG(L_ERROR, "Unable to set card into SPI mode\n");
        return false;
    }
    toggleSelect(0);
    if (reset() == 0x01) {
        if (cmd8() == 0x01) {
            int retries = 5;
            while (retries --) {
                if (cmd55() == 0x01) {
                    if (cmd41() == 0x00) {
                        SD_DEBUG_LOG(L_DEBUG, "cmd41 got zero response\n");
                        break;
                    }
                }
            }
        
            if (retries > 0) {
                if (cmd58() & 0x40) {
                    SD_DEBUG_LOG(L_INFO, "SDHC card detected\n");
                    m_isSDHC = true;
                    toggleSelect(1);
                    return true;
                } else if (cmd1() == 0x00) {
                    SD_DEBUG_LOG(L_INFO, "SD card detected\n");
                    m_isSDHC = false;
                    toggleSelect(1);
                    return true;
                }
            } else {
                SD_DEBUG_LOG(L_ERROR, "Unable to identify SD card\n");
                return false;
            }
        }
    } else {
        SD_DEBUG_LOG(L_ERROR, "SD Card Reset failed\n");
    }
    return false;
}

bool SPISDCard::writeSector(uint32_t lba, const uint8_t *data) {
    uint8_t request[] = {0xff, 0x58, 0x00, 0x00, 0x00, 0x00, 0xff};
    uint8_t writegap[] = {0xff, 0xfe};
    uint8_t crc[] = {0xff, 0xff, 0xff};
    uint8_t buffer[1];

    if (!m_isSDHC) lba <<= 9;

    request[2] = (lba >> 24);
    request[3] = (lba >> 16) & 0xff;
    request[4] = (lba >> 8) & 0xff;
    request[5] = lba & 0xff;
  
    if (cmd(request, sizeof request, buffer, sizeof buffer, 0x00, false) != 0x00) {
        toggleSelect(1);
        SD_DEBUG_LOG(L_ERROR, "Write attempt failed with: %02X\n", buffer[0]);
        return false;
    }
  
    m_spi.send(writegap, sizeof writegap);
    m_spi.send(data, 512);
    m_spi.send(crc, sizeof crc);

    int timeout = 100000;
    while (timeout--) {
        uint8_t status = m_spi.byte(0xff);
        if (status != 0x00) {
            break;
        }
    }
  
    toggleSelect(1);
    return timeout > 0;
}

bool SPISDCard::readSectorTry(uint32_t lba, uint8_t *data) {
    uint8_t request[] = {0x51, 0x00, 0x05, 0x00, 0x00, 0xff};
    uint8_t buffer[1];

    if (!m_isSDHC) lba <<= 9;
  
    request[1] = (lba >> 24);
    request[2] = (lba >> 16) & 0xff;
    request[3] = (lba >> 8) & 0xff;
    request[4] = lba & 0xff;

    toggleSelect(0);
    uint8_t result = cmd(request, sizeof request, buffer, sizeof buffer, 0x00, false);
    if (result != 0x00 && result != 0x04) {
        toggleSelect(1);
        SD_DEBUG_LOG(L_ERROR, "SD read failed with result: %02X\n", result);
        return false;
    }
  
    int timeout = 50000;
    while (m_spi.byte(0xff) != 0xfe && timeout--) tight_loop_contents;  
    if (timeout <= 1) {
        toggleSelect(1);
        SD_DEBUG_LOG(L_ERROR, "Read failed (2)\n");
        return 1;
    }

    if (data == 0) {
        gpio_put(m_directModeGPIO, 0);
        // Maybe we can do this faster somehow else (using some global buffer)
        for (int i = 0; i < 512; i++) {
            m_spi.byte(0xff);
        }
    } else {
        m_spi.recv(data, 512);
    }

    if (data == 0) {
        gpio_put(m_directModeGPIO, 1);
    }

    uint8_t crc[2];
    m_spi.recv(crc, 2);

    toggleSelect(1);

    uint16_t crcw = (crc[0] << 8) | crc[1];
#ifndef SD_NO_CRC
    uint16_t calculatedCrc = 0;
    if (data != NULL && (calculatedCrc = CRC::crc16iv(data, 512, 0)) != crcw) {
    SD_DEBUG_LOG(L_ERROR, "CRC mismatch: %04X != %04X\n", crcw, calculatedCrc);
    return false;
  }
#endif
  
  return true;
}

bool SPISDCard::readSector(uint32_t lba, uint8_t *data) {
    uint8_t resets = 3;
  
	gpio_put(m_cs, 0);
    do {
        uint8_t retries = 10;
        while (!readSectorTry(lba, data) && retries --) tight_loop_contents();
        if (retries >= 0) {
	        gpio_put(m_cs, 1);
            return true;
        }
        initNoSel();
    } while (resets--);

    gpio_put(m_cs, 1);
    return false;
}
