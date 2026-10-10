#include "USBService.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usbrtc.h"

using namespace calypso;

const char *USBService::name() {
    return NAME;
}

bool USBService::init() {
    board_init();
    tusb_init();
    return true;
}

bool USBService::needsAttention() {
    return true;
}

void USBService::attention() {
    tuh_task();
    usbrtc_task();
}

void USBService::cleanup() {}
