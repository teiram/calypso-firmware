#ifndef USB_HID_DRIVER_H
#define USB_HID_DRIVER_H

#include <cinttypes>
#include <utility>

namespace calypso  {
    class USBHIDDriver {
    public:
        virtual bool initialize(uint8_t deviceAddress, uint8_t instance) = 0;
        virtual std::pair<uint8_t*, uint16_t> customReport() = 0;
        virtual bool hasCustomReport() = 0;
    };

    class USBHIDBaseDriver: public USBHIDDriver {
    public:
        virtual bool initialize(uint8_t deviceAddress, uint8_t instance) {return true;}
        virtual std::pair<uint8_t*, uint16_t> customReport() { return std::make_pair(nullptr, 0); }
        virtual bool hasCustomReport() {return false;}
    };
}

#endif //USB_HID_DRIVER_H
