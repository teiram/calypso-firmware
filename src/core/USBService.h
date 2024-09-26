#ifndef USB_SERVICE_H
#define USB_SERVICE_H

#include "Service.h"

namespace calypso {
    class USBService: public Service {
        constexpr static const char NAME[] = {"USBService"};
        const char* name();
        bool init();
        void cleanup();
        bool needsAttention();
        void attention();
    };
}

#endif //USB_SERVICE_H