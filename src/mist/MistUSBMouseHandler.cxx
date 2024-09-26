#include "MistUSBMouseHandler.h"
#include "calypso-debug.h"

#include <cstdio>

extern "C" {
    #include "mist-firmware/user_io.h"
}
using namespace calypso;

void MistUSBMouseHandler::handleReport(uint8_t const* report, uint16_t len) {
    USB_DEBUG_DUMP(L_DEBUG, "Mouse report", report, len);
    uint8_t buttons;
    int8_t x, y, z;
    //user_io_mouse(0, buttons, x, y, z);
}