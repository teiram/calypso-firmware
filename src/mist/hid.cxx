#include "hid.h"
#include "MistUSBJoystickHandler.h"
#include "MistUSBKeyboardHandler.h"
#include "MistUSBMouseHandler.h"
extern "C" {
    #include "mist-firmware/ini_parser.h"
}
using namespace calypso;

extern MistUSBJoystickHandler usbJoystickHandler;
extern MistUSBKeyboardHandler usbKeyboardHandler;
extern MistUSBMouseHandler usbMouseHandler;

uint8_t joystick_count() {
	return usbJoystickHandler.numDevices();
}

void joystick_poll() {
	usbJoystickHandler.updateJoysticks();
}

int8_t hid_keyboard_present() {
	return usbKeyboardHandler.numDevices() > 0;
}

uint8_t get_keyboards() {
	return usbKeyboardHandler.numDevices();
}

uint8_t get_mice() {
	return usbMouseHandler.numDevices();
}

void hid_joystick_button_remap_init() {
	usbJoystickHandler.clearButtonMappings();
}

void hid_set_kbd_led(uint8_t led, bool on) {

}

int8_t hid_joystick_button_remap(char *s, int8_t action, int tag) {
	if (action == INI_SAVE) {
		return 0;
	} else {
		return usbJoystickHandler.addButtonMapping(s);
	}
}
