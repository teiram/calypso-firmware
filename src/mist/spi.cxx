#include "spi.h"
#include "SPIDevice.h"
#include "Configuration.h"
#include "hardware/gpio.h"
#include <stdio.h>

extern calypso::SPIDevice spi;

#define SPI_SLOW_BAUD   500000
#define SPI_SDC_BAUD   24000000
#define SPI_MMC_BAUD   16000000
static unsigned char spi_speed = SPI_SLOW_CLK_VALUE;

void spi_wait4xfer_end() {
    while (spi.isBusy()) tight_loop_contents;
}


void EnableFpga() {
    gpio_put(Configuration::GPIO_MIST_DATAIO, 0);
}

void DisableFpga() {
    spi_wait4xfer_end();
    gpio_put(Configuration::GPIO_MIST_DATAIO, 1);
}

void EnableOsd() {
    gpio_put(Configuration::GPIO_MIST_OSD, 0);
}

void DisableOsd() {
    spi_wait4xfer_end();
    gpio_put(Configuration::GPIO_MIST_OSD, 1);
}

void EnableIO() {
    gpio_put(Configuration::GPIO_MIST_USERIO, 0);
}

void DisableIO() {
    spi_wait4xfer_end();
    gpio_put(Configuration::GPIO_MIST_USERIO, 1);
}

void EnableDMode() {
    gpio_put(Configuration::GPIO_MIST_DMODE, 0);
}

void DisableDMode() {
    spi_wait4xfer_end();
    gpio_put(Configuration::GPIO_MIST_DMODE, 1);
}

void spi_block(unsigned short num) {
    while (num--) {
        spi.byte(0xff);
    }
}

void spi_read(char *addr, uint16_t len) {
    spi.recv((uint8_t *) addr, len);
}

void spi_block_read(char *addr) {
    spi.recv((uint8_t *) addr, 512);
}

void spi_write(const char *addr, uint16_t len) {
    spi.send((uint8_t *) addr, len);
}

unsigned char SPI(unsigned char outByte) {
    return spi.byte(outByte);
}

void spi_block_write(const char *addr) {
    spi.send((uint8_t *) addr, 512);
}

void spi_slow() {
    spi.setBaudrate(SPI_SLOW_BAUD);
    spi_speed = SPI_SLOW_CLK_VALUE;
}

void spi_fast() {
    spi.setBaudrate(SPI_SDC_BAUD);
    spi_speed = SPI_SDC_CLK_VALUE;
}

void spi_fast_mmc() {
    spi.setBaudrate(SPI_MMC_BAUD);
    spi_speed = SPI_MMC_CLK_VALUE;
}

unsigned char spi_get_speed() {
  return spi_speed;
}

void spi_set_speed(unsigned char speed) {
  switch (speed) {
    case SPI_SLOW_CLK_VALUE:
      spi_slow();
      break;

    case SPI_SDC_CLK_VALUE:
      spi_fast();
      break;

    default:
      spi_fast_mmc();
  }
}

unsigned char spi_in() {
  return SPI(0);
}

void spi8(unsigned char parm) {
  SPI(parm);
}

void spi16(unsigned short parm) {
  SPI(parm >> 8);
  SPI(parm >> 0);
}

void spi16le(unsigned short parm) {
  SPI(parm >> 0);
  SPI(parm >> 8);
}

void spi24(unsigned long parm) {
  SPI(parm >> 16);
  SPI(parm >> 8);
  SPI(parm >> 0);
}

void spi32(unsigned long parm) {
  SPI(parm >> 24);
  SPI(parm >> 16);
  SPI(parm >> 8);
  SPI(parm >> 0);
}

// little endian: lsb first
void spi32le(unsigned long parm) {
  SPI(parm >> 0);
  SPI(parm >> 8);
  SPI(parm >> 16);
  SPI(parm >> 24);
}

void spi_n(unsigned char value, unsigned short cnt) {
  while(cnt--) 
    SPI(value);
}

/* OSD related SPI functions */
void spi_osd_cmd_cont(unsigned char cmd) {
  EnableOsd();
  SPI(cmd);
}

void spi_osd_cmd(unsigned char cmd) {
  spi_osd_cmd_cont(cmd);
  DisableOsd();
}

void spi_osd_cmd8_cont(unsigned char cmd, unsigned char parm) {
  EnableOsd();
  SPI(cmd);
  SPI(parm);
}

void spi_osd_cmd8(unsigned char cmd, unsigned char parm) {
  spi_osd_cmd8_cont(cmd, parm);
  DisableOsd();
}

void spi_osd_cmd32_cont(unsigned char cmd, unsigned long parm) {
  EnableOsd();
  SPI(cmd);
  spi32(parm);
}

void spi_osd_cmd32(unsigned char cmd, unsigned long parm) {
  spi_osd_cmd32_cont(cmd, parm);
  DisableOsd();
}

void spi_osd_cmd32le_cont(unsigned char cmd, unsigned long parm) {
  EnableOsd();
  SPI(cmd);
  spi32le(parm);
}

void spi_osd_cmd32le(unsigned char cmd, unsigned long parm) {
  spi_osd_cmd32le_cont(cmd, parm);
  DisableOsd();
}

/* User_io related SPI functions */
void spi_uio_cmd_cont(unsigned char cmd) {
  EnableIO();
  SPI(cmd);
}

void spi_uio_cmd(unsigned char cmd) {
  spi_uio_cmd_cont(cmd);
  DisableIO();
}

void spi_uio_cmd8_cont(unsigned char cmd, unsigned char parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
}

void spi_uio_cmd8(unsigned char cmd, unsigned char parm) {
  spi_uio_cmd8_cont(cmd, parm);
  DisableIO();
}

void spi_uio_cmd32(unsigned char cmd, unsigned long parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
  SPI(parm>>8);
  SPI(parm>>16);
  SPI(parm>>24);
  DisableIO();
}

void spi_uio_cmd64(unsigned char cmd, unsigned long long parm) {
  EnableIO();
  SPI(cmd);
  SPI(parm);
  SPI(parm>>8);
  SPI(parm>>16);
  SPI(parm>>24);
  SPI(parm>>32);
  SPI(parm>>40);
  SPI(parm>>48);
  SPI(parm>>56);
  DisableIO();
}
