#ifndef BUTTON_H
#define BUTTON_H

#include <inttypes.h>

namespace calypso {
    class Button {
    private:
        uint8_t m_buttonGpio;
    public:
        Button(uint8_t buttonGpio);
        bool init();
        bool read();
    };
}
#endif //BUTTON_H