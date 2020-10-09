
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WString.h>
#include <WiFi.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include "Tester.h"
#include "../src/LinkedList.h"
#include "../src/lltoa.h"
#include "../src/cb_delay.h"
#include "../src/Template.h"
#include "../src/ESPUI.h"


int counter;
char lastErrorMsg[100] = {0};
char lastErrorKey[100] = {0};

// class MockWifi: public WiFiClass {
// public:
//     int modeCalled = 0;
//     wifi_mode_t modeSetTo[100];

//     bool modeRets[100];
//     int modeRetNext = 0; 

//     int beginCalled = 0;
//     struct {
//         const char* ssid;
//         const char* passphrase;
//         int32_t channel;
//         const uint8_t* bssid;
//         bool connect;
//     } beginSetTo[100];

//     wl_status_t beginRets[100];
//     int beginRetNext = 0; 

//     uint8_t waitForConnectResults[100];
//     int waitForConnectResultNext = 0;

//     IPAddress localIPs[100];
//     int localIPNext = 0;

//     bool mode(wifi_mode_t m) {
//         modeSetTo[modeCalled] = m;
//         modeCalled++;

//         bool ret = modeRets[modeRetNext];
//         modeRetNext++;
//         return ret;
//     }

//     wl_status_t begin(const char* ssid, const char *passphrase = NULL, int32_t channel = 0, const uint8_t* bssid = NULL, bool connect = true) {
//         beginSetTo[beginCalled].ssid = ssid;
//         beginSetTo[beginCalled].passphrase = passphrase;
//         beginSetTo[beginCalled].channel = channel;
//         beginSetTo[beginCalled].bssid = bssid;
//         beginSetTo[beginCalled].connect = connect;
//         beginCalled++;

//         wl_status_t ret = beginRets[beginRetNext];
//         beginRetNext++;
//         return ret;
//     }

//     uint8_t waitForConnectResult() {
//         uint8_t ret = waitForConnectResults[waitForConnectResultNext];
//         waitForConnectResultNext++;
//         return ret;
//     }

//     IPAddress localIP() {
//         IPAddress ret = localIPs[localIPNext];
//         localIPNext++;
//         return ret;
//     }
    
// };

// class MockEEPROM: public EEPROMClass {
// public:
//     char eeprom[4000] = {0};

//     bool commitRets[100];
//     int commitCalled = 0;

//     size_t writeString(int address, const char* value) {
//         strcpy(&(eeprom[address]), value);
//         return strlen(value);
//     }
//     size_t writeString(int address, String value) {
//         return writeString(address, value.c_str());
//     }
//     //size_t readString(int address, char* value, size_t maxLen);
//     String readString(int address) {
//         char buff[1000];
//         strcpy(buff, &(eeprom[address]));
//         String ret(buff);
//         return ret;
//     }

//     bool commit() {
//         bool ret = commitRets[commitCalled];
//         commitCalled++;
//         return ret;
//     }
// };

// class MockSerial: public HardwareSerial {
// public:
//     int availables[100] = {0};
//     int availableNext = 0;

//     String output = "";
    
//     String inputs[100];
//     int inputNext = 0;

//     MockSerial(): HardwareSerial(0) {}

//     int available() {
//         int ret = availables[availableNext];
//         availableNext++;
//         return ret;
//     }

//     String readString() {
//         String ret = inputs[inputNext];
//         inputNext++;
//         return ret;
//     }
    
//     size_t print(const char* s) {
//         output += s;
//         return strlen(s);
//     }

//     size_t print(int i) {
//         String str(i);
//         return print(str.c_str());
//     }

//     size_t println(const char* s) {
//         size_t ret = print(s);
//         ret += print("\n");
//         return ret;
//     }
// };

int testCallback(void* arg) {
    return ESPUICALL_OK;
}

int main() {
    // env_init();
    Tester tester(true, false);

    Template::setErrorHandler([](const char* msg, const char* key) {
        strcpy(lastErrorMsg, msg);
        strcpy(lastErrorKey, key);
    });

    tester.run("Test for ESPUIWiFiApp with setup", [](Tester* tester) {
        String ssid = "testwifissid";
        String password = "password1234";

        // MockSerial serial;
        // MockWifi wifi;
        // MockEEPROM eeprom;

        counter = 0;  // cb_delay mock counter

        // mock wifi EEPROM data
        // TODO: test scanario when stored data is worng (can not connect)
        // TODO: test when setup done / user not set up wifi
        int EEPROM_addr = 0;
        EEPROM.begin(1000);
        EEPROM.writeString(EEPROM_addr, ssid);
        EEPROM_addr += (strlen(ssid.c_str()) + 1);
        EEPROM.writeString(EEPROM_addr, password);
        EEPROM_addr += (strlen(password.c_str()) + 1);

        Serial.verbose = true;
        Serial.echo = false;
        Serial.output = "";
        Serial.inputNext = 0;
        Serial.inputNextChar = 0;
        Serial.inputMax = 0;
        Serial.availableNext = 0;
        Serial.availableMax = 0;
        Serial.availables[0] = 0;
        Serial.availables[1] = 0;
        Serial.availables[2] = 1;   // sending 'y' to serial port at 3th second
        Serial.inputs[0] = "y";

        Serial.availables[3] = 0;
        Serial.availables[4] = 0;
        Serial.availables[5] = 0;
        Serial.availables[6] = ssid.length(); // 4 x ESPUIWIFIAPP_SETUP_INPUT_WAIT_MS ms later SSID sent
        Serial.inputs[1] = ssid;
        
        Serial.availables[7] = 0;
        Serial.availables[8] = 0;
        Serial.availables[9] = 0;
        Serial.availables[10] = 0;
        Serial.availables[11] = password.length(); // 5 x ESPUIWIFIAPP_SETUP_INPUT_WAIT_MS ms later password sent
        Serial.inputs[2] = password;
        Serial.availableMax = 12;
        Serial.inputMax = 3;
        
        WiFi.waitForConnectResults[0] = WL_CONNECT_FAILED;
        WiFi.waitForConnectResults[1] = WL_CONNECT_FAILED;
        WiFi.waitForConnectResults[2] = WL_CONNECTED;       // connection established at th 3th times (3th second)
        WiFi.waitForConnectResultMax = 3;

        IPAddress ip(192, 168, 1, 123);
        WiFi.localIPs[0] = ip;
        WiFi.localIPMax = 1;

        ESPUIWiFiApp app(&WiFi, [](){ counter++; }, &Serial, &EEPROM);
        app.begin();

        tester->assertEquals(__FL__, "Type 'y' to set up WiFi credentials (waiting for 5 seconds..)\r\n5..\r\n4..\r\n3..\r\nType WiFi SSID:\r\nType WiFi Password:\r\nCredentials are saved..\r\nConnecting to WiFi (SSID: 'testwifissid')...\r\nWiFi connection failed, retry..\r\nWiFi connection failed, retry..\r\nWiFi connected.\r\nLocal IP:\r\n192.168.1.123\r\n", Serial.output.c_str());
        tester->assertTrue(__FL__, counter > 0);
    });

    tester.run("Test for ESPUIWiFiApp without setup", [](Tester* tester) {
        String ssid = "testwifissid";
        String password = "password1234";

        counter = 0;  // cb_delay mock counter

        // mock wifi EEPROM data
        int EEPROM_addr = 0;
        EEPROM.begin(1000);
        EEPROM.writeString(EEPROM_addr, ssid);
        EEPROM_addr += (strlen(ssid.c_str()) + 1);
        EEPROM.writeString(EEPROM_addr, password);
        EEPROM_addr += (strlen(password.c_str()) + 1);

        Serial.output = "";
        Serial.inputNext = 0;
        Serial.inputNextChar = 0;
        Serial.availableNext = 0;
        Serial.availableMax = 0;
        Serial.availables[0] = 0;

        WiFi.waitForConnectResults[0] = WL_CONNECT_FAILED;
        WiFi.waitForConnectResults[1] = WL_CONNECT_FAILED;
        WiFi.waitForConnectResults[2] = WL_CONNECTED;       // connection established at th 3th times (3th second)
        WiFi.waitForConnectResultMax = 3;

        IPAddress ip(192, 168, 1, 123);
        WiFi.localIPs[0] = ip;
        WiFi.localIPMax = 1;

        ESPUIWiFiApp app(&WiFi, [](){ counter++; }, &Serial, &EEPROM);
        app.begin();

        tester->assertEquals(__FL__, "Type 'y' to set up WiFi credentials (waiting for 5 seconds..)\r\n5..\r\n4..\r\n3..\r\n2..\r\n1..\r\nConnecting to WiFi (SSID: 'testwifissid')...\r\nWiFi connection failed, retry..\r\nWiFi connection failed, retry..\r\nWiFi connected.\r\nLocal IP:\r\n192.168.1.123\r\n", Serial.output.c_str());
        tester->assertTrue(__FL__, counter > 0);
    });

    tester.run("Test for ESPUIControl", [](Tester* tester) {
        String restr;

        strcpy(lastErrorMsg, "");
        strcpy(lastErrorKey, "");

        ESPUIControl ctrl1;
        restr = ctrl1.toString();
        tester->assertEquals(__FL__, R"({
    html: `false`,
    target: {
        selector: 'body', 
        all: true, 
        prepend: false
    },
    script: false
})", restr.c_str());
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);

        ESPUIControl ctrl2("<foo id=\"{{ id }}\" name=\"{{ name }}\" class=\"{{ class }}\">bar</foo>", "#parent-elem", false, true, "() => { console.log('hello'); }", "this-class");
        ctrl2.set("id", "this-id");
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);
        

        restr = ctrl2.toString();
        tester->assertEquals(__FL__, R"({
    html: `<foo id="this-id" name="this-id" class="this-class">bar</foo>`,
    target: {
        selector: '#parent-elem', 
        all: false, 
        prepend: true
    },
    script: () => { console.log('hello'); }
})", restr.c_str());
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);


        ESPUIControl ctrl3("<foo id=\"{{ id }}\" name=\"{{ name }}\" class=\"{{ class }}\">bar1</foo>", "#parent-elem", false, true, "() => { console.log('hello'); }", "this-class");
        ctrl3.set("id", "this-id");
        ctrl3.set("name", "this-name");
        restr = ctrl3.toString();
        tester->assertEquals(__FL__, R"({
    html: `<foo id="this-id" name="this-name" class="this-class">bar1</foo>`,
    target: {
        selector: '#parent-elem', 
        all: false, 
        prepend: true
    },
    script: () => { console.log('hello'); }
})", restr.c_str());
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);


        ESPUIControl ctrl4("<foo id=\"{{ id }}\" name=\"{{ name }}\" class=\"{{ class }}\" onclick=\"{{ onclick }}\">bar1</foo>", "#parent-elem", false, true, "() => { console.log('hello'); }", "this-class");
        ctrl4.set("id", "this-id");
        ctrl4.set("name", "this-name");
        ctrl4.set("onclick", testCallback);
        restr = ctrl4.toString();
        String expected(R"OUTPUT({
    html: `<foo id="this-id" name="this-name" class="this-class" onclick="app.socket.send(JSON.stringify({
        call: '{{ callback }}',
        args: [event]
    }))">bar1</foo>`,
    target: {
        selector: '#parent-elem', 
        all: false, 
        prepend: true
    },
    script: () => { console.log('hello'); }
})OUTPUT");
        Template::set(&expected, "callback", (long long)testCallback);
        tester->assertEquals(__FL__, expected.c_str(), restr.c_str());
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);

    });

    tester.run("Test for ESPUIConnection", [](Tester* tester) {
        const String string("/ws");
        AsyncClient cli;
        AsyncWebServer server(80);
        AsyncWebServerRequest* request = new AsyncWebServerRequest(&server, &cli);
        AsyncWebSocket socket("/");
        AsyncWebSocketClient client(request, &socket);
        ESPUIConnection conn(&socket, &client);
        tester->assertEquals(__FL__, (void*)&socket, (void*)conn.getSocket());
        tester->assertEquals(__FL__, (void*)&client, (void*)conn.getClient());
    });

    tester.run("Test for ESPUIControlCounter", [](Tester* tester) {
        String id;

        ESPUIControlCounter counterStart("");
        int first = atoi(counterStart.getId().c_str()); 

        ESPUIControlCounter counterA("testprefixA-");
        id = counterA.getId();
        tester->assertEquals(__FL__, ("testprefixA-" + String(first+1)).c_str(), id.c_str());

        ESPUIControlCounter counterB("testprefixB-");
        id = counterB.getId();
        tester->assertEquals(__FL__, ("testprefixB-" + String(first+2)).c_str(), id.c_str());

        ESPUIControlCounter counterC("testprefixC-");
        id = counterC.getId();
        tester->assertEquals(__FL__, ("testprefixC-" + String(first+3)).c_str(), id.c_str());
    });

    tester.run("Test for Template", [](Tester* tester) {
        String tpl;
        bool result;

        strcpy(lastErrorMsg, "");
        strcpy(lastErrorKey, "");
        tpl = "abcd{{ value }}efgh";
        result = Template::set(&tpl, "value", 1234);
        tester->assertEquals(__FL__, "abcd1234efgh", tpl.c_str());
        tester->assertTrue(__FL__, result);
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);
        result = Template::check(tpl);
        tester->assertTrue(__FL__, result);
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);


        strcpy(lastErrorMsg, "");
        strcpy(lastErrorKey, "");
        tpl = "abcd{{ value }}ef{{ value }}gh";
        result = Template::set(&tpl, "value", 1234);
        tester->assertEquals(__FL__, "abcd1234ef1234gh", tpl.c_str());
        tester->assertTrue(__FL__, result);
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);
        result = Template::check(tpl);
        tester->assertTrue(__FL__, result);
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);


        strcpy(lastErrorMsg, "");
        strcpy(lastErrorKey, "");
        tpl = "ab{{ foo }}cd{{ value }}ef{{ value }}gh";
        result = Template::set(&tpl, "value", 1234);
        tester->assertEquals(__FL__, "ab{{ foo }}cd1234ef1234gh", tpl.c_str());
        tester->assertTrue(__FL__, result);
        tester->assertEquals(__FL__, "", lastErrorMsg);
        tester->assertEquals(__FL__, "", lastErrorKey);
        result = Template::check(tpl);
        tester->assertFalse(__FL__, result);
        tester->assertEquals(__FL__, "ERROR: Template variable is unset: %s\n", lastErrorMsg);
        tester->assertEquals(__FL__, "{{ foo }}", lastErrorKey);


        strcpy(lastErrorMsg, "");
        strcpy(lastErrorKey, "");
        tpl = "ab{{ foo }}cd{{ value }}ef{{ value }}gh";
        result = Template::set(&tpl, "bar", 1234);
        tester->assertEquals(__FL__, "ab{{ foo }}cd{{ value }}ef{{ value }}gh", tpl.c_str());
        tester->assertFalse(__FL__, result);
        tester->assertEquals(__FL__, "ERROR: Template key not found: '%s'\n", lastErrorMsg);
        tester->assertEquals(__FL__, "bar", lastErrorKey);
        result = Template::check(tpl);
        tester->assertFalse(__FL__, result);
        tester->assertEquals(__FL__, "ERROR: Template variable is unset: %s\n", lastErrorMsg);
        tester->assertEquals(__FL__, "{{ foo }}", lastErrorKey);
    });

    tester.run("Testing linked list", [](Tester* tester) {
        XLinkedList<int> lst;
        for (int i=0; i<1000; i++) lst.add(i);
        for (int i=0; i<1000; i+=100) tester->assertEquals(__FL__, i, lst.get(i));
        tester->assertEquals(__FL__, 1000, lst.size());
        lst.clear();
        tester->assertEquals(__FL__, 0, lst.size());
        tester->assertTrue(__FL__, true, "It should run without memory leak (test is not correct thus have to check available memore before and after)");
    });

    tester.run("Testing cb_delay", [](Tester* tester) {
        counter = 0;
        cb_delay(1000, []() {
            counter++;
        });
        tester->assertTrue(__FL__, counter > 0);
    });

    tester.run("Testing lltoa", [](Tester* tester) {
        char buff[100];
        long long value = 12345678;
        char* result = lltoa(buff, value);
        tester->assertNotNull(__FL__, result);
        tester->assertEquals(__FL__, buff, result);
        tester->assertEquals(__FL__, "12345678", buff);

        value = -12345678;
        result = lltoa(buff, value);
        tester->assertNotNull(__FL__, result);
        tester->assertEquals(__FL__, buff, result);
        tester->assertEquals(__FL__, "-12345678", buff);

        value = 0;
        result = lltoa(buff, value);
        tester->assertNotNull(__FL__, result);
        tester->assertEquals(__FL__, buff, result);
        tester->assertEquals(__FL__, "0", buff);

        value = 0x1ABCFED;
        result = lltoa(buff, value, 16);
        tester->assertNotNull(__FL__, result);
        tester->assertEquals(__FL__, buff, result);
        tester->assertEquals(__FL__, "1abcfed", buff);
    });
}