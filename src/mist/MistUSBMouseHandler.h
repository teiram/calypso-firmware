#ifndef MIST_USB_MOUSE_HANDLER_H
#define MIST_USB_MOUSE_HANDLER_H

#include "MistHIDUSBController.h"

namespace calypso {
    class MistUSBMouseHandler: public USBReportHandler {
    private:
        int8_t getMouseIndex(uint8_t deviceId);
        void updateMouseStatus(uint8_t index, uint8_t const* report, uint16_t length);
        void updateMouseStatusFallback(uint8_t index, uint8_t const* report, uint16_t length);

        static constexpr uint8_t MAX_MICE = 4; 
        uint8_t m_numDevices;
        struct {
            bool used;
            uint8_t deviceId;
            uint16_t vid;
            uint16_t pid;
            uint8_t mouseNumber;
            hid_report_t *report;
        } m_mice[MAX_MICE];
    public:
        MistUSBMouseHandler();
        uint8_t numDevices() const;
        bool addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid, hid_report_t *report);
        void removeDevice(uint8_t const deviceId);
        void handleReport(uint8_t const index, uint8_t const* report, uint16_t len);
    };
}

#endif //MIST_USB_MOUSE_HANDLER_H