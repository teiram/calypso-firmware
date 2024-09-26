#include "HIDUSBController.h"
#include "tusb.h"
#include "calypso-debug.h"

using namespace calypso;

void NullUSBReportHandler::handleReport(uint8_t const* report, uint16_t length) {
    USB_DEBUG_LOG(L_DEBUG, "NullUSBReportHandler handling report of size %d\n", length);
}

bool HIDUSBController::addKbdReportHandler(USBReportHandler* handler) {
    for (uint8_t i = 0; i < HIDUSBController::MAX_KBD_HANDLERS; i++) {
        if (m_kbdReportHandlers[i] == nullptr) {
            m_kbdReportHandlers[i] = handler;
            return true;
        }
    }
    USB_DEBUG_LOG(L_WARN, "Unable to add Keyboard report handler. Reached max number of handlers\n");
    return false;
}

bool HIDUSBController::addMouseReportHandler(USBReportHandler* handler) {
    for (uint8_t i = 0; i < HIDUSBController::MAX_MOUSE_HANDLERS; i++) {
        if (m_mouseReportHandlers[i] == nullptr) {
            m_mouseReportHandlers[i] = handler;
            return true;
        }
    }
    USB_DEBUG_LOG(L_WARN, "Unable to add mouse report handler. Reached max number of handlers\n");
    return false;
}

void HIDUSBController::forwardReportToHandlers(USBReportHandler* handlers[], uint8_t arraySize, 
    uint8_t const* report, uint16_t len) {
    for (uint8_t i = 0; i < arraySize; i++) {
        if (handlers[i] != nullptr) {
            handlers[i]->handleReport(report, len);
        }
    }
}

void HIDUSBController::handleReport(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD ) {
        forwardReportToHandlers(m_kbdReportHandlers, MAX_KBD_HANDLERS, report, len);
    } else if (itf_protocol == HID_ITF_PROTOCOL_MOUSE ) {
        forwardReportToHandlers(m_mouseReportHandlers, MAX_MOUSE_HANDLERS, report, len);
    }
    tuh_hid_receive_report(dev_addr, instance);
}
//Singleton, must be available if USB support is compiled in
extern HIDUSBController hidUSBController;

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    hidUSBController.handleReport(dev_addr, instance, report, len);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {  
    if (tuh_configuration_set(dev_addr, 1, nullptr, 0)) {
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            USB_DEBUG_LOG(L_ERROR, "Unable to request reports for dev_addr 0x%08x, instance 0x%02x\n", dev_addr, instance);
        }
    } else {
        USB_DEBUG_LOG(L_ERROR, "Error setting USB device configuration\n");
    }
}
