#include "tzx_processor.h"
#include "calypso-debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "fat_compat.h"
#include "data_io.h"
#include "menu.h"
#ifdef __cplusplus
}
#endif

#include "TzxService.h"
#include "FileStreamAdaptor.h"

using namespace calypso;

extern TzxService tzxService;

FileStreamAdaptor adaptor;

static char getmenupage(uint8_t idx, char action, menu_page_t *page) {
    if (action == MENU_PAGE_EXIT) {
        tzxService.eject();
        adaptor.detach();
        return 0;
    }

    page->title = (char*)"TZX Player";
    page->flags = 0;
    page->timer = 50;
    page->stdexit = MENU_STD_EXIT;
    return 0;
}

static char msg[32];

static char getmenuitem(uint8_t idx, char action, menu_item_t* item) {
    switch (idx) {
        case 0:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (tzxService.playing() ? "PAUSE" : "PLAY");
                item->active = 1;
            } else if (action == MENU_ACT_SEL) {
                tzxService.play();
            }
            break;
        case 4:
            if (action == MENU_ACT_GET) {
                item->item = (char*) tzxService.currentStatus();
                item->active = 0;
            }
            break;
        case 5:
            if (action == MENU_ACT_GET) {
                snprintf(msg, 32, "Start Block: %d", tzxService.startBlock());
                item->item = msg;
                item->active = !tzxService.playing();
            } else if (action == MENU_ACT_RIGHT) {
                tzxService.setStartBlock(tzxService.startBlock() + 1);
            } else if (action == MENU_ACT_LEFT) {
                tzxService.setStartBlock(tzxService.startBlock() - 1);
            }
            break;
        default:
            item->item = nullptr;
            item->active = 0;
            break;
    }
    return idx > 5 ? 0 : 1;
}

static void handle_tzx(FIL *file, int index, const char *name, const char *ext) {
    adaptor.attach(file);
    tzxService.insert(&adaptor);

    if (tzxService.tapeLoaded()) {
        SetupMenu(&getmenupage, &getmenuitem, NULL);
    }
}

data_io_processor_t TZX_PROCESSOR = {
    .id = "TZX",
    .file_tx_send = &handle_tzx
};

uint8_t tzx_processor_register() {
    return data_io_add_processor(&TZX_PROCESSOR);
}
