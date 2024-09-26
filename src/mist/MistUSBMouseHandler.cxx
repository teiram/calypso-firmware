#include "MistUSBMouseHandler.h"
#include "calypso-debug.h"

#include <cstdio>

extern "C" {
    #include "mist-firmware/user_io.h"
}
using namespace calypso;

void MistUSBMouseHandler::handleReport(uint8_t const* report, uint16_t len) {
    USB_DEBUG_DUMP(L_DEBUG, "Mouse report", report, len);
    // TODO: Check if this is right
    if (len >= 8) {
        uint8_t buttons = report[1];
        int8_t x = report[2];
        int8_t y = report[4];
        int8_t z = report[6];
        user_io_mouse(0, buttons, x, y, z);
    }
}