#ifndef ESPUI_H
#define ESPUI_H
#include <WString.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LinkedList.h"
#include "cb_delay.h"

#define ESPUICALL_OK 0
#define ESPUICALL_ERR 1

#define ESPUIWIFIAPP_SETUP_WAIT_SEC 5
#define ESPUIWIFIAPP_SETUP_INPUT_WAIT_MS 300

typedef int (*TESPUICallback)(void*);

static XLinkedList<TESPUICallback> ESPUICallbacks;

// --------

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

// --------

class ESPUIControl: public ESPUIControlCounter {
    static const char* tpl;
    String html;
    const char* selector;
    bool all;
    bool prepend;
    const char* script;
    const char* clazz;
    String output;
    String _id = "";
    String name = "";
public:
    ESPUIControl(String html = "", const char* selector = "body", bool all = true, bool prepend = false, const char* script = NULL, const char* clazz = "");
    bool set(const char* key, const char* value);
    bool set(const char* key, TESPUICallback value);
    String toString();
};

// --------

class ESPUIWiFiApp {
    String ssid;
    String password;
    void connect();
protected:
    WiFiClass* wifi;
private:
    cb_delay_func_t whileConnectingLoop;
protected:
    Stream* ioStream;
private:
    EEPROMClass* eeprom;
public:
    ESPUIWiFiApp(WiFiClass* wifi = &WiFi, cb_delay_func_t whileConnectingLoop = NULL, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    void begin();
    void establish();
};

// --------

class ESPUIApp: public ESPUIWiFiApp {

    AsyncWebServer* server;
    AsyncWebSocket* ws;
    XLinkedList<ESPUIConnection*> connects;
    String controls;

    String getSetterMessage(String selector, String prop, String content, bool inAllDOMElement = true);
public:
    ESPUIApp(uint16_t port = 80, const char* wsuri = "/ws", WiFiClass* wifi = &WiFi, cb_delay_func_t whileConnectingLoop = NULL, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~ESPUIApp();
    void add(String control, bool prepend = false);
    void add(ESPUIControl control, bool prepend = false);
    void begin();
    void set(String selector, String prop, String content, bool inAllDOMElement = true);
    void setOne(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement = true);
    void setExcept(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement = true);
    void setById(String id, String prop, String content, bool inAllDOMElement = true);
    void setOneById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement = true);
    void setExceptById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement = true);
    void setByName(String name, String prop, String content, bool inAllDOMElement = true);
    void setOneByName(ESPUIConnection* conn, String name, String prop, String content, bool inAllDOMElement = true);
    void setExceptByName(ESPUIConnection* conn, String name, String prop, String content, bool inAllDOMElement = true);
};

// --------


#endif // ESPUI_H