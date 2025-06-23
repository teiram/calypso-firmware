#include "buttons.h"
#include "Button.h"
#include <stdio.h>

using namespace calypso;

extern Button userButton;

unsigned char MenuButton() {
    return !userButton.read();
}
