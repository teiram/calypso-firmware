#include "tap_processor.h"
#include "calypso-debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "fat_compat.h"
#include "data_io.h"
#include "menu.h"
#include "menu-8bit.h"
#ifdef __cplusplus
}
#endif

#include "TapeService.h"
#include "TzxTapeParser.h"
#include "TapTapeParser.h"
#include "C64TapParser.h"
#include "FileStreamAdaptor.h"
#include <cstring>

using namespace calypso;

extern TapeService tapeService;
extern TzxTapeParser tzxTapeParser;
extern TapTapeParser tapTapeParser;
extern C64TapParser c64TapParser;

FileStreamAdaptor adaptor;

static TapeParser* getTapeParser(const char *extension) {
    TAPE_DEBUG_LOG(L_DEBUG, "getTapeParser %s\n", extension);
    if (!strcasecmp(extension, "CDT") || !strcasecmp(extension, "TZX")) {
        return &tzxTapeParser;
    } else if (!strcasecmp(extension, "TAP")) {
        return &c64TapParser;
    } else {
        return nullptr;
    }
}

static char getmenupage(uint8_t idx, char action, menu_page_t *page) {
    if (action == MENU_PAGE_EXIT) {
        return 0;
    }

    page->title = (char*)"TAPE";
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
                item->item = (char*) (tapeService.playing() ? "             STOP" : "             PLAY");
                item->active = tapeService.inserted();
                item->stipple = !tapeService.inserted();
            } else if (action == MENU_ACT_SEL) {
                if (tapeService.playing()) {
                    tapeService.stop();
                } else {
                    tapeService.play();
                }
            }
            break;
        case 1:
            if (action == MENU_ACT_GET) {
                item->item = (char *) "             CLOSE";
                item->active = 1;
                item->stipple = 0;
            } else if (action == MENU_ACT_SEL) {
                tapeService.eject();
                clear_sticky_menu_handler();
                CloseMenu();
            }
            break;
        case 3:
            if (action == MENU_ACT_GET) {
                snprintf(msg, 32, "Driver: %s", tapeService.tapeParser()->type());
                item->item = msg;
                item->active = 0;
                item->stipple = !tapeService.inserted();
            }
            break;
        case 4:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (tapeService.inserted() ? tapeService.currentStatus() : "");
                item->active = 0;
                item->stipple = !tapeService.inserted();
            }
            break;
        case 5:
            if (action == MENU_ACT_GET) {
                snprintf(msg, 32, "Start Block: %d", tapeService.startBlock());
                item->item = msg;
                item->active = !tapeService.playing();
                item->stipple = tapeService.playing() || !tapeService.hasBlockSupport();
            } else if (action == MENU_ACT_RIGHT) {
                tapeService.setStartBlock(tapeService.startBlock() + 1);
            } else if (action == MENU_ACT_LEFT) {
                tapeService.setStartBlock(tapeService.startBlock() - 1);
            }
            break;

        default:
            item->item = nullptr;
            item->active = 0;
            break;
    }
    return idx > 6 ? 0 : 1;
}

static void handle_tap(FIL *file, int index, const char *name, const char *ext) {
    TapeParser* parser = getTapeParser(ext);
    if (parser != nullptr) {
        adaptor.attach(file);
        tapeService.insert(&adaptor, parser);
        SetupMenu(&getmenupage, &getmenuitem, NULL);
        set_sticky_menu_handler({.page_handler = getmenupage, .items_handler = getmenuitem, .key_handler = NULL});
    } else {
        ErrorMessage("Unsupported tape format", 255);
    }
}

data_io_processor_t TAP_PROCESSOR = {
    .id = "TAP",
    .file_tx_send = &handle_tap
};

uint8_t tap_processor_register() {
    return data_io_add_processor(&TAP_PROCESSOR);
}
