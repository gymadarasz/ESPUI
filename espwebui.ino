#include <ESPAsyncWebServer.h>
#include <functional>
#include "EEPROM.h"
#include "LinkedList.h"
#include "ArduinoJson.h"
#include <stdlib.h>

char* lltoa(char* buff, long long value, int base = 10) {
    // check that the base if valid
    if (base < 2 || base > 36) { *buff = '\0'; return buff; }

    char* ptr = buff, *ptr1 = buff, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return buff;
}


typedef void (*cb_delay_callback_func_t)(void);

void cb_delay(long ms, cb_delay_callback_func_t callback = nullptr) {
    ms += millis();
    while(millis() < ms) if (callback) callback(); 
}

typedef void (*TTemplateErrorHandler)(const char* msg, const char* key);

class Template {
    static const String prefix;
    static const String suffix;
    static void defaultErrorHandler(const char* msg, const char* key);
public:
    static TTemplateErrorHandler errorHandler;
    static bool set(String* tpl, const char* key, String value);
    static void check(String tpl);
};

const String Template::prefix = "{{ ";
const String Template::suffix = " }}";

TTemplateErrorHandler Template::errorHandler = Template::defaultErrorHandler;

void Template::defaultErrorHandler(const char* msg, const char* key) {
    Serial.printf(msg, key);
}

bool Template::set(String* tpl, const char* key, String value) {
    String search = prefix + key + suffix;
    if (tpl->indexOf(search) < 0) {
        errorHandler("ERROR: Template key not found: '%s'\n", key);
        return false;
    }
    tpl->replace(search, value);
    return true;
}

void Template::check(String tpl) {
    size_t prefixAt = tpl.indexOf(prefix);
    size_t suffixAt = tpl.indexOf(suffix);
    if (prefixAt >= 0 && suffixAt > prefixAt) {
        String substr = tpl.substring(prefixAt, suffixAt + suffix.length());
        errorHandler("ERROR: Template variable is unset: %s\n", substr.c_str());
    }
}

class ESPUIControlCounter {
    static int next;
    const char* prefix;
    String id;
public:  
    ESPUIControlCounter(const char* prefix = "espuictrl"): prefix(prefix) {
        id = prefix + String(next);
        next++;
    }
    String getId() {
        return id;
    }
};

int ESPUIControlCounter::next = 0;

class ESPUIConnection {
    AsyncWebSocket* socket;
    AsyncWebSocketClient* client;
public:
    ESPUIConnection(AsyncWebSocket* socket, AsyncWebSocketClient* client);
    AsyncWebSocket* getSocket();
    AsyncWebSocketClient* getClient();
};

ESPUIConnection::ESPUIConnection(AsyncWebSocket* socket, AsyncWebSocketClient* client): socket(socket), client(client) {}

AsyncWebSocket* ESPUIConnection::getSocket() {
    return socket;
}

AsyncWebSocketClient* ESPUIConnection::getClient() {
    return client;
}

#define CB_OK 0
#define CB_ERR 1

typedef int (*TESPUICallback)(void*);

typedef void (*errfn_t)(const char* arg);

class ESPUIControl: public ESPUIControlCounter {

    const char* tpl = R"TPL(
        {
            html: `{{ html }}`,
            target: {
                selector: '{{ selector }}', 
                all: {{ all }}, 
                prepend: {{ prepend }}
            },
            script: {{ script }}
        }
    )TPL";

    String html;
    const char* selector;
    bool all;
    bool prepend;
    const char* script;
    const char* clazz;

    String output;

    
public:
    ESPUIControl(String html = "", const char* selector = "body", bool all = true, bool prepend = false, const char* script = NULL, const char* clazz = ""):
        ESPUIControlCounter(), html(html), selector(selector), all(all), prepend(prepend), script(script), clazz(clazz) {}

    bool set(const char* key, const char* value) {
        return Template::set(&html, key, value);
    }

    bool set(const char* key, long long value) {
        char buff[32];
        return set(key, lltoa(buff, value));
    }

    bool set(const char* key, TESPUICallback value) {
        return set(key, (intptr_t)value);
    }

    String toString() {
        output = tpl;
        Template::set(&output, "html", html.length() ? html : "false");
        Template::set(&output, "selector", selector ? selector : "false");
        Template::set(&output, "all", all ? "true" : "false");
        Template::set(&output, "prepend", prepend ? "true" : "false");
        Template::set(&output, "script", script ? script : "false");
        Template::set(&output, "id", getId());
        Template::set(&output, "class", clazz);
        return output;
    }
};


class ESPUIApp {
    Stream* ioStream;
    EEPROMClass* eeprom;
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    XLinkedList<ESPUIConnection*>* connects;
    String controls;

    String ssid;
    String password;
    cb_delay_callback_func_t whileConnectingLoop;
    void connect();
    String getSetterMessage(String selector, String prop, String content, bool inAllDOMElement = true);
public:
    ESPUIApp(uint16_t port = 80, const char* wsuri = "/ws", cb_delay_callback_func_t whileConnectingLoop = NULL, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~ESPUIApp();
    void add(String control, bool prepend = false);
    void add(ESPUIControl control, bool prepend = false);
    void begin();
    void establish();
    void set(String selector, String prop, String content, bool inAllDOMElement = true);
    void setOne(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement = true);
    void setExcept(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement = true);
    void setById(String id, String prop, String content, bool inAllDOMElement = true);
    void setOneById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement = true);
    void setExceptById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement = true);
};

ESPUIApp::ESPUIApp(uint16_t port, const char* wsuri, cb_delay_callback_func_t whileConnectingLoop, Stream* ioStream, EEPROMClass* eeprom): whileConnectingLoop(whileConnectingLoop), ioStream(ioStream), eeprom(eeprom) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
    connects = new XLinkedList<ESPUIConnection*>();
}

ESPUIApp::~ESPUIApp() {
    delete server;
    delete ws;
    while (connects->size()) delete connects->pop();
    delete connects;
}

void ESPUIApp::add(String control, bool prepend) {
    if (prepend) {
        controls = control + "," + controls;
    } else {
        controls += control + ",";
    }
}

void ESPUIApp::add(ESPUIControl control, bool prepend) {
    add(control.toString(), prepend);
}

void ESPUIApp::begin() {

    const int wsec = 5;
    const long idelay = 300;
    const long wifiaddrStart = 0;

    long wifiaddr;

    // ask if we need to set up wifi credentials

    ioStream->print("Type 'y' to set up WiFi credentials (waiting for ");
    ioStream->print(wsec);
    ioStream->println(" seconds..)");
    String inpstr = "";
    for (int i=wsec; i>0; i--) {
        ioStream->print(i);
        ioStream->println("..");
        cb_delay(1000, whileConnectingLoop);
        if (ioStream->available()) {
            inpstr = ioStream->readString();
            inpstr.trim();
            break;
        }
    }
    if (inpstr == "y") {

        // read new wifi credentials data
        
        ioStream->println("Type WiFi SSID:");
        while (!ioStream->available()) cb_delay(idelay, whileConnectingLoop);
        ssid = ioStream->readString();
        ssid.trim();
        ioStream->println("Type WiFi Password:");
        while (!ioStream->available()) cb_delay(idelay, whileConnectingLoop);
        password = ioStream->readString();
        password.trim();

        // store new wifi credentials

        wifiaddr = wifiaddrStart;
        eeprom->writeString(wifiaddr, ssid);
        wifiaddr += ssid.length() + 1;
        eeprom->writeString(wifiaddr, password);
        eeprom->commit();

        ioStream->println("Credentials are saved..");
    }

    // load wifi credentials

    wifiaddr = wifiaddrStart;
    ssid = eeprom->readString(wifiaddr);
    wifiaddr += ssid.length() + 1;
    password = eeprom->readString(wifiaddr);

    // connecting to wifi
    connect();

    server->on("/", [this](AsyncWebServerRequest *request) {
        String html = R"INDEX_HTML(
            <html>
                <head>
                    <title>Hello World!</title>
                    <style></style>
                    <script>
                        'use strict';

                        class ESPUIAppClient {
                            constructor(controls) {
                                this.controls = controls;

                                this.socket = new WebSocket("ws://{{ host }}/ws");

                                this.socket.onopen = (event) => {
                                    console.log('socket open', event);
                                    // this.socket.send(JSON.stringify({
                                    //     "event": "onSocketOpen",
                                    //     "arguments": [e]
                                    // }));
                                };

                                this.socket.onmessage = (event) => {
                                    console.log(`[message] Data received from server: ${event.data}`, event);
                                    let json = JSON.parse(event.data);
                                    this.set.apply(this, json);
                                };

                                this.socket.onclose = (event) => {
                                    if (event.wasClean) {
                                        console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
                                    } else {
                                        // e.g. server process killed or network down
                                        // event.code is usually 1006 in this case
                                        alert('[close] Connection died');
                                    }
                                };

                                this.socket.onerror = (error) => {
                                    alert(`[error] ${error.message}`);
                                };
                            }

                            show() {
                                this.controls.forEach((ctrl) => {
                                    if (ctrl.target && ctrl.target.selector && ctrl.html) {
                                        (ctrl.target.all ? 
                                            document.querySelectorAll(ctrl.target.selector) : 
                                            [document.querySelector(ctrl.target.selector)]
                                        ).forEach((elem) => {
                                            if (ctrl.target.prepend) {
                                                elem.innerHTML = ctrl.html + elem.innerHTML;
                                            } else {
                                                elem.innerHTML += ctrl.html;
                                            }
                                        });
                                    }
                                    if (typeof ctrl.script === 'function') {
                                        ctrl.script(this);
                                    }
                                });
                            }

                            set(selector, prop, content, all = true) {
                                (all ? document.querySelectorAll(selector) : document.querySelector(selector)).forEach((elem) => {
                                    elem[prop] = content;
                                });
                            }

                        }

                        let app = new ESPUIAppClient([{{ controls }}]);
                        
                    </script>
                </head>
                <body onload="app.show()"></body>
            </html>
        )INDEX_HTML";

        Template::set(&html, "host", WiFi.localIP().toString());
        Template::set(&html, "controls", controls);
        Template::check(html);
        
        request->send(200, "text/html", html);
    });

    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    ws->onEvent([this](AsyncWebSocket *socket, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {

        StaticJsonDocument<2000> doc;
        DeserializationError error;
        ESPUIConnection* conn;

        const char* call;

        switch (type) {

            case WS_EVT_DISCONNECT:
                ioStream->printf("Disconnected!\n");

                for (size_t i = 0; i < connects->size(); i++) {
                    if (connects->get(i)->getClient()->id() == client->id()) {
                        connects->remove(i);
                        i--;
                    }
                }

                break;

            case WS_EVT_PONG:
                ioStream->printf("Received PONG!\n");
                break;

            case WS_EVT_ERROR:
                ioStream->printf("WebSocket Error!\n");
                break;

            case WS_EVT_CONNECT:
                ioStream->print("Connected: ");
                ioStream->println(client->id());
                conn = new ESPUIConnection(socket, client); 
                connects->add(conn);
                break;

            case WS_EVT_DATA:
                ioStream->println("Data:");
                ioStream->println(client->id());
                ioStream->println((char*)data);

                error = deserializeJson(doc, data);
                call = doc["call"];
                if (error) {
                    ioStream->print(F("deserializeJson() failed: "));
                    ioStream->println(error.c_str());
                } else {
                    ioStream->printf("call: %s\n", call);
                    TESPUICallback callfn = (TESPUICallback)atoll(call);
                    if (CB_OK != callfn(&doc)) {
                        ioStream->println("error");
                    }
                }
                break;

            default:
                ioStream->println("Unknown??");
                break;
        }
    });
    server->addHandler(ws);

    server->begin();

}

void ESPUIApp::establish() {
    if (WiFi.status() != WL_CONNECTED) connect();
}

void ESPUIApp::connect() {
    const size_t sSize = 100;
    char ssids[sSize];
    char passwords[sSize];
    ssid.toCharArray(ssids, sSize);
    password.toCharArray(passwords, sSize);
    ioStream->println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    while(true) {
        WiFi.begin(ssids, passwords);
        if (WiFi.waitForConnectResult() == WL_CONNECTED) break;
        ioStream->println("WiFi connection failed, retry..");
        cb_delay(1000, whileConnectingLoop);
    }

    ioStream->println("WiFi connected.\nLocal IP:");
    ioStream->println(WiFi.localIP().toString());
}

String ESPUIApp::getSetterMessage(String selector, String prop, String content, bool inAllDOMElement) {
    String msg("[\"{{ selector }}\", \"{{ prop }}\", \"{{ content }}\", {{ all }}]");
    Template::set(&msg, "selector", selector);
    Template::set(&msg, "prop", prop);
    Template::set(&msg, "content", content);
    Template::set(&msg, "all", inAllDOMElement ? "true" : "false");
    Template::check(msg);
    return msg;
}

void ESPUIApp::set(String selector, String prop, String content, bool inAllDOMElement) {
    String msg = getSetterMessage(selector, prop, content, inAllDOMElement);
    ws->textAll(msg);
}

void ESPUIApp::setOne(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement) {
    String msg = getSetterMessage(selector, prop, content, inAllDOMElement);
    ws->text(conn->getClient()->id(), msg);
}

void ESPUIApp::setExcept(ESPUIConnection* conn, String selector, String prop, String content, bool inAllDOMElement) {
    String msg = getSetterMessage(selector, prop, content, inAllDOMElement);
    size_t size = connects->size();
    AsyncWebSocketClient* cli = conn->getClient();
    for (size_t i=0; i<size; i++) {
        AsyncWebSocketClient* client = connects->get(i)->getClient();
        if (cli->id() != client->id())
            ws->text(client->id(), msg);
    }
}

void ESPUIApp::setById(String id, String prop, String content, bool inAllDOMElement) {
    set("#" + id, prop, content, inAllDOMElement);
}

void ESPUIApp::setOneById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement) {
    set("#" + id, prop, content, inAllDOMElement);
}

void ESPUIApp::setExceptById(ESPUIConnection* conn, String id, String prop, String content, bool inAllDOMElement) {
    set("#" + id, prop, content, inAllDOMElement);
}



// ---------------

ESPUIApp app(80, "/ws"/*, [app]() {

}, &Serial, &EEPROM*/);

int onButton1Click(void* args) {
    Serial.println("CLICKED!");
    return CB_OK;
}

String label1id;

void setup()
{
    const int baudrate = 115200;
    const int eepromSize = 1000;

    Serial.begin(115200);

    if (!EEPROM.begin(eepromSize)) {
        Serial.println("EEPROM init failure.");
        ESP.restart();
    }


    const char* header_html = R"HTML(
        <h1 id="{{ id }}" class="{{ class }}">{{ text }}</h1>
    )HTML";

    const char* label_html = R"HTML(
        <label id="{{ id }}" class="{{ class }}">{{ text }}</label>
    )HTML";
    
    const char* button_html = R"HTML(
        <button id="{{ id }}"  class="{{ class }}" onclick="app.socket.send(JSON.stringify({
            call: '{{ callback }}',
            args: [event]
        }))">{{ text }}</button>
    )HTML";

    const char* input_html = R"HTML(
        <input id="{{ id }}" class="{{ class }}" type="{{ type }}" value="{{ value }}" placeholder="{{ placeholder }}">
    )HTML";

    const char* select_html = R"HTML(
        <select id="{{ id }}" class="{{ class }}" {{ multiple }}>
            {{ options }}
        </select>
    )HTML";

    const char* option_html = R"HTML(
        <option id="{{ id }}" class="{{ class }}" value="{{ value }}" {{ selected }}>{{ text }}</option>
    )HTML";


    ESPUIControl header1(header_html);
    header1.set("text", "My Test Header Line");
    app.add(header1);
    
    ESPUIControl label1(label_html);
    label1.set("text", "My Test Label");
    app.add(label1);
    label1id = label1.getId();
    
    ESPUIControl button1(button_html);
    button1.set("callback", onButton1Click);
    button1.set("text", "My Test Callback Button");
    app.add(button1);

    app.begin();

}

long last = 0;
void loop() {
    app.establish();

    long now = millis();
    if (now-last > 1000) {
        last = now;

        app.setById(label1id, "innerHTML", String(String(now) + "ms"));
    }
}
