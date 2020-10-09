#include "src/ESPUI.h"


// ---------------

ESPUIApp app(80, "/ws");

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
