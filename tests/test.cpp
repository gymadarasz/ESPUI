#include <stdio.h>
#include <stdlib.h>
#include "../lltoa.h"
#include "Tester.h"

int main() {
    Tester tester;

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