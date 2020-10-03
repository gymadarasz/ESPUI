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


class WiFiAppClient {
    AsyncWebSocket* socket;
    AsyncWebSocketClient* client;
public:
    WiFiAppClient(AsyncWebSocket* socket, AsyncWebSocketClient* client);
    AsyncWebSocket* getSocket();
    AsyncWebSocketClient* getClient();
};

WiFiAppClient::WiFiAppClient(AsyncWebSocket* socket, AsyncWebSocketClient* client): socket(socket), client(client) {}

AsyncWebSocket* WiFiAppClient::getSocket() {
    return socket;
}

AsyncWebSocketClient* WiFiAppClient::getClient() {
    return client;
}

#define CB_OK 0
#define CB_ERR 1

typedef int (*call_t)(void*);



class WSAppControl {
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

    const char* html;
    const char* selector;
    bool all;
    bool prepend;
    const char* script;

    String output;
public:
    WSAppControl(const char* html = NULL, const char* selector = NULL, bool all = false, bool prepend = false, const char* script = NULL):
        html(html), selector(selector), all(all), prepend(prepend), script(script) {}

    String toString() {
        output = tpl;
        output.replace("{{ html }}", html ? html : "false");
        output.replace("{{ selector }}", selector ? selector : "false");
        output.replace("{{ all }}", all ? "true" : "false");
        output.replace("{{ prepend }}", prepend ? "true" : "false");
        output.replace("{{ script }}", script ? script : "false");
        return output;
    }
};

class WSAppButtonCtrl: public WSAppControl {
    call_t call;
    String output;
public:
    WSAppButtonCtrl(call_t call, const char* selector = "body", bool all = false, bool prepend = false, const char* script = NULL):
        call(call),  WSAppControl(R"HTML(
            <button onclick="app.socket.send(JSON.stringify({
                call: '{{ call }}', 
                args: [event]
            }))">Test Button</button>
        )HTML", selector, all, prepend, script) {}

    String toString() {
        output = WSAppControl::toString();
        Serial.print("DBG(output):"); Serial.println(output);
        char tmp[32];
        output.replace("{{ call }}", lltoa(tmp, (uintptr_t)call));
        return output;
    }
};


class WiFiApp {
    Stream* ioStream;
    EEPROMClass* eeprom;
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    XLinkedList<WiFiAppClient*>* appClients;
    String controls = "";
public:
    WiFiApp(uint16_t port = 80, const char* wsuri = "/ws", size_t clientListSize = 10, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~WiFiApp();
    void addControl(String control, bool prepend = false);
    void begin();
};

WiFiApp::WiFiApp(uint16_t port, const char* wsuri, size_t clientListSize, Stream* ioStream, EEPROMClass* eeprom): ioStream(ioStream), eeprom(eeprom) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
    appClients = new XLinkedList<WiFiAppClient*>();
}

WiFiApp::~WiFiApp() {
    delete server;
    delete ws;
    while (appClients->size()) delete appClients->pop();
    delete appClients;
}

void WiFiApp::addControl(String control, bool prepend) {
    if (prepend) {
        controls = control + "," + controls;
    } else {
        controls += control + ",";
    }
}

void WiFiApp::begin() {

    const int wsec = 5;
    const long idelay = 300;
    const long wifiaddrStart = 0;
    const size_t sSize = 100;

    String ssid;
    String password;
    long wifiaddr;

    // ask if we need to set up wifi credentials

    ioStream->print("Type 'y' to set up WiFi credentials (waiting for ");
    ioStream->print(wsec);
    ioStream->println(" seconds..)");
    String inpstr = "";
    for (int i=wsec; i>0; i--) {
        ioStream->print(i);
        ioStream->println("..");
        delay(1000);
        if (ioStream->available()) {
            inpstr = ioStream->readString();
            inpstr.trim();
            break;
        }
    }
    if (inpstr == "y") {

        // read new wifi credentials data
        
        ioStream->println("Type WiFi SSID:");
        while (!ioStream->available()) delay(idelay);
        ssid = ioStream->readString();
        ssid.trim();
        ioStream->println("Type WiFi Password:");
        while (!ioStream->available()) delay(idelay);
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
        delay(300);
    }

    ioStream->println("WiFi connected.\nLocal IP:");
    ioStream->println(WiFi.localIP().toString());

    server->on("/", [this](AsyncWebServerRequest *request) {
        String html = R"INDEX_HTML(
            <html>
                <head>
                    <title>Hello World!</title>
                    <style></style>
                    <script>
                        'use strict';

                        class WsAppClient {
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
                        }

                        let app = new WsAppClient([{{ controls }}]);
                        
                    </script>
                </head>
                <body onload="app.show()">
                    Hello World
                </body>
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
        WiFiAppClient* appClient;

        const char* call;

        switch (type) {

            case WS_EVT_DISCONNECT:
                ioStream->printf("Disconnected!\n");

                for (size_t i = 0; i < appClients->size(); i++) {
                    if (appClients->get(i)->getClient()->id() == client->id()) {
                        appClients->remove(i);
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
                appClient = new WiFiAppClient(socket, client); 
                appClients->add(appClient);
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
                    call_t callfn = (call_t)atoll(call);
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



// --------------

void tests() {
    
}

// ---------------

WiFiApp app(80, "/ws", 10, &Serial, &EEPROM);

int onButtonClick(void* args) {
    Serial.println("CLICKED!");
    return CB_OK;
}


void setup()
{
    tests();

    const int baudrate = 115200;
    const int eepromSize = 1000;

    Serial.begin(115200);

    if (!EEPROM.begin(eepromSize)) {
        Serial.println("EEPROM init failure.");
        ESP.restart();
    }

    
    WSAppButtonCtrl button(onButtonClick);
    app.addControl(button.toString());

    WSAppControl control("<h1>MAIN HEADER</h1>", "body", false, true);
    app.addControl(control.toString());

    app.begin();
}

void loop() {}