#ifndef MIST_USB_JOYSTICK_HANDLER_H
#define MIST_USB_JOYSTICK_HANDLER_H

#include "MistHIDUSBController.h"
#include <stdbool.h>

namespace calypso {
    class MistUSBJoystickHandler: public USBReportHandler {
    public:
        MistUSBJoystickHandler();
        bool addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid, hid_report_t *report);
        void removeDevice(uint8_t const deviceId);
        void handleReport(uint8_t const deviceId, uint8_t const* report, uint16_t length);
        uint8_t numDevices() const;
        void updateJoysticks();
        bool addButtonMapping(const char* s);
        void clearButtonMappings();
    private:
        int8_t getJoystickIndex(uint8_t deviceId) const;
        void updateJoystickStatus(uint8_t deviceId, uint8_t const* report, uint16_t length);
        uint16_t collectBits(uint8_t const* data, uint16_t offset, uint8_t size, bool isSigned);
        static constexpr uint8_t MAX_JOYSTICKS = 4;
        static constexpr uint8_t MAX_BUTTON_MAPPINGS = 8;
        uint8_t m_numDevices;
        struct {
            bool used;
            uint8_t deviceId;
            uint16_t vid;
            uint16_t pid;
            uint8_t joyNumber;
            uint32_t mistJoyStatus;
            hid_report_t *report;
        } m_joysticks[MAX_JOYSTICKS];
        struct {
            uint16_t vid; 
            uint16_t pid;
            uint8_t offset;
            uint8_t button;
        } m_button_mappings[MAX_BUTTON_MAPPINGS];
    };
}

#endif //MIST_USB_JOYSTICK_HANDLER_H