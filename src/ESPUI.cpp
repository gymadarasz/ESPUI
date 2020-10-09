#include "ESPUI.h"
#include "Template.h"
#include "ArduinoJson.h"

int ESPUIControlCounter::next = 0;

ESPUIControlCounter::ESPUIControlCounter(const char* prefix): prefix(prefix) {
    id = prefix + String(++next);
}

String ESPUIControlCounter::getId() {
    return id;
}

// --------

ESPUIConnection::ESPUIConnection(AsyncWebSocket* socket, AsyncWebSocketClient* client): socket(socket), client(client) {}

AsyncWebSocket* ESPUIConnection::getSocket() {
    return socket;
}

AsyncWebSocketClient* ESPUIConnection::getClient() {
    return client;
}

// --------

const char* ESPUIControl::tpl = R"TPL({
    html: `{{ html }}`,
    target: {
        selector: '{{ selector }}', 
        all: {{ all }}, 
        prepend: {{ prepend }}
    },
    script: {{ script }}
})TPL";

ESPUIControl::ESPUIControl(String html, const char* selector, bool all, bool prepend, const char* script, const char* clazz):
        ESPUIControlCounter(), html(html), selector(selector), all(all), prepend(prepend), script(script), clazz(clazz) {};

bool ESPUIControl::set(const char* key, const char* value) {
    if (!strcmp(key, "id")) {
        _id = value;
    }
    bool ret = Template::set(&html, key, value);
    return ret;
}

bool ESPUIControl::set(const char* key, TESPUICallback value) {

    String cbjs(R"JS(app.socket.send(JSON.stringify({
        call: '{{ callback }}',
        args: [event]
    })))JS");

    Template::set(&cbjs, "callback", (intptr_t)value);
    Template::check(cbjs);

    ESPUICallbacks.add(value);

    return set(key, cbjs.c_str());
}

String ESPUIControl::toString() {
    output = tpl;
    Template::set(&output, "html", html.length() ? html : "false");
    Template::set(&output, "selector", selector ? selector : "false");
    Template::set(&output, "all", all ? "true" : "false");
    Template::set(&output, "prepend", prepend ? "true" : "false");
    Template::set(&output, "script", script ? script : "false");
    if (html.length() && !_id.length()) Template::set(&output, "id", getId());
    if (Template::has(&output, "name")) Template::set(&output, "name", _id.length() ? _id : getId());
    if (Template::has(&output, "class") || strlen(clazz)) Template::set(&output, "class", clazz);
    return output;
}

// --------

ESPUIWiFiApp::ESPUIWiFiApp(WiFiClass* wifi, cb_delay_func_t whileConnectingLoop, Stream* ioStream, EEPROMClass* eeprom): 
    wifi(wifi), whileConnectingLoop(whileConnectingLoop), ioStream(ioStream), eeprom(eeprom) {}

void ESPUIWiFiApp::begin() {

    const int wsec = ESPUIWIFIAPP_SETUP_WAIT_SEC;
    const long idelay = ESPUIWIFIAPP_SETUP_INPUT_WAIT_MS;
    const long wifiaddrStart = 0; // todo: config
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

}

void ESPUIWiFiApp::establish() {
    if (wifi->status() != WL_CONNECTED) connect();
}

void ESPUIWiFiApp::connect() {
    const size_t sSize = 100;
    char ssids[sSize];
    char passwords[sSize];
    ssid.toCharArray(ssids, sSize);
    password.toCharArray(passwords, sSize);
    ioStream->printf("Connecting to WiFi (SSID: '%s')...", ssids);
    ioStream->println();
    wifi->mode(WIFI_STA);
    while(true) {
        wifi->begin(ssids, passwords);
        if (wifi->waitForConnectResult() == WL_CONNECTED) break;
        ioStream->println("WiFi connection failed, retry..");
        cb_delay(1000, whileConnectingLoop);
    }

    ioStream->println("WiFi connected.");
    ioStream->println("Local IP:");
    ioStream->println(wifi->localIP().toString());
}

// --------

ESPUIApp::ESPUIApp(uint16_t port, const char* wsuri, WiFiClass* wifi, cb_delay_func_t whileConnectingLoop, Stream* ioStream, EEPROMClass* eeprom): 
    ESPUIWiFiApp(wifi, whileConnectingLoop, ioStream, eeprom) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
}

ESPUIApp::~ESPUIApp() {
    delete server;
    delete ws;
    while (connects.size()) delete connects.pop();
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

                for (int i = 0; i < connects.size(); i++) {
                    if (connects.get(i)->getClient()->id() == client->id()) {
                        connects.remove(i);
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
                connects.add(conn);
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
                    for (int i=0; i < ESPUICallbacks.size(); i++) {
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
    size_t size = connects.size();
    AsyncWebSocketClient* cli = conn->getClient();
    for (size_t i=0; i<size; i++) {
        AsyncWebSocketClient* client = connects.get(i)->getClient();
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

// --------
