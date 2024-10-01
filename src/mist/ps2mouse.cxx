#include "ps2mouse.h"
#include "PS2Device.h"
#include "calypso-debug.h"

using namespace calypso;
extern PS2Device mouse;

void ps2mouse_poll() {

    while (mouse.available() >= 3) {
        uint8_t mouseb = mouse.getData();
        if (mouseb & 0x08) {
            mouseb &= 0x07; //We discard overflow and sign bits so far
            uint8_t mousex = mouse.getData();
            uint8_t mousey = mouse.getData();
            //If we got
            PS2_DEBUG_LOG(L_DEBUG, "PS2 Mouse report (buttons: %02x, mousex: %02x, mousey: %02x)\n", mouseb, mousex, mousey);
            user_io_mouse(0, mouseb & 7, mousex, -mousey, 0);
        } else {
            PS2_DEBUG_LOG(L_WARN, "PS2 Mouse stream misalignment. 1st byte: 0x%02x\n", mouseb);
        }
    }
}





