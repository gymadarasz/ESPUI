
#include <ESPAsyncWebServer.h>
#include <functional>
#include "EEPROM.h"

class WiFiAppClient {
    AsyncWebSocket* socket;
    AsyncWebSocketClient* client;
public:
    WiFiAppClient(AsyncWebSocket* socket, AsyncWebSocketClient* client);
    AsyncWebSocketClient* getClient();
};

WiFiAppClient::WiFiAppClient(AsyncWebSocket* socket, AsyncWebSocketClient* client): socket(socket), client(client) {}

AsyncWebSocketClient* WiFiAppClient::getClient() {
    return client;
}


class WiFiAppClientList {
    WiFiAppClient** clients;
    size_t size;
    size_t length = 0;
public:
    WiFiAppClientList(size_t size);
    ~WiFiAppClientList();
    bool resize(size_t newSize);
    WiFiAppClient* add(AsyncWebSocket* socket, AsyncWebSocketClient* client);
    void remove(AsyncWebSocketClient* client);
};

WiFiAppClientList::WiFiAppClientList(size_t size): size(size) {
    if (size <= 0) size = 1;
    clients = (WiFiAppClient**)calloc(size, sizeof(WiFiAppClient*));
}

WiFiAppClientList::~WiFiAppClientList() {
    for (size_t i = 0; i < size; i++) {
        delete clients[i];
        clients[i] = NULL;
    }
    free(clients);
}

bool WiFiAppClientList::resize(size_t newSize) {
    if (newSize < size) return false;
    WiFiAppClient** newClients = (WiFiAppClient**)realloc(clients, sizeof(WiFiAppClient*) * newSize);
    if (!newClients) return false;
    for (size_t i = size-1; i < newSize; i++) newClients[i] = NULL;
    clients = newClients;
    size = newSize;
    return true;
}

WiFiAppClient* WiFiAppClientList::add(AsyncWebSocket* socket, AsyncWebSocketClient* client) {
    for (size_t i = 0; i < size; i++) {
        if (NULL == clients[i]) return clients[i] = new WiFiAppClient(socket, client);
    }
    if (resize(size * 2)) {
        return add(socket, client);
    }
    return NULL;
}

void WiFiAppClientList::remove(AsyncWebSocketClient* client) {
    for (size_t i = 0; i < size; i++) {
        if (client->id() == clients[i]->getClient()->id()) {
            delete clients[i];
            clients[i] = NULL;
        }
    }
}


class WiFiApp {
    Stream* ioStream;
    EEPROMClass* eeprom;
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    WiFiAppClientList* clientList;
public:
    WiFiApp(uint16_t port = 80, const char* wsuri = "/ws", size_t clientListSize = 10, Stream* ioStream = &Serial, EEPROMClass* eeprom = &EEPROM);
    ~WiFiApp();
    void begin();
};

WiFiApp::WiFiApp(uint16_t port, const char* wsuri, size_t clientListSize, Stream* ioStream, EEPROMClass* eeprom): ioStream(ioStream), eeprom(eeprom) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(wsuri);
    clientList = new WiFiAppClientList(clientListSize);
}

WiFiApp::~WiFiApp() {
    delete server;
    delete ws;
    delete clientList;
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

    ws->onEvent([this](AsyncWebSocket *socket, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_DISCONNECT:
            ioStream->printf("Disconnected!\n");
            clientList->remove(client);
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
            if (!clientList->add(socket, client)) {
                ioStream->println("ERROR: App-Client is not attached.");
            }
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
    WiFiAppClientList clientList();
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