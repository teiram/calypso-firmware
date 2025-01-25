#ifndef PS3_USB_HID_DRIVER_H
#define PS3_USB_HID_DRIVER_H

#include "USBHIDDriver.h"

namespace calypso {
    class PS3USBHIDDriver: public USBHIDBaseDriver {
    private:
        uint8_t* m_report;
        uint16_t m_reportLength;
    public:
        PS3USBHIDDriver(uint8_t* report, uint16_t reportLength);
        bool initialize(uint8_t deviceAddress, uint8_t instance);
        bool hasCustomReport();
        std::pair<uint8_t*, uint16_t> customReport();
    };
}
#endif //PS3_USB_HID_DRIVER_H