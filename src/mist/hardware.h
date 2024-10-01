#ifndef HARDWARE_H
#define HARDWARE_H

#include <inttypes.h>
#include "attrs.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define lowest(a,b) ((a) < (b) ? (a) : (b))

#define LED_GPIO       25
#define DISKLED_ON    do { gpio_put(LED_GPIO, true);} while (0);
#define DISKLED_OFF   do { gpio_put(LED_GPIO, false);} while (0);

#define SPI_MINIMIGV1_HACK {}
#define STORE_VARS_SIZE   0x20
#define STORE_VARS_POS    (0x20000000 + (256*1024) - STORE_VARS_SIZE)

#define USB_LOAD_VAR         *(int*)(STORE_VARS_POS+4)
#define USB_LOAD_VALUE       12345678

#define DEBUG_MODE_VAR       *(int*)(STORE_VARS_POS+8)
#define DEBUG_MODE_VALUE     87654321
#define DEBUG_MODE           (DEBUG_MODE_VAR == DEBUG_MODE_VALUE)

#define VIDEO_KEEP_VALUE     0x87654321
#define VIDEO_KEEP_VAR       (*(int*)(STORE_VARS_POS+0x10))
#define VIDEO_ALTERED_VAR    (*(uint8_t*)(STORE_VARS_POS+0x14))
#define VIDEO_SD_DISABLE_VAR (*(uint8_t*)(STORE_VARS_POS+0x15))
#define VIDEO_YPBPR_VAR      (*(uint8_t*)(STORE_VARS_POS+0x16))

#define USB_BOOT_VALUE       0x8007F007
// #define USB_BOOT_VAR         (*(int*)0x0020FF18)
#define USB_BOOT_VAR         (*(int*)(STORE_VARS_POS+0x18))


#define SECTOR_BUFFER_SIZE   4096

#ifdef BOOT_FLASH_ON_ERROR
void BootFromFlash();
#endif

char mmc_inserted(void);
char mmc_write_protected(void);

void USART_Init(unsigned long baudrate);
void USART_Write(unsigned char c);
unsigned char USART_Read(void);
void USART_Poll(void);

unsigned long CheckButton(void);

void Timer_Init(void);
unsigned long GetTimer(unsigned long offset);
unsigned long CheckTimer(unsigned long t);
void WaitTimer(unsigned long time);

int GetRTTC();
char GetRTC(unsigned char *d);
char SetRTC(unsigned char *d);

void MCUReset();

int GetSPICLK();

void InitADC(void);
void PollADC();

unsigned char Buttons();
unsigned char MenuButton();
unsigned char UserButton();

void InitDB9();
char GetDB9(char index, unsigned char *joy_map);
void DB9SetLegacy(uint8_t on);

void EnableIO(void);
void DisableIO(void);

#define DEBUG_FUNC_IN()
#define DEBUG_FUNC_OUT()

int8_t pl2303_is_blocked(void);
uint8_t *get_mac();
int iprintf(const char *fmt, ...);
int8_t pl2303_present(void);
void pl2303_settings(uint32_t rate, uint8_t bits, uint8_t parity, uint8_t stop);
void pl2303_tx(uint8_t *data, uint8_t len);
void pl2303_tx_byte(uint8_t byte);
uint8_t pl2303_rx_available(void);
uint8_t pl2303_rx(void);
int8_t pl2303_is_blocked(void);
uint8_t get_pl2303s(void);
void SPIN();

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_H

