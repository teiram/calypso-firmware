#ifndef USBDEV_H
#define USBDEV_H

#include <inttypes.h>
#include "usb.h"

#define BULK_IN_SIZE  64
#define BULK_OUT_SIZE 64

void usb_dev_open(void);
void usb_dev_reconnect(void);

uint8_t  usb_cdc_is_configured(void);
uint16_t usb_cdc_write(const char *pData, uint16_t length);
uint16_t usb_cdc_read(char *pData, uint16_t length);

uint8_t  usb_storage_is_configured(void);
uint16_t usb_storage_write(const char *pData, uint16_t length);
uint16_t usb_storage_read(char *pData, uint16_t length);

void usb_attached(uint8_t dev, uint8_t idx, uint16_t vid, uint16_t pid, uint8_t *desc, uint16_t desclen);
void usb_detached(uint8_t dev);
void usb_handle_data(uint8_t dev, uint8_t *desc, uint16_t desclen);


#endif // USBDEV_H