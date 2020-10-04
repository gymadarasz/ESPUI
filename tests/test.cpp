#include <stdio.h>
#include <stdlib.h>
#include <WString.h>
#include "Tester.h"
#include "../LinkedList.h"
#include "../lltoa.h"
#include "../cb_delay.h"
#include "../Template.h"

int counter;

int main() {
    Tester tester;

    tester.run("Test for Template", [](Tester* tester) {
        String tpl("abcd{{ value }}efgh");
        Template::set(&tpl, "value", 1234);
        tester->assertEquals(__FL__, "abcd1234efgh", tpl.c_str());
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