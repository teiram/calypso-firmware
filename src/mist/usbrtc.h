// USB RTC support for the Calypso firmware
// Author: Shaeon (Carlos Palmero)
//
// Supports an i2c-tiny-usb adapter (VID/PID 0403:C631) with a DS3231/DS1307
// RTC at I2C address 0x68, compatible with the USB RTC modules supported by
// the MiST firmware (usb/rtc/i2c-tiny.c).
//
// Date format, same as MiST (ctime_t):
//   d[0] year-1900 (year 2000+yy -> yy+100), d[1] month, d[2] day,
//   d[3] hour, d[4] min, d[5] sec, d[6] weekday (1-7)

#ifndef USBRTC_H
#define USBRTC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Call periodically from the main loop (after tuh_task)
void usbrtc_task(void);

// true if a USB RTC is present and the cached time is valid
bool usbrtc_get_time(uint8_t *d);

// Set the RTC; true if an RTC is present (the write is asynchronous)
bool usbrtc_set_time(const uint8_t *d);

#ifdef __cplusplus
}
#endif

#endif // USBRTC_H
