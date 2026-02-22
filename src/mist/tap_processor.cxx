#include "tap_processor.h"
#include "calypso-debug.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "fat_compat.h"
#include "user_io.h"
#include "menu.h"
#include "menu-8bit.h"
#ifdef __cplusplus
}
#endif

#include "TapeService.h"
#include "TzxTapeParser.h"
#include "TapTapeParser.h"
#include "C64TapParser.h"
#include "Apple1BinParser.h"
#include "FileStreamAdaptor.h"
#include <cstring>

using namespace calypso;

extern TapeService tapeService;
extern TzxTapeParser tzxTapeParser;
extern TapTapeParser tapTapeParser;
extern C64TapParser c64TapParser;
extern Apple1BinParser apple1BinParser;

FileStreamAdaptor adaptor;
TapeParser *parser;
static char extension[4];
static char currentFilename[64] = {0};

static TapeParser* getTapeParser(const char *format, const char *machine) {
    TAPE_DEBUG_LOG(L_DEBUG, "getTapeParser format=%s, machine=%s\n", format, machine);
    strncpy(extension, format, 3);
    if (!strcasecmp(format, "CDT") || !strcasecmp(format, "TZX")) {
        return &tzxTapeParser;
    } else if (!strcasecmp(format, "TAP") && !strcasecmp(machine, "C64")) {
        return &c64TapParser;
    } else if (!strcasecmp(format, "BIN") && !strcasecmp(machine, "Apple-I")) {
        return &apple1BinParser;
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


static char file_selected(uint8_t idx, const char *filename) {
    TAPE_DEBUG_LOG(L_DEBUG, "Selected file: %s\n", filename);
	FIL file;

	if (f_open(&file, filename, FA_READ) == FR_OK) {
        adaptor.close();
        adaptor.attach(&file);
        if (!tapeService.insert(&adaptor, parser)) {
            adaptor.close();
            currentFilename[0] = 0;
        } else {
            strncpy(currentFilename, filename, 64);
        }
	}
	return 0;
}


static char msg[32];

static char getmenuitem(uint8_t idx, char action, menu_item_t* item) {
    switch (idx) {
        case 0:
            if (action == MENU_ACT_GET) {
                item->item = (char*) "\x19 Open File";
                item->active = !tapeService.playing();
                item->stipple = tapeService.playing();
            } else if (action == MENU_ACT_SEL) {
                SelectFileNG(extension, SCAN_DIR | SCAN_LFN, file_selected, 1);
            }
            break;
        case 1:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (tapeService.playing() ? "\x19 STOP" : "\x19 PLAY");
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
        case 2:
            if (action == MENU_ACT_GET) {
                item->item = (char*) "\x19 REWIND";
                item->active = tapeService.inserted() && !tapeService.playing();
                item->stipple = !tapeService.inserted() || tapeService.playing();
            } else if (action == MENU_ACT_SEL) {
                tapeService.tapeParser()->rewind(adaptor);
            }
            break;
        case 4:
            if (action == MENU_ACT_GET) {
                item->item = currentFilename[0] ? currentFilename : (char *)"";
                item->active = 0;
                item->stipple = 0;
            }
            break;
        case 5:
            if (action == MENU_ACT_GET) {
                snprintf(msg, 32, "Driver: %s", tapeService.tapeParserType());
                item->item = msg;
                item->active = 0;
                item->stipple = !tapeService.inserted();
            }
            break;
        case 6:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (tapeService.inserted() ? tapeService.currentStatus() : "");
                item->active = 0;
                item->stipple = !tapeService.inserted();
            }
            break;
        case 7:
            if (action == MENU_ACT_GET) {
                snprintf(msg, 32, "Start Block: %d", tapeService.startBlock());
                item->item = msg;
                item->active = tapeService.inserted() && !tapeService.playing();
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
    return idx > 7 ? 0 : 1;
}

static void init_menu(const char *format, const char *machine) {
    parser = getTapeParser(format, machine);
    if (parser != nullptr) {
        SetupMenu(&getmenupage, &getmenuitem, NULL);
    } else {
        ErrorMessage("Unsupported tape format", 255);
    }
}

menu_page_plugin_t TAP_PLAYER = {
    .id = "TAP",
    .init_menu = &init_menu
};

uint8_t tap_player_register() {
    return page_plugin_add(&TAP_PLAYER);
}
