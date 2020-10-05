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

