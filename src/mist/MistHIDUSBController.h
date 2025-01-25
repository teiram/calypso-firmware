#ifndef MIST_HID_USB_CONTROLLER_H
#define MIST_HID_USB_CONTROLLER_H

#include <inttypes.h>
#include "USBHIDDriver.h"
#include "tusb_config.h"
extern "C" {
    #include "usb/hidparser.h"
}

namespace calypso {
    class USBReportHandler {
    public:
        virtual bool addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid, hid_report_t *report) {return true;};
        virtual void removeDevice(uint8_t const deviceId) {};
        virtual void handleReport(uint8_t const deviceId, uint8_t const* report, uint16_t length) = 0;
    };

    class MistHIDUSBController {
    public:
    typedef struct {
        uint16_t vid;
        uint16_t pid;
        USBHIDDriver &driver;
    } custom_driver_t;
    MistHIDUSBController(USBReportHandler* kbdReportHandler,
    USBReportHandler* mouseReportHandler,
    USBReportHandler* joystickReportHandler);
    bool addHIDEntry(uint8_t deviceAddress, uint8_t instance, uint8_t const* report,
        uint16_t reportLength);
    void removeHIDEntry(uint8_t instance);
    void handleReport(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    private:
        int8_t getFreeHIDEntry();
        int8_t getHIDIndex(uint8_t instance);
        USBHIDDriver *getDriver(uint16_t vid, uint16_t pid);
        static constexpr uint8_t MAX_HIDS = 8;
        static constexpr uint8_t MAX_CUSTOM_BEHAVIORS = 4;
        USBReportHandler* m_kbdReportHandler;
        USBReportHandler* m_mouseReportHandler;
        USBReportHandler* m_joystickReportHandler;
        struct {
            bool used;
            uint8_t instance;
            hid_report_t report;
        } m_hids[MAX_HIDS];

        custom_driver_t *m_drivers;
        uint8_t m_numDrivers;
    public:

    };
}

#endif //MIST_HID_USB_CONTROLLER_H