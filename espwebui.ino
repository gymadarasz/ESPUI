#include <ESPAsyncWebServer.h>
#include <functional>
#include "EEPROM.h"
#include "LinkedList.h"

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


class WiFiApp {
    Stream* ioStream;
    EEPROMClass* eeprom;
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    XLinkedList<WiFiAppClient*>* clients;
public:
    WiFiApp(uint16_t port = 80, const char* wsuri = "/ws", size_t clientListSize = 10, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~WiFiApp();
    void begin();
};

WiFiApp::WiFiApp(uint16_t port, const char* wsuri, size_t clientListSize, Stream* ioStream, EEPROMClass* eeprom): ioStream(ioStream), eeprom(eeprom) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
    clients = new XLinkedList<WiFiAppClient*>();
}

WiFiApp::~WiFiApp() {
    delete server;
    delete ws;
    while (clients.size()) delete clients.pop();
    delete clients;
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

    server->on("/", [](AsyncWebServerRequest *request) {
        String html = R"INDEX_HTML(
            <html>
                <head>
                    <title>Hello World!</title>
                    <script>
                        'use strict';

                        class WsAppClient {
                            constructor() {
                                this.socket = new WebSocket("ws://{{ host }}/ws");

                                this.socket.onopen = function(e) {
                                    // alert("[open] Connection established");
                                    // alert("Sending to server");
                                    // socket.send("My name is John");
                                };

                                this.socket.onmessage = function(event) {
                                    // alert(`[message] Data received from server: ${event.data}`);
                                };

                                this.socket.onclose = function(event) {
                                    if (event.wasClean) {
                                        // alert(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
                                    } else {
                                        // e.g. server process killed or network down
                                        // event.code is usually 1006 in this case
                                        alert('[close] Connection died');
                                    }
                                };

                                this.socket.onerror = function(error) {
                                    alert(`[error] ${error.message}`);
                                };

                            }
                        }

                        let app = new WsAppClient();
                        
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

    ws->onEvent([this](AsyncWebSocket *socket, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_DISCONNECT:
            ioStream->printf("Disconnected!\n");

            for (size_t i = 0; i < clients->size(); i++) {
                if (clients->get(i)->getClient()->id() == client->id()) {
                    clients->remove(i);
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
            
            clients->add(new WiFiAppClient(socket, client));
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

// --------------

void tests() {
    
}

// ---------------

WiFiApp app(80, "/ws", 10, &Serial, &EEPROM);


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

    app.begin();

}

void loop() {}