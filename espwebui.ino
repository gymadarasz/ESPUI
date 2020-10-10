#include "ESPUI.h"
#include "ctrls/basics.cpp";
#include "ctrls/timer.cpp";


// ---------------

ESPUIApp app(80, "/ws");

int onButton1Click(void* args) {
    Serial.println("CLICKED!");
    return ESPUICALL_OK;
}

int countDown = 3;

void setup()
{
    const int baudrate = 115200;
    const int eepromSize = 1000;

    Serial.begin(115200);

    if (!EEPROM.begin(eepromSize)) {
        Serial.println("EEPROM init failure.");
        ESP.restart();
    }


    ESPUIControl header1(htmlCtrlHeader);
    header1.set("text", "My Test Header Line");
    app.add(header1);
    
    ESPUIControl label1(htmlCtrlLabel);
    label1.set("text", "My Test Label");
    label1.set("name", "myLabel1");
    app.add(label1);
    
    ESPUIControl button1(htmlCtrlButton);
    button1.set("onclick", onButton1Click);
    button1.set("text", "My Test Callback Button");
    app.add(button1);

    ESPUIScript timerInit(jsCtrlTimerInit);
    app.add(timerInit);

    ESPUIScript timer1(jsCtrlTimer);
    timer1.set("name", "myTimer1");
    timer1.set("period", 1000);
    timer1.set("onTick", [countDown](void* args) {
        Serial.printf("Front end timer tick call back, stop timer after %d call back..\n", countDown);
        countDown--;
        if (!countDown) {
            app.call("timers", "stop", "[\"myTimer1\"]");
            countDown = 3;
        }
        return ESPUICALL_OK;
    });
    app.add(timer1);

    app.begin();
}

long last = 0;
void loop() {
    app.establish();

    long now = millis();
    if (now-last > 300) {
        last = now;

        app.setByName("myLabel1", "innerHTML", String(String(now) + "ms"));
    }
}

// TODO: tests
