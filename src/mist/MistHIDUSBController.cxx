#include "MistHIDUSBController.h"
#include "PS3USBHIDDriver.h"
#include "tusb.h"
#include "calypso-debug.h"

using namespace calypso;

extern PS3USBHIDDriver ps3Driver;

MistHIDUSBController::custom_driver_t hid_drivers[] = {
    {
            .vid = 0x054c,
            .pid = 0x0268,
            .driver = ps3Driver
    }
};

MistHIDUSBController::MistHIDUSBController(USBReportHandler* kbdReportHandler,
            USBReportHandler* mouseReportHandler,
            USBReportHandler* joystickReportHandler):
    m_kbdReportHandler(kbdReportHandler),
    m_mouseReportHandler(mouseReportHandler),
    m_joystickReportHandler(joystickReportHandler)
{
    for (int i = 0; i < MAX_HIDS; i++) {
        m_hids[i].used = false;
    }
    m_drivers = hid_drivers;
    m_numDrivers= sizeof(hid_drivers) / sizeof(custom_driver_t);
    USB_DEBUG_LOG(L_INFO, "Number of custom drivers: %d\n", m_numDrivers);
}

int8_t MistHIDUSBController::getFreeHIDEntry() {
    for (int i = 0; i < MAX_HIDS; i++) {
        if (!m_hids[i].used) {
            return i;
        }
    }
    return -1;
}

int8_t MistHIDUSBController::getHIDIndex(uint8_t instance) {
    for (int i = 0; i < MAX_HIDS; i++) {
        if (m_hids[i].used && m_hids[i].instance == instance) {
            return i;
        }
    }
    return -1;
}

USBHIDDriver* MistHIDUSBController::getDriver(uint16_t vid, uint16_t pid) {
    for (uint8_t i = 0; i < m_numDrivers; i++) {
        custom_driver_t *driver = &m_drivers[i];
        if (driver->pid == pid && driver->vid == vid) {
            USB_DEBUG_LOG(L_DEBUG, "Found driver for device %04x/%04x\n", vid, pid);
            return &(driver->driver);
        }
    }
    return nullptr;
}

bool MistHIDUSBController::addHIDEntry(uint8_t deviceAddress, uint8_t instance,
    uint8_t const* report,
    uint16_t reportLength) {
    
    int8_t index = getFreeHIDEntry();
    if (index > -1) {
        USBHIDDriver *driver = nullptr;
        uint16_t pid;
        uint16_t vid;
        if (tuh_vid_pid_get(deviceAddress, &vid, &pid)) {
            driver = getDriver(vid, pid);
        } else {
            USB_DEBUG_LOG(L_WARN, "Unable to get vid/vid for connected device\n");
        }

        if (driver != nullptr && driver->hasCustomReport()) {
            std::pair<uint8_t const*, uint16_t> report = driver->customReport();
            USB_DEBUG_LOG(L_DEBUG, "Using custom report for device %04x/%04x\n", vid, pid);
            parse_report_descriptor(driver->customReport().first,
                driver->customReport().second, &m_hids[index].report);
        } else {
            parse_report_descriptor((uint8_t *) report, reportLength, &m_hids[index].report);
        }
        if (m_hids[index].report.type != REPORT_TYPE_NONE) {
            m_hids[index].instance = instance;
            m_hids[index].used = true;
            //Apply custom behavior if any
            if (driver != nullptr) {
                if (!driver->initialize(deviceAddress, instance)) {
                    USB_DEBUG_LOG(L_WARN, "Error initializing device with address: %d\n", deviceAddress);
                }
            }
            //Inform the proper handler about the new device
            switch (m_hids[index].report.type) {
                case REPORT_TYPE_KEYBOARD:
                    m_kbdReportHandler->addDevice(instance, vid, pid, &m_hids[index].report);
                    break;
                case REPORT_TYPE_MOUSE:
                    m_mouseReportHandler->addDevice(instance, vid, pid, &m_hids[index].report);
                    break;
                case REPORT_TYPE_JOYSTICK:
                    m_joystickReportHandler->addDevice(instance, vid, pid, &m_hids[index].report);
                    break;
            }
            if (!tuh_hid_receive_report(deviceAddress, instance)) {
                USB_DEBUG_LOG(L_ERROR, "Unable to request reports for dev_addr: %d, instance %d\n", deviceAddress, instance);
            }
            return true;
        } else {
            USB_DEBUG_LOG(L_WARN, "Unable to get a valid report from device\n");
        }
    } else {
        USB_DEBUG_LOG(L_WARN, "No free HID entries\n");
    }
    return false;
}


void MistHIDUSBController::removeHIDEntry(uint8_t instance) {
    for (int i = 0; i < MAX_HIDS; i++) {
        if (m_hids[i].used && m_hids[i].instance == instance) {
            m_hids[i].used = false;
        }
    }
}

void MistHIDUSBController::handleReport(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    USB_DEBUG_LOG(L_DEBUG, "handleReport(addr: %d, instance: %d)\n", dev_addr, instance);
    USB_DEBUG_DUMP(L_DEBUG, "report", report, len);
    int8_t index = getHIDIndex(instance);
    if (index > -1) {
        switch (m_hids[index].report.type) {
            case REPORT_TYPE_KEYBOARD:
                m_kbdReportHandler->handleReport(instance, report, len);
                break;
            case REPORT_TYPE_MOUSE:
                m_mouseReportHandler->handleReport(instance, report, len);
                break;
            case REPORT_TYPE_JOYSTICK:
                m_joystickReportHandler->handleReport(instance, report, len); 
                break;
            default:
                USB_DEBUG_LOG(L_WARN, "Unexpected report type\n");
        }
    } else {
        USB_DEBUG_LOG(L_INFO, "Received report for unregistered device\n");
    }
    tuh_hid_receive_report(dev_addr, instance);
}

//Singleton, must be available if USB support is compiled in
extern MistHIDUSBController mistHIDUSBController;

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    mistHIDUSBController.handleReport(dev_addr, instance, report, len);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    USB_DEBUG_LOG(L_INFO, "Mounted device addr: %d, instance: %d, hids:%d, protocol: %d\n", 
        dev_addr, instance, tuh_hid_itf_get_count(dev_addr),
        tuh_hid_interface_protocol(dev_addr, instance));
    USB_DEBUG_DUMP(L_DEBUG, "desc_report", desc_report, desc_len);
    if (!mistHIDUSBController.addHIDEntry(dev_addr, instance, desc_report, desc_len)) {
        USB_DEBUG_LOG(L_WARN, "Unable to register HID Entry for device with addr %d, instance %d\n",
            dev_addr, instance);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    USB_DEBUG_LOG(L_DEBUG, "Disconnected device with addr: %d, instance: %d\n",
        dev_addr, instance);
    mistHIDUSBController.removeHIDEntry(instance);
}
