#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WString.h>
#include "Tester.h"
#include "../LinkedList.h"
#include "../lltoa.h"
#include "../cb_delay.h"
#include "../Template.h"
#include "../ESPUI.h"

int counter;
char lastErrorMsg[100] = {0};
char lastErrorKey[100] = {0};

int main() {
    Tester tester(true, true);

    tester.run("Test for ESPUIConnection", [](Tester* tester) {
        AsyncWebSocket* socket = new AsyncWebSocket();
        AsyncWebSocketClient* client = new AsyncWebSocketClient();
        ESPUIConnection conn(socket, client);
        tester->assertEquals(__FL__, (void*)socket, (void*)conn.getSocket());
        tester->assertEquals(__FL__, (void*)client, (void*)conn.getClient());
        delete socket;
        delete client;
    });

    tester.run("Test for ESPUIControlCounter", [](Tester* tester) {
        String id;

        ESPUIControlCounter counterA("testprefixA-");
        id = counterA.getId();
        tester->assertEquals(__FL__, "testprefixA-1", id.c_str());

        ESPUIControlCounter counterB("testprefixB-");
        id = counterB.getId();
        tester->assertEquals(__FL__, "testprefixB-2", id.c_str());

        ESPUIControlCounter counterC("testprefixC-");
        id = counterC.getId();
        tester->assertEquals(__FL__, "testprefixC-3", id.c_str());
    });

    tester.run("Test for Template", [](Tester* tester) {
        String tpl;
        bool result;

        Template::setErrorHandler([](const char* msg, const char* key) {
            strcpy(lastErrorMsg, msg);
            strcpy(lastErrorKey, key);
        });

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
        tester->assertEquals(__FL__, 1000, counter);
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