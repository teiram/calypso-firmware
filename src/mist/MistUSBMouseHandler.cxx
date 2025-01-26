#include "MistUSBMouseHandler.h"
#include "MistUtil.h"
#include "calypso-debug.h"

#include <cstdio>

extern "C" {
    #include "mist-firmware/user_io.h"
    #include "mist-firmware/mist_cfg.h"
}
using namespace calypso;

MistUSBMouseHandler::MistUSBMouseHandler():
    m_numDevices(0) {

    for (uint8_t i = 0; i < MAX_MICE; i++) {
        m_mice[i].used = false;
    }
}

bool MistUSBMouseHandler::addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid, 
    hid_report_t *report) {
    for (uint8_t i = 0; i < MAX_MICE; i++) {
        if (!m_mice[i].used) {
            m_mice[i].deviceId = deviceId;
            m_mice[i].vid = vid;
            m_mice[i].pid = pid;
            m_mice[i].report = report;
            m_mice[i].mouseNumber = m_numDevices++;
            m_mice[i].used = true;
            USB_DEBUG_LOG(L_INFO, "Successfully added mouse %04x/%04x with deviceId: %d\n",
                vid, pid, deviceId);
            return true;
        }
    }
    return false;
}

void MistUSBMouseHandler::removeDevice(uint8_t const deviceId) {
    int8_t mouseNumberRemoved = -1;
    uint8_t i;
    for (i = 0; i < MAX_MICE; i++) {
        if (m_mice[i].used && m_mice[i].deviceId == deviceId) {
            m_mice[i].used = false;
            mouseNumberRemoved = m_mice[i].mouseNumber;
            m_numDevices -= 1;
            USB_DEBUG_LOG(L_DEBUG, "Removing mouse with number %d\n", m_mice[i].mouseNumber);
            break;
        }
    }
    if (mouseNumberRemoved > -1) {
        for (i = 0; i < MAX_MICE; i++) {
            if (m_mice[i].used && m_mice[i].mouseNumber > mouseNumberRemoved) {
                m_mice[i].mouseNumber -= 1;
                USB_DEBUG_LOG(L_DEBUG, "Mouse %04x/%04x renumbered to %d\n", m_mice[i].vid, m_mice[i].pid, m_mice[i].mouseNumber);
            }
        }
    }
}

uint8_t MistUSBMouseHandler::numDevices() const {
    return m_numDevices;
}

int8_t MistUSBMouseHandler::getMouseIndex(uint8_t deviceId) {
    for (uint8_t i = 0; i < MAX_MICE; i++) {
        if (m_mice[i].deviceId == deviceId) {
            return i;
        }
    }
    return -1;
}

void MistUSBMouseHandler::handleReport(uint8_t const deviceId, uint8_t const* report, uint16_t len) {
    USB_DEBUG_DUMP(L_TRACE, "USB Mouse report", report, len);
    int8_t index = getMouseIndex(deviceId);
    if (index > -1) {
        hid_report_t *conf = m_mice[index].report;
        if (len == conf->report_size - (conf->report_id ? 1 : 0)) {
            updateMouseStatus(index, report, len);
        } else if (len == conf->report_size + (conf->report_id ? 1 : 0)) {
            updateMouseStatus(index, report + (conf->report_id ? 1 : 0), len);
        } else {
            USB_DEBUG_LOG(L_WARN, "Unexpected report size: %d, expected: %d, with report id: %d\n", 
                len, conf->report_size, conf->report_id);
            updateMouseStatusFallback(index, report, len);
        }
    } else {
        USB_DEBUG_LOG(L_WARN, "Received report for unknown mouse with deviceId: %d\n", deviceId);
    }
}

void MistUSBMouseHandler::updateMouseStatusFallback(uint8_t index, uint8_t const* report,uint16_t length) {
    if (length >= 3) {
        uint8_t buttons = report[0];
        int8_t x = report[1];
        int8_t y = report[2];
        int8_t z = length > 3 ? report[3] : 0;
        uint8_t mouseNumber = m_mice[index].mouseNumber;
        user_io_mouse(mouseNumber > 1 ? 1 : mouseNumber, buttons, x, y, z);
    }
}

void MistUSBMouseHandler::updateMouseStatus(uint8_t index, uint8_t const* report, uint16_t length) {
    hid_report_t *conf = m_mice[index].report;
    uint8_t buttons = 0;
	uint8_t extraButtons = 0;
	int16_t a[MAX_AXES];
    uint8_t i;
    for (i = 0; i < MAX_AXES; i++) {
		// if logical minimum is > logical maximum then logical minimum 
		// is signed. This means that the value itself is also signed
		bool isSigned = conf->joystick_mouse.axis[i].logical.min > 
				conf->joystick_mouse.axis[i].logical.max;
		a[i] = MistUtil::collectBits(report, conf->joystick_mouse.axis[i].offset, 
			conf->joystick_mouse.axis[i].size, isSigned);
	}
    for (i = 0; i < 4; i++) {
		if (report[conf->joystick_mouse.button[i].byte_offset] & 
		    conf->joystick_mouse.button[i].bitmask) {
            buttons |= 1 << i;
        }
    }

	for (i = 0; i < 3; i++) {
		if (i < 2) {
            a[i] = a[i] * mist_cfg.mouse_speed;
            a[i] /= 100;
        }
        if ((int16_t) a[i] > 127) a[i] = 127;
        if ((int16_t) a[i] < -128) a[i] = -128;
    }
    uint8_t mouseNumber = m_mice[index].mouseNumber;
	user_io_mouse(mouseNumber > 1 ? 1 : mouseNumber, buttons, a[0], a[1], a[2]);
}
