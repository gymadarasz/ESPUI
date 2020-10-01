
#include <ESPAsyncWebServer.h>
#include <functional>
#include "EEPROM.h"

class WiFiApp {
    AsyncWebServer* server;
    AsyncWebSocket* ws;
public:
    WiFiApp(uint16_t port = 80, const char* wsuri = "/ws");
    ~WiFiApp();
    void begin(Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
};

WiFiApp::WiFiApp(uint16_t port, const char* wsuri) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
}

WiFiApp::~WiFiApp() {
    delete server;
    delete ws;
}

void WiFiApp::begin(Stream* ioStream, EEPROMClass* eeprom) {

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
        EEPROM.writeString(wifiaddr, ssid);
        wifiaddr += ssid.length() + 1;
        EEPROM.writeString(wifiaddr, password);
        EEPROM.commit();

        ioStream->println("Credentials are saved..");
    }

    // load wifi credentials

    wifiaddr = wifiaddrStart;
    ssid = EEPROM.readString(wifiaddr);
    wifiaddr += ssid.length() + 1;
    password = EEPROM.readString(wifiaddr);

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
    }

    ioStream->println("WiFi connected.\nLocal IP:");
    ioStream->println(WiFi.localIP().toString());

    server->on("/", [](AsyncWebServerRequest *request) {
        String html = R"INDEX_HTML(
            <html>
                <head>
                    <title>Hello World!</title>
                    <script>
                        let socket = new WebSocket("ws://{{ host }}/ws");

                        socket.onopen = function(e) {
                            alert("[open] Connection established");
                            alert("Sending to server");
                            socket.send("My name is John");
                        };

                        socket.onmessage = function(event) {
                            alert(`[message] Data received from server: ${event.data}`);
                        };

                        socket.onclose = function(event) {
                            if (event.wasClean) {
                                alert(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
                            } else {
                                // e.g. server process killed or network down
                                // event.code is usually 1006 in this case
                                alert('[close] Connection died');
                            }
                        };

                        socket.onerror = function(error) {
                            alert(`[error] ${error.message}`);
                        };
                    </script>
                </head>
                <body>
                    Hello World!
                </body>
            </html>
        )INDEX_HTML";

        html.replace("{{ host }}", WiFi.localIP().toString());
        
        request->send(200, "text/html", html);
    });

    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    ws->onEvent([ioStream](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_DISCONNECT:
            ioStream->printf("Disconnected!\n");
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
            break;

            case WS_EVT_DATA:   
            ioStream->println("Data:");
            ioStream->println(client->id());
            ioStream->println((char*)data);
            break;

            default:
            ioStream->println("Unknown??");
            break;
        }
    });
    server->addHandler(ws);

    server->begin();

}


WiFiApp app(80, "/ws");

// -------------- 



void setup()
{
    const int baudrate = 115200;
    const int eepromSize = 1000;

    Serial.begin(115200);

    if (!EEPROM.begin(eepromSize)) {
        Serial.println("EEPROM init failure.");
        ESP.restart();
    }

    app.begin(&Serial, &EEPROM);

}

void loop() {}