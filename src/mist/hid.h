#ifndef HID_H
#define HID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

uint8_t joystick_count();
void joystick_poll();
int8_t hid_keyboard_present();
uint8_t get_keyboards();
uint8_t get_mice();
void hid_joystick_button_remap_init();
void hid_set_kbd_led(uint8_t led, bool on);
int8_t hid_joystick_button_remap(char *s, int8_t action, int tag);
#ifdef __cplusplus
}
#endif

#endif
