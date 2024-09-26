#ifndef MIST_SPI_H
#define MIST_SPI_H

#include <inttypes.h>
#include "attrs.h"
#include "hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

    void spi_slow();
    void spi_fast();
    void spi_fast_mmc();
    void spi_wait4xfer_end();
    unsigned char spi_get_speed();
    void spi_set_speed(unsigned char speed);

    // Selection signals
    void EnableFpga(void);
    void DisableFpga(void);
    void EnableOsd(void);
    void DisableOsd(void);
    void EnableDMode();
    void DisableDMode();

    unsigned char spi_in();
    void spi8(unsigned char parm);
    void spi16(unsigned short parm);
    void spi16le(unsigned short parm);
    void spi24(unsigned long parm);
    void spi32(unsigned long parm);
    void spi32le(unsigned long parm);
    void spi_n(unsigned char value, unsigned short cnt);

    // Block transfers
    void spi_block_read(char *addr);
    void spi_read(char *addr, uint16_t len);
    void spi_block_write(const char *addr);
    void spi_write(const char *addr, uint16_t len);
    void spi_block(unsigned short num);

    // OSD
    void spi_osd_cmd_cont(unsigned char cmd);
    void spi_osd_cmd(unsigned char cmd);
    void spi_osd_cmd8_cont(unsigned char cmd, unsigned char parm);
    void spi_osd_cmd8(unsigned char cmd, unsigned char parm);
    void spi_osd_cmd32_cont(unsigned char cmd, unsigned long parm);
    void spi_osd_cmd32(unsigned char cmd, unsigned long parm);
    void spi_osd_cmd32le_cont(unsigned char cmd, unsigned long parm);
    void spi_osd_cmd32le(unsigned char cmd, unsigned long parm);

    // User IO
    void spi_uio_cmd_cont(unsigned char cmd);
    void spi_uio_cmd(unsigned char cmd);
    void spi_uio_cmd8(unsigned char cmd, unsigned char parm);
    void spi_uio_cmd8_cont(unsigned char cmd, unsigned char parm);
    void spi_uio_cmd32(unsigned char cmd, unsigned long parm);
    void spi_uio_cmd64(unsigned char cmd, unsigned long long parm);

    unsigned char SPI(unsigned char outByte);

    #define SPI_SDC_CLK_VALUE 2     // 24 MHz
    #define SPI_MMC_CLK_VALUE 3     // 16 MHz
    #define SPI_SLOW_CLK_VALUE 120  // 400kHz

#ifdef __cplusplus
}
#endif

#endif