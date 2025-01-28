#include "MistUSBJoystickHandler.h"
#include "calypso-debug.h"
#include "Configuration.h"
#include "MistUtil.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
    #include "mist-firmware/user_io.h"
    #include "mist-firmware/state.h"
    #include "mist-firmware/mist_cfg.h"
    #include "mist-firmware/usb/joymapping.h"
}
using namespace calypso;


static constexpr int16_t JOYSTICK_AXIS_MIN = 0;
static constexpr int16_t JOYSTICK_AXIS_MID = 127;
static constexpr int16_t JOYSTICK_AXIS_MAX = 255;
static constexpr int16_t JOYSTICK_AXIS_TRIGGER_MIN = 64;
static constexpr int16_t JOYSTICK_AXIS_TRIGGER_MAX = 192;

MistUSBJoystickHandler::MistUSBJoystickHandler():
    m_numDevices(0) {

    for (uint8_t i = 0; i < MAX_JOYSTICKS; i++) {
        m_joysticks[i].used = false;
    }
    for (uint8_t i = 0; i < MAX_BUTTON_MAPPINGS; i++) {
        m_button_mappings[i] = {0, 0, 0, 0};
    }
}

bool MistUSBJoystickHandler::addDevice(uint8_t const deviceId, uint16_t vid, uint16_t pid,
    hid_report_t *report) {
    for (uint8_t i = 0; i < MAX_JOYSTICKS; i++) {
        if (!m_joysticks[i].used) {
            m_joysticks[i].deviceId = deviceId;
            m_joysticks[i].vid = vid;
            m_joysticks[i].pid = pid;
            m_joysticks[i].joyNumber = m_numDevices++;
            m_joysticks[i].report = report;
            m_joysticks[i].used = true;
           	StateNumJoysticksSet(m_numDevices);
            StateUsbIdSet(vid, pid, report->joystick_mouse.button_count, 
                m_joysticks[i].joyNumber);
            return true;
        }
    }
    return false;
}


void MistUSBJoystickHandler::removeDevice(uint8_t const deviceId) {
    int8_t joystickNumberRemoved = -1;
    uint8_t i;
    for (i = 0; i < MAX_JOYSTICKS; i++) {
        if (m_joysticks[i].used && m_joysticks[i].deviceId == deviceId) {
            m_joysticks[i].used = false;
            joystickNumberRemoved = m_joysticks[i].joyNumber;
            m_numDevices -= 1;
            StateNumJoysticksSet(m_numDevices);
            StateUsbIdSet(0, 0, 0, m_numDevices);
            break;
        }
    }
    if (joystickNumberRemoved > -1) {
        for (i = 0; i < MAX_JOYSTICKS; i++) {
            if (m_joysticks[i].used && m_joysticks[i].joyNumber > joystickNumberRemoved) {
                m_joysticks[i].joyNumber -= 1;
                StateUsbIdSet(m_joysticks[i].vid, m_joysticks[i].pid, 
                    m_joysticks[i].report->joystick_mouse.button_count, 
                     m_joysticks[i].joyNumber);
            }
        }
    }
}

uint8_t MistUSBJoystickHandler::numDevices() const {
    return m_numDevices;
}

int8_t MistUSBJoystickHandler::getJoystickIndex(uint8_t const deviceId) const {
    for (uint8_t i = 0; i < MAX_JOYSTICKS; i++) {
        if (m_joysticks[i].deviceId == deviceId) {
            return i;
        }
    }
    return -1;
}

void MistUSBJoystickHandler::clearButtonMappings() {
    for (uint8_t i = 0; i < MAX_BUTTON_MAPPINGS; i++) {
        m_button_mappings[i] = {0, 0, 0, 0};
    }
}

bool MistUSBJoystickHandler::addButtonMapping(const char* s) {

	if (strlen(s) < 13) {
		return false;
	}
	// parse remap request
	for (uint8_t i = 0; i < MAX_BUTTON_MAPPINGS; i++) {
		if (!m_button_mappings[i].vid) {
			// first two entries are comma seperated 
			m_button_mappings[i].vid = strtol(s, NULL, 16);
			m_button_mappings[i].pid = strtol(s+5, NULL, 16);
			m_button_mappings[i].offset = strtol(s+10, NULL, 10);
			s += 10;
            while (*s && (*s != ',')) s++;
            s++;
			m_button_mappings[i].button = strtol(s, NULL, 10);
			return true;
		}
	}
	return false;
}

void MistUSBJoystickHandler::updateJoystickStatus(uint8_t index, uint8_t const* report, uint16_t length) {
    hid_report_t *conf = m_joysticks[index].report;
    uint8_t buttons = 0;
	uint8_t extraButtons = 0;
	int16_t a[MAX_AXES];

    for (uint8_t i = 0; i < MAX_AXES; i++) {
		bool isSigned = conf->joystick_mouse.axis[i].logical.min > 
				conf->joystick_mouse.axis[i].logical.max;
		a[i] = MistUtil::collectBits(report, conf->joystick_mouse.axis[i].offset, 
			conf->joystick_mouse.axis[i].size, isSigned);
	}
    for (uint8_t i = 0; i < 4; i++) {
        USB_DEBUG_LOG(L_TRACE, "Checking for button %d, with byte offset %d and bitmask %d on 0x%02x\n",
            i,
            conf->joystick_mouse.button[i].byte_offset,
            conf->joystick_mouse.button[i].bitmask,
            report[conf->joystick_mouse.button[i].byte_offset]);
		if (report[conf->joystick_mouse.button[i].byte_offset] & 
		    conf->joystick_mouse.button[i].bitmask) {
            buttons |= 1 << i;
        }
    }
			
	for (uint8_t i = 4; i < 12; i++) {
		if (report[conf->joystick_mouse.button[i].byte_offset] & 
			conf->joystick_mouse.button[i].bitmask) {
                extraButtons |= (1<<(i-4));
        }
    }

    uint32_t fullValue = 0;
    if(a[0] < JOYSTICK_AXIS_TRIGGER_MIN) fullValue |= JOY_LEFT;
	if(a[0] > JOYSTICK_AXIS_TRIGGER_MAX) fullValue |= JOY_RIGHT;
	if(a[1] < JOYSTICK_AXIS_TRIGGER_MIN) fullValue |= JOY_UP;
	if(a[1] > JOYSTICK_AXIS_TRIGGER_MAX) fullValue |= JOY_DOWN;
	fullValue |= buttons << JOY_BTN_SHIFT;
    fullValue |= extraButtons << 8;

    //TODO: Maybe include hat support and some other weird stuff in MiST hid.c
    m_joysticks[index].mistJoyStatus = fullValue;
}

void MistUSBJoystickHandler::handleReport(uint8_t const deviceId, uint8_t const* report, uint16_t length) {
    USB_DEBUG_DUMP(L_TRACE, "USB Joystick report", report, length);
    int8_t index = getJoystickIndex(deviceId);
    if (index > -1) {
        hid_report_t *conf = m_joysticks[index].report;
        //Seems the report id is already stripped by tinyusb?
        if (length == conf->report_size - (conf->report_id ? 1 : 0)) {
            updateJoystickStatus(index, report, length);
        } else if (length == conf->report_size + (conf->report_id ? 1 : 0)) {
            updateJoystickStatus(index, report + (conf->report_id ? 1 : 0), length);
        } else {
            USB_DEBUG_LOG(L_WARN, "Unexpected report size: %d, expected: %d, with report id: %d\n", 
                length, conf->report_size, conf->report_id);
        }
    }
}

void MistUSBJoystickHandler::updateJoysticks() {
    for (uint8_t i = 0; i < MAX_JOYSTICKS; i++) {
        if (m_joysticks[i].used) {
            uint32_t value = m_joysticks[i].mistJoyStatus;
            uint8_t joyNumber = m_joysticks[i].joyNumber;
            joyNumber += mist_cfg.joystick0_prefer_db9 ? 1 : 0;

            USB_DEBUG_LOG(L_DEBUG, "New value %08x for joystick with number %d\n",
                value, joyNumber);
            user_io_digital_joystick(joyNumber, value & 0xff);
            user_io_digital_joystick_ext(joyNumber, value);

            StateUsbIdSet(m_joysticks[i].vid, m_joysticks[i].pid, 
                m_joysticks[i].report->joystick_mouse.button_count, joyNumber);
		    StateUsbJoySet(value & 0xff, value >> 8, joyNumber);

            //Send to OSD
            uint8_t osdJoyIndex = mist_cfg.joystick_db9_fixed_index ? joyNumber + 1: joyNumber;
            StateJoySet(value, osdJoyIndex);
		    StateJoySetExtra(value >> 8, osdJoyIndex);

            //Use as OSD menu control
            virtual_joystick_keyboard(value);
        }
    }
}
