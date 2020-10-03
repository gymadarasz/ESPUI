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


class SelfCounter {
    static int next;
    const char* prefix;
    String id;
public:  
    SelfCounter(const char* prefix = ""): prefix(prefix) {
        id = prefix + String(next);
        next++;
    }
    String getId() {
        return id;
    }
};

int SelfCounter::next = 0;

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

class ESPUIControl: public SelfCounter {
    static const String prefix;
    static const String suffix;

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

    String output;

    
public:
    ESPUIControl(String html = "", const char* selector = "body", bool all = true, bool prepend = false, const char* script = NULL):
        SelfCounter("ctrl-"), html(html), selector(selector), all(all), prepend(prepend), script(script) {}

    bool set(const char* key, const char* value) {
        String search = prefix + key + suffix;
        if (html.indexOf(search) < 0) return false;
        html.replace(search, value);
        return true;
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
        output.replace(prefix + "html" + suffix, html.length() ? html : "false");
        output.replace(prefix + "selector" + suffix, selector ? selector : "false");
        output.replace(prefix + "all" + suffix, all ? "true" : "false");
        output.replace(prefix + "prepend" + suffix, prepend ? "true" : "false");
        output.replace(prefix + "script" + suffix, script ? script : "false");
        output.replace(prefix + "id" + suffix, getId());
        return output;
    }
};

const String ESPUIControl::prefix = "{{ ";
const String ESPUIControl::suffix = " }}";


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
public:
    ESPUIApp(uint16_t port = 80, const char* wsuri = "/ws", cb_delay_callback_func_t whileConnectingLoop = NULL, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~ESPUIApp();
    void add(String control, bool prepend = false);
    void add(ESPUIControl control, bool prepend = false);
    void begin();
    void establish();
    AsyncWebSocket* getWs();
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
                                    this[json.call]['apply'](this, json.args);
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

                            replace(selector, content, inner = true, all = true) {
                                (all ? document.querySelectorAll(selector) : document.querySelector(selector)).forEach((elem) => {
                                    if (inner) elem.innerHTML = content;
                                    else elem.outerHTML = content;
                                });
                            }
                        }

                        let app = new ESPUIAppClient([{{ controls }}]);
                        
                    </script>
                </head>
                <body onload="app.show()"></body>
            </html>
        )INDEX_HTML";

        html.replace("{{ host }}", WiFi.localIP().toString());
        html.replace("{{ controls }}", controls);
        
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

AsyncWebSocket* ESPUIApp::getWs() {
    return ws;
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
        <h1 id="{{ id }}">{{ label }}</h1>
    )HTML";

    const char* label_html = R"HTML(
        <label id="{{ id }}">{{ label }}</label>
    )HTML";
    
    const char* button_html = R"HTML(
        <button id="{{ id }}" onclick="app.socket.send(JSON.stringify({
            call: '{{ callback }}',
            args: [event]
        }))">{{ label }}</button>
    )HTML";



    ESPUIControl header1(header_html);
    header1.set("label", "My Test Header Line");
    app.add(header1);
    
    ESPUIControl label1(label_html);
    label1.set("label", "My Test Label");
    app.add(label1);
    label1id = label1.getId();
    

    ESPUIControl button1(button_html);
    button1.set("callback", onButton1Click);
    button1.set("label", "My Test Callback Button");
    app.add(button1);

    app.begin();

}

long last = 0;
void loop() {
    app.establish();

    long now = millis();
    if (now-last > 1000) {
        last = now;

        String msg(R"MSG({
            "call": "replace",
            "args": ["#{{ id }}", "{{ content }}", {{ inner }}, {{ all }}]
        })MSG");
        msg.replace("{{ id }}", label1id);
        msg.replace("{{ content }}", String(now) + "ms");
        msg.replace("{{ inner }}", "true");
        msg.replace("{{ all }}", "true");

        app.getWs()->textAll(msg);
    }
}
