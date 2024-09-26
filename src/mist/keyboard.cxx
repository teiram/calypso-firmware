#include "keyboard.h"
#include "PS2Device.h"
#include <cstdio>
#include <cstring>
#include "calypso-debug.h"

// Modifier bitmask
//  bit  7      6       5       4       3       2       1       0
//  key  RGUI   RALT    RSHIFT  RCTRL   LGUI    LALT    LSHIFT  LCTRL

#define KBD_REPORT_LENGTH   6
#define MOD     0x400

#define LCTRL   0x01
#define LSHIFT  0x02
#define LALT    0x04
#define LGUI    0x08
#define RCTRL   0x10
#define RSHIFT  0x20
#define RALT    0x40
#define RGUI    0x80

static const uint16_t ps2_translations[] = {
    0x03,               // 00: ErrorUndefined
    0x42,               // 01: F9
    0x00,               // 02: Unhandled
    0x3E,               // 03: F5
    0x3C,               // 04: F3
    0x3A,               // 05: F1
    0x3B,               // 06: F2
    0x45,               // 07: F12 (OSD)
    0x00,               // 08: Unhandled
    0x43,               // 09: F10
    0x41,               // 0a: F8
    0x3F,               // 0b: F6
    0x3D,               // 0c: F4
    0x2B,               // 0d: Tab
    0x35,               // 0e: `
    0x67,               // 0f: KP =
    0x00,               // 10: Unhandled
    0x00,               // 11: Unhandled
    MOD | LSHIFT,       // 12: Unhandled
    MOD | LALT,         // 13: Unhandled
    MOD | LCTRL,        // 14: Unhandled
    0x14,               // 15: q
    0x1E,               // 16: 1
    0x00,               // 17: Unhandled
    0x6A,               // 18: F15
    0x00,               // 19: Unhandled
    0x1D,               // 1a: z
    0x16,               // 1b: s
    0x04,               // 1c: a
    0x1A,               // 1d: w
    0x1F,               // 1e: 2
    0x00,               // 1f: Unhandled
    0x00,               // 20: Unhandled
    0x06,               // 21: c
    0x1B,               // 22: x
    0x07,               // 23: d
    0x08,               // 24: e
    0x21,               // 25: 4
    0x20,               // 26: 3
    0x00,               // 27: Unhandled
    0x00,               // 28: Unhandled
    0x2C,               // 29: Space
    0x19,               // 2a: v
    0x09,               // 2b: f
    0x17,               // 2c: t
    0x15,               // 2d: r
    0x22,               // 2e: 5
    0x00,               // 2f: Unhandled
    0x00,               // 30: Unhandled
    0x11,               // 31: n
    0x05,               // 32: b
    0x0B,               // 33: h
    0x0A,               // 34: g
    0x1C,               // 35: y
    0x23,               // 36: 6
    0x00,               // 37: Unhandled
    0x00,               // 38: Unhandled
    0x00,               // 39: Unhandled
    0x10,               // 3a: m
    0x0D,               // 3b: j
    0x18,               // 3c: u
    0x24,               // 3d: 7
    0x25,               // 3e: 8
    0x00,               // 3f: Unhandled
    0x00,               // 40: Unhandled
    0x36,               // 41: ,
    0x0E,               // 42: k
    0x0C,               // 43: i
    0x12,               // 44: o
    0x27,               // 45: 0
    0x26,               // 46: 9
    0x00,               // 47: Unhandled
    0x00,               // 48: Unhandled
    0x37,               // 49: .
    0x38,               // 4a: /
    0x0F,               // 4b: l
    0x33,               // 4c: ;
    0x13,               // 4d: p
    0x2D,               // 4e: -
    0x00,               // 4f: Unhandled
    0x00,               // 50: Unhandled
    0x00,               // 51: Unhandled
    0x34,               // 52: '
    0x00,               // 53: Unhandled
    0x2F,               // 54: [
    0x2E,               // 55: =
    0x00,               // 56: Unhandled
    0x00,               // 57: Unhandled
    0x39,               // 58: Caps Lock
    MOD | RSHIFT,       // 59: Unhandled
    0x28,               // 5a: Return
    0x30,               // 5b: ]
    0x00,               // 5c: Unhandled
    0x32,               // 5d: Europe 1
    0x00,               // 5e: Unhandled
    0x00,               // 5f: Unhandled
    0x00,               // 60: Unhandled
    0x64,               // 61: Europe 2
    0x00,               // 62: Unhandled
    0x00,               // 63: Unhandled
    0x00,               // 64: Unhandled
    0x00,               // 65: Unhandled
    0x2A,               // 66: Backspace
    0x00,               // 67: Unhandled
    0x00,               // 68: Unhandled
    0x59,               // 69: KP 1
    0x00,               // 6a: Unhandled
    0x5C,               // 6b: KP 4
    0x6C,               // 6c: F17
    0x00,               // 6d: Unhandled
    0x00,               // 6e: Unhandled
    0x48,               // 6f: Pause (special key handled inside user_io)
    0x6F,               // 70: F20
    0x63,               // 71: KP .
    0x5A,               // 72: KP 2
    0x5D,               // 73: KP 5
    0x5E,               // 74: KP 6
    0x6D,               // 75: F18
    0x29,               // 76: Escape
    0x68,               // 77: Num Lock
    0x44,               // 78: F11
    0x57,               // 79: KP +
    0x5B,               // 7a: KP 3
    0x56,               // 7b: KP -
    0x55,               // 7c: KP *
    0x6E,               // 7d: F19
    0x69,               // 7e: Scroll Lock
    0x00,               // 7f: Unhandled
    0x00,               // 80: Unhandled
    0x00,               // 81: Unhandled
    0x00,               // 82: Unhandled
    0x40,               // 83: F7
};

static const uint16_t ps2_ext_translations[] = {
    0x00, // 00: Unhandled
    0x00, // 01: Unhandled
    0x00, // 02: Unhandled
    0x00, // 03: Unhandled
    0x00, // 04: Unhandled
    0x00, // 05: Unhandled
    0x00, // 06: Unhandled
    0x00, // 07: Unhandled
    0x00, // 08: Unhandled
    0x00, // 09: Unhandled
    0x00, // 0a: Unhandled
    0x00, // 0b: Unhandled
    0x00, // 0c: Unhandled
    0x00, // 0d: Unhandled
    0x00, // 0e: Unhandled
    0x00, // 0f: Unhandled
    0x00, // 10: Unhandled
    MOD | RALT, // 11: Right alt
    0x00, // 12: Unhandled
    0x00, // 13: Unhandled
    MOD | RCTRL, // 14: Right ctrl
    0x00, // 15: Unhandled
    0x00, // 16: Unhandled
    0x00, // 17: Unhandled
    0x00, // 18: Unhandled
    0x00, // 19: Unhandled
    0x00, // 1a: Unhandled
    0x00, // 1b: Unhandled
    0x00, // 1c: Unhandled
    0x00, // 1d: Unhandled
    0x00, // 1e: Unhandled
    MOD | LGUI, // 1f: LGUI
    0x00, // 20: Unhandled
    0x00, // 21: Unhandled
    0x00, // 22: Unhandled
    0x00, // 23: Unhandled
    0x00, // 24: Unhandled
    0x00, // 25: Unhandled
    0x00, // 26: Unhandled
    0x00, // 27: Unhandled
    0x00, // 28: Unhandled
    0x00, // 29: Unhandled
    0x00, // 2a: Unhandled
    0x00, // 2b: Unhandled
    0x00, // 2c: Unhandled
    0x00, // 2d: Unhandled
    0x00, // 2e: Unhandled
    0x65, // 2f: App
    0x00, // 30: Unhandled
    0x00, // 31: Unhandled
    0x00, // 32: Unhandled
    0x00, // 33: Unhandled
    0x00, // 34: Unhandled
    0x00, // 35: Unhandled
    0x00, // 36: Unhandled
    0x66, // 37: Power
    0x00, // 38: Unhandled
    0x00, // 39: Unhandled
    0x00, // 3a: Unhandled
    0x00, // 3b: Unhandled
    0x00, // 3c: Unhandled
    0x00, // 3d: Unhandled7
    0x00, // 3e: Unhandled
    0x00, // 3f: Unhandled
    0x00, // 40: Unhandled
    0x00, // 41: Unhandled
    0x00, // 42: Unhandled
    0x00, // 43: Unhandled
    0x00, // 44: Unhandled
    0x00, // 45: Unhandled
    0x00, // 46: Unhandled
    0x00, // 47: Unhandled
    0x00, // 48: Unhandled
    0x00, // 49: Unhandled
    0x54, // 4a: KP /
    0x00, // 4b: Unhandled
    0x00, // 4c: Unhandled
    0x00, // 4d: Unhandled
    0x00, // 4e: Unhandled
    0x00, // 4f: Unhandled
    0x00, // 50: Unhandled
    0x00, // 51: Unhandled
    0x00, // 52: Unhandled
    0x00, // 53: Unhandled
    0x00, // 54: Unhandled
    0x00, // 55: Unhandled
    0x00, // 56: Unhandled
    0x00, // 57: Unhandled
    0x00, // 58: Unhandled
    0x00, // 59: Unhandled
    0x58, // 5a: KP Enter
    0x00, // 5b: Unhandled
    0x00, // 5c: Unhandled
    0x00, // 5d: Unhandled
    0x00, // 5e: Unhandled
    0x00, // 5f: Unhandled
    0x00, // 60: Unhandled
    0x00, // 61: Unhandled
    0x00, // 62: Unhandled
    0x00, // 63: Unhandled
    0x00, // 64: Unhandled
    0x00, // 65: Unhandled
    0x00, // 66: Unhandled
    0x00, // 67: Unhandled
    0x00, // 68: Unhandled
    0x4D, // 69: End
    0x00, // 6a: Unhandled
    0x50, // 6b: Left Arrow
    0x4A, // 6c: Home
    0x00, // 6d: Unhandled
    0x00, // 6e: Unhandled
    0x00, // 6f: Unhandled
    0x6B, // 70: insert (for keyrah)
    0x4C, // 71: Delete
    0x51, // 72: Down Arrow
    0x00, // 73: Unhandled
    0x4F, // 74: Right Arrow
    0x52, // 75: Up Arrow
    0x00, // 76: Unhandled
    0x00, // 77: Unhandled
    0x00, // 78: Unhandled
    0x00, // 79: Unhandled
    0x4E, // 7a: Page Down
    0x00, // 7b: Unhandled
    0x46, // 7c: Print Screen
    0x4B, // 7d: Page Up
};

using namespace calypso;
extern PS2Device keyboard;

static uint8_t keyReport[KBD_REPORT_LENGTH];
static uint8_t keyReportClone[KBD_REPORT_LENGTH]; //Since mist_user_io might modify the report
static uint8_t modifiers = 0;
static bool extended = false;
static bool released = false;

static uint32_t pressed[256/32];

static inline uint16_t getCode(uint8_t key) {
    return extended ? 
        key < (sizeof ps2_ext_translations) / 2 ? ps2_ext_translations[key] : 0 :
        key < (sizeof ps2_translations) / 2 ? ps2_translations[key] : 0;
}

void ps2_poll() {
    bool changed = false;
    uint8_t key;
    while (keyboard.hasData()) {
        uint8_t key = keyboard.getData();
        printf("Handling PS2 code 0x%02x\n", key);
        if (key == 0xe0) {
            extended = true;
        } else if (key == 0xf0) {
            released = true;
        } else {
            uint16_t code = getCode(key);
            if (code & MOD) {
                modifiers = released ? modifiers & ~code : modifiers | code;
                changed = true;
            } else {
                uint8_t keyc = code & 0xff;
                if (released) {
                    //Remove the key from the report if there
                    for (uint8_t i = 0; i < KBD_REPORT_LENGTH; i++) {
                        if (keyReport[i] == keyc) {
                            keyReport[i] = 0;
                            changed = true;
                        }
                    }
                } else {
                    //Add to report if not there yet
                    for (uint8_t i = 0; i < KBD_REPORT_LENGTH; i++) {
                        if (keyReport[i] == keyc) {
                            break;
                        } else if (keyReport[i] == 0) {
                            keyReport[i] = keyc;
                            changed = 1;
                            break;
                        }
                    }
                }
            }
            extended = false;
            released = false;
        }
    }
    if (changed) {
        memcpy(keyReportClone, keyReport, KBD_REPORT_LENGTH);
        PS2_DEBUG_DUMP(L_DEBUG, "PS2", keyReportClone, KBD_REPORT_LENGTH);
        user_io_kbd(modifiers, keyReportClone, UIO_PRIORITY_KEYBOARD, 0, 0);
    }
}

