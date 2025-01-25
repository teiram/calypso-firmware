#ifndef MIST_USB_KEYBOARD_HANDLER_H
#define MIST_USB_KEYBOARD_HANDLER_H

#include "MistHIDUSBController.h"

namespace calypso {
    class MistUSBKeyboardHandler: public USBReportHandler {
    private:
        uint8_t m_numDevices;
    public:
        MistUSBKeyboardHandler();
        uint8_t numDevices() const;
        bool addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid, hid_report_t *report);
        void removeDevice(uint8_t const deviceId);
        void handleReport(uint8_t const index, uint8_t const* report, uint16_t len);
    };
}

#endif //MIST_USB_KEYBOARD_HANDLER_H