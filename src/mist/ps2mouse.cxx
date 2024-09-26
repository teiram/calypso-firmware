#include "ps2mouse.h"
#include "PS2Device.h"
#include "calypso-debug.h"

using namespace calypso;
extern PS2Device mouse;

void ps2mouse_poll() {

    while (mouse.available() >= 3) {
        uint8_t mouseb = mouse.getData();
        uint8_t mousex = mouse.getData();
        uint8_t mousey = mouse.getData();
        PS2_DEBUG_LOG(L_DEBUG, "PS2 Mouse report (buttons: %02x, mousex: %02x, mousey: %02x)\n", mouseb, mousex, mousey);
        user_io_mouse(0, mouseb & 7, mousex, -mousey, 0);
    }
}





