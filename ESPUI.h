#ifndef ESPUI_H
#define ESPUI_H

#include <WString.h>

class ESPUIControlCounter {
    static int next;
    const char* prefix;
    String id;
public:  
    ESPUIControlCounter(const char* prefix = "espuictrl");
    String getId();
};


#endif // ESPUI_H