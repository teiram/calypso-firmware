#ifndef HID_USB_CONTROLLER_H
#define HID_USB_CONTROLLER_H

#include <inttypes.h>
#include "tusb_config.h"

namespace calypso {
    class USBReportHandler {
    public:
        virtual void handleReport(uint8_t const* report, uint16_t length) = 0;
    };

    class NullUSBReportHandler: public USBReportHandler {
        void handleReport(uint8_t const* report, uint16_t length);
    };
    class HIDUSBController {
    private:
        static constexpr uint8_t MAX_KBD_HANDLERS = 2;
        static constexpr uint8_t MAX_MOUSE_HANDLERS = 2;
        USBReportHandler* m_kbdReportHandlers[MAX_KBD_HANDLERS];
        USBReportHandler* m_mouseReportHandlers[MAX_MOUSE_HANDLERS];
        void forwardReportToHandlers(USBReportHandler* handlers[], uint8_t arraySize, uint8_t const* report, uint16_t len);
    public:
        bool addKbdReportHandler(USBReportHandler* handler);
        bool addMouseReportHandler(USBReportHandler* handler);
        void handleReport(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    };
}

#endif //HID_USB_CONTROLLER_H