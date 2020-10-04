#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tester.h"

#define TESTER_CHECK(cond) { \
    if (cond) { \
        success++; \
        printf("."); \
    } else { \
        failed++; \
        printf("\nTest failed at: %s:%d\nExpectaion: %s", file, line, expectation); \
        if (dieFast) stat(); \
    } \
}

#define TESTER_CHECK_STR(cond) { \
    if (cond) { \
        success++; \
        printf("."); \
    } else { \
        failed++; \
        printf("\nTest failed at: %s:%d\nExpectaion: %s", file, line, expectation); \
        printf("\nExpected:\n%s\nGiven result:\n%s", expected, result); \
        if (dieFast) stat(); \
    } \
}

Tester::Tester(bool verbose, bool dieFast): verbose(verbose), dieFast(dieFast) {
    printf("Unit test runner,\nauthor: <gyula.madarasz@gmail.com>");
}

Tester::~Tester() {
    stat();
} 

void Tester::run(const char* scenario, TTestFunction testfn) {
    if (verbose) printf("\nScenario: %s\n", scenario);
    testfn(this);
}

void Tester::assertNull(const char* file, int line, void* result, const char* expectation) {
    TESTER_CHECK(nullptr == result);
}

void Tester::assertNotNull(const char* file, int line, void* result, const char* expectation) {
    TESTER_CHECK(nullptr != result);
}

void Tester::assertEquals(const char* file, int line, void* expected, void* result, const char* expectation) {
    TESTER_CHECK(expected == result);
}

void Tester::assertNotEquals(const char* file, int line, void* expected, void* result, const char* expectation) {
    TESTER_CHECK(expected != result);
}

void Tester::assertEquals(const char* file, int line, const char* expected, const char* result, const char* expectation) {
    TESTER_CHECK_STR(expected && result && !strcmp(expected, result));
}

void Tester::assertNotEquals(const char* file, int line, const char* expected, const char* result, const char* expectation) {
    TESTER_CHECK_STR(expected && result && strcmp(expected, result));
}

void Tester::stat() {
    printf("\nAll tests finished, assertations: %d\nSuccess: %d\nFailed: %d\n", success+failed, success, failed);
    if (!failed) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) failed.\n", failed);
        exit(-1);
    }
}
