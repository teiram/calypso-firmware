#ifndef MIST_USB_MOUSE_HANDLER_H
#define MIST_USB_MOUSE_HANDLER_H

#include "HIDUSBController.h"

namespace calypso {
    class MistUSBMouseHandler: public USBReportHandler {
    public:
        void handleReport(uint8_t const* report, uint16_t len);
    };
}

#endif //MIST_USB_MOUSE_HANDLER_H