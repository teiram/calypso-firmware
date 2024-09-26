#include "USBService.h"
#include "bsp/board.h"
#include "tusb.h"

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
}

void USBService::cleanup() {}
