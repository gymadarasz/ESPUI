#include "ESPUI.h"
#include "Template.h"

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
