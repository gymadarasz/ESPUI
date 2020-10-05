#include <stdlib.h>
#include <functional>
#include <ESPAsyncWebServer.h>
#include "EEPROM.h"
#include "LinkedList.h"
#include "ArduinoJson.h"
#include "lltoa.h"
#include "cb_delay.h"
#include "Template.h"
#include "ESPUI.h"


class ESPUIApp: public ESPUIWiFiApp {

    AsyncWebServer* server;
    AsyncWebSocket* ws;
    XLinkedList<ESPUIConnection*>* connects;
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

ESPUIApp::ESPUIApp(uint16_t port, const char* wsuri, WiFiClass* wifi, cb_delay_func_t whileConnectingLoop, Stream* ioStream, EEPROMClass* eeprom): 
    ESPUIWiFiApp(wifi, whileConnectingLoop, ioStream, eeprom) {
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

    ESPUIWiFiApp::begin();

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

        Template::set(&html, "host", wifi->localIP().toString()); // todo: bubble up WiFi as dependecy 
        Template::set(&html, "controls", controls);
        Template::check(html);
        
        request->send(200, "text/html", html);
    });

    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    ws->onEvent([this](AsyncWebSocket *socket, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {

        StaticJsonDocument<2000> doc; // todo: config
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

                    bool found = false;
                    for (size_t i=0; i < ESPUICallbacks.size(); i++) {
                        if (ESPUICallbacks.get(i) == callfn) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) ioStream->println("ERROR: Unregistered callback");
                    if (ESPUICALL_OK != callfn(&doc)) ioStream->println("ERROR: Callback responded an error");
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

void ESPUIApp::setByName(String name, String prop, String content, bool inAllDOMElement) {
    set("name=[\"" + name + "\"]", prop, content, inAllDOMElement);
}

void ESPUIApp::setOneByName(ESPUIConnection* conn, String name, String prop, String content, bool inAllDOMElement) {
    set("name=[\"" + name + "\"]", prop, content, inAllDOMElement);
}

void ESPUIApp::setExceptByName(ESPUIConnection* conn, String name, String prop, String content, bool inAllDOMElement) {
    set("name=[\"" + name + "\"]", prop, content, inAllDOMElement);
}



// ---------------

ESPUIApp app(80, "/ws"/*, &WiFi, [app]() {

}, &Serial, &EEPROM*/);

int onButton1Click(void* args) {
    Serial.println("CLICKED!");
    return ESPUICALL_OK;
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

    const char* attribute_html = R"HTML(
        {{ name }}="{{ value }}"
    )HTML";

    const char* tag_html = R"HTML(
        <{{ tag }}{{ attributes }} />
    )HTML";

    const char* tag_empty_html = R"HTML(
        <{{ tag }}{{ attributes }}></{{ tag }}>
    )HTML";

    const char* header_html = R"HTML(
        <h1 id="{{ id }}" class="{{ class }}">{{ text }}</h1>
    )HTML";

    const char* label_html = R"HTML(
        <label id="{{ id }}" class="{{ class }}">{{ text }}</label>
    )HTML";

    const char* input_html = R"HTML(
        <input id="{{ id }}" name="{{ name }}" class="{{ class }}" type="{{ type }}" value="{{ value }}" {{ checked }} placeholder="{{ placeholder }}" onchange="{{ onchange }}">
    )HTML";

    const char* select_html = R"HTML(
        <select id="{{ id }}" name="{{ name }}" class="{{ class }}" {{ multiple }} onchange="{{ onchange }}"></select>
    )HTML";

    const char* option_html = R"HTML(
        <option id="{{ id }}" name="{{ name }}" class="{{ class }}" value="{{ value }}" {{ selected }}>{{ text }}</option>
    )HTML";

    const char* textarea_html = R"HTML(
        <textarea id="{{ id }}" name="{{ name }}" class="{{ class }}" rows="{{ rows }}" cols="{{ cols }}" onchange="{{ onchange }}">{{ text }}</textarea>
    )HTML";
    
    const char* button_html = R"HTML(
        <button id="{{ id }}" name="{{ name }}" class="{{ class }}" onclick="{{ onclick }}">{{ text }}</button>
    )HTML";

    // todo: fieldset and legend
    // todo: datalist (autocomplete)

    const char* output_html = R"HTML(
        <output id="{{ id }}" name="{{ name }}" class="{{ class }}">{{ text }}</output>
    )HTML";

    // todo: canvas (and draw)

    ESPUIControl header1(header_html);
    header1.set("text", "My Test Header Line");
    app.add(header1);
    
    ESPUIControl label1(label_html);
    label1.set("text", "My Test Label");
    app.add(label1);
    label1id = label1.getId();
    
    ESPUIControl button1(button_html);
    button1.set("onclick", onButton1Click);
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

// TODO: tests
