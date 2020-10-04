#include "ESPUI.h"

int ESPUIControlCounter::next = 0;

ESPUIControlCounter::ESPUIControlCounter(const char* prefix): prefix(prefix) {
    id = prefix + String(++next);
}

String ESPUIControlCounter::getId() {
    return id;
}
