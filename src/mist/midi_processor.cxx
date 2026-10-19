#include "midi_processor.h"
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

#include "MidiPlayerService.h"
#include "MidiFileParser.h"
#include "FileStreamAdaptor.h"
#include <cstring>

using namespace calypso;

extern MidiPlayerService midiPlayerService;

FileStreamAdaptor midiAdaptor;
MidiFileParser midiParser;
static char extension[4] = "MID";
static char currentFilename[64] = {0};

static char getmenupage(uint8_t idx, char action, menu_page_t *page) {
    if (action == MENU_PAGE_EXIT) {
        return 0;
    }

    page->title = (char*)"MIDI";
    page->flags = 0;
    page->timer = 50;
    page->stdexit = MENU_STD_EXIT;
    return 0;
}

static char file_selected(uint8_t idx, const char *filename) {
    MIDI_DEBUG_LOG(L_DEBUG, "Selected file: %s\n", filename);
    FIL file;

    if (f_open(&file, filename, FA_READ) == FR_OK) {
        midiAdaptor.close();
        // Stop whatever is playing before the open file is replaced under it.
        midiPlayerService.stop();
        midiAdaptor.attach(&file);
        if (!midiPlayerService.insert(&midiAdaptor, &midiParser)) {
            midiAdaptor.close();
            currentFilename[0] = 0;
        } else {
            strncpy(currentFilename, filename, 64);
        }
    }
    return 0;
}

static char msg[32];
static char progressBar[40];

// A solid block (0x7f) per played cell and the hollow box glyph (0x19, the same
// one the menu uses as its bullet) per remaining cell. Reads only the parser's
// byte cursors, so it costs nothing beyond the 50 ms menu repaint.
static const char *renderProgress() {
    const int cells = 24;
    const int fill = (midiPlayerService.progress() * cells) / 100;
    int j = 0;
    progressBar[j++] = '[';
    for (int i = 0; i < cells; i++) {
        progressBar[j++] = (i < fill) ? '\x7f' : '\x19';
    }
    progressBar[j++] = ']';
    progressBar[j] = 0;
    return progressBar;
}

static char getmenuitem(uint8_t idx, char action, menu_item_t* item) {
    switch (idx) {
        case 0:
            if (action == MENU_ACT_GET) {
                item->item = (char*) "\x19 Open File";
                item->active = !midiPlayerService.playing();
                item->stipple = midiPlayerService.playing();
            } else if (action == MENU_ACT_SEL) {
                SelectFileNG(extension, SCAN_DIR | SCAN_LFN, file_selected, 1);
            }
            break;
        case 1:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (midiPlayerService.playing() ? "\x19 STOP" : "\x19 PLAY");
                item->active = midiPlayerService.inserted();
                item->stipple = !midiPlayerService.inserted();
            } else if (action == MENU_ACT_SEL) {
                if (midiPlayerService.playing()) {
                    midiPlayerService.stop();
                } else {
                    midiPlayerService.play();
                }
            }
            break;
        case 2:
            if (action == MENU_ACT_GET) {
                item->item = (char*) "\x19 REWIND";
                item->active = midiPlayerService.inserted() && !midiPlayerService.playing();
                item->stipple = !midiPlayerService.inserted() || midiPlayerService.playing();
            } else if (action == MENU_ACT_SEL) {
                midiPlayerService.rewind();
            }
            break;
        case 3:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (midiPlayerService.inserted() ? renderProgress() : "");
                item->active = 0;
                item->stipple = !midiPlayerService.inserted();
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
                snprintf(msg, 32, "Driver: %s", midiPlayerService.parserType());
                item->item = msg;
                item->active = 0;
                item->stipple = !midiPlayerService.inserted();
            }
            break;
        case 6:
            if (action == MENU_ACT_GET) {
                item->item = (char*) (midiPlayerService.inserted() ? midiPlayerService.currentStatus() : "");
                item->active = 0;
                item->stipple = !midiPlayerService.inserted();
            }
            break;
        case 7:
            if (action == MENU_ACT_GET) {
                MidiFileParser *parser = midiPlayerService.parser();
                snprintf(msg, 32, "Tracks: %d", parser != nullptr ? parser->numTracks() : 0);
                item->item = msg;
                item->active = 0;
                item->stipple = !midiPlayerService.inserted();
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
    if (format != nullptr && format[0] != 0) {
        strncpy(extension, format, 3);
        extension[3] = 0;
    }
    SetupMenu(&getmenupage, &getmenuitem, NULL);
}

menu_page_plugin_t MIDI_PLAYER = {
    .id = "MID",
    .init_menu = &init_menu
};

uint8_t midi_player_register() {
    return page_plugin_add(&MIDI_PLAYER);
}
