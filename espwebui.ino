
#include <ESPAsyncWebServer.h>
#include <functional>
#include "EEPROM.h"

class WifiApp {
    AsyncWebServer* server;
    AsyncWebSocket* ws;
public:
    WifiApp(uint16_t port = 80) {
        server = new AsyncWebServer(port);
        ws = new AsyncWebSocket("/ws");
    }
    ~WifiApp() {
        delete server;
        delete ws;
    }

    void begin() {

        const int wsec = 5;
        const long idelay = 300;
        const long wifiaddrStart = 0;
        const size_t sSize = 100;

        String ssid;
        String password;
        long wifiaddr;

        // ask if we need to set up wifi credentials

        Serial.print("Type 'y' to set up WiFi credentials (waiting for ");
        Serial.print(wsec);
        Serial.println(" seconds..)");
        String inpstr = "";
        for (int i=wsec; i>0; i--) {
            Serial.print(i);
            Serial.println("..");
            delay(1000);
            if (Serial.available()) {
                inpstr = Serial.readString();
                inpstr.trim();
                break;
            }
        }
        if (inpstr == "y") {

            // read new wifi credentials data
            
            Serial.println("Type WiFi SSID:");
            while (!Serial.available()) delay(idelay);
            ssid = Serial.readString();
            ssid.trim();
            Serial.println("Type WiFi Password:");
            while (!Serial.available()) delay(idelay);
            password = Serial.readString();
            password.trim();

            // store new wifi credentials

            wifiaddr = wifiaddrStart;
            EEPROM.writeString(wifiaddr, ssid);
            wifiaddr += ssid.length() + 1;
            EEPROM.writeString(wifiaddr, password);
            EEPROM.commit();

            Serial.println("Credentials are saved..");
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
        Serial.println("Connecting to WiFi...");
        WiFi.mode(WIFI_STA);
        while(true) {
            WiFi.begin(ssids, passwords);
            if (WiFi.waitForConnectResult() == WL_CONNECTED) break;
            Serial.println("WiFi connection failed, retry..");
        }

        Serial.println("WiFi connected.\nLocal IP:");
        Serial.println(WiFi.localIP().toString());

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

        ws->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
            switch (type) {
                case WS_EVT_DISCONNECT:
                Serial.printf("Disconnected!\n");
                break;

                case WS_EVT_PONG:
                Serial.printf("Received PONG!\n");
                break;

                case WS_EVT_ERROR:
                Serial.printf("WebSocket Error!\n");
                break;

                case WS_EVT_CONNECT:
                Serial.print("Connected: ");
                Serial.println(client->id());
                break;

                case WS_EVT_DATA:   
                Serial.println("Data:");
                Serial.println(client->id());
                Serial.println((char*)data);
                break;

                default:
                Serial.println("Unknown??");
                break;
            }
        });
        server->addHandler(ws);

        server->begin();

    }
};

WifiApp app(80);

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

    app.begin();

}

void loop() {}