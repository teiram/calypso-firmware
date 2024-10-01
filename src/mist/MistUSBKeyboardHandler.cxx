#include "MistUSBKeyboardHandler.h"
#include <stdio.h>
#include <string.h>
#include "calypso-debug.h"
extern "C" {
    #include "mist-firmware/user_io.h"
}
using namespace calypso;

void MistUSBKeyboardHandler::handleReport(uint8_t const* report, uint16_t len) {
    if (len >= 8) {
        USB_DEBUG_DUMP(L_TRACE, "USB Keyboard report", report, len);
        //Assume there is only report when there are changes
        unsigned char clonedReport[6];
        memcpy(clonedReport, report + 2, 6);
        user_io_kbd(report[0], clonedReport, UIO_PRIORITY_KEYBOARD, 0, 0);
    } else {
        USB_DEBUG_LOG(L_WARN, "Unexpected keyboard report size %d\n", len);
    }
}