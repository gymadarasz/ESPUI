#ifndef ESPUI_H
#define ESPUI_H

#include <WString.h>
#include <ESPAsyncWebServer.h>

class ESPUIControlCounter {
    static int next;
    const char* prefix;
    String id;
public:  
    ESPUIControlCounter(const char* prefix = "espuictrl");
    String getId();
};

// --------

class ESPUIConnection {
    AsyncWebSocket* socket;
    AsyncWebSocketClient* client;
public:
    ESPUIConnection(AsyncWebSocket* socket, AsyncWebSocketClient* client);
    AsyncWebSocket* getSocket();
    AsyncWebSocketClient* getClient();
};


#endif // ESPUI_H