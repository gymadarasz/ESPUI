#ifndef TESTER_H
#define TESTER_H

#define __FL__ __FILE__, __LINE__

class Tester;

typedef void (*TTestFunction)(Tester*);

class Tester {
    bool verbose;
    bool dieFast;
    int success = 0;
    int failed = 0;
    void stat();
public:
    Tester(bool verbose = true, bool dieFast = false);
    ~Tester();
    void run(const char* scenario, TTestFunction testfn);
    void assertNull(const char* file, int line, void* result, const char* expectation = "Given result should be null");
    void assertNotNull(const char* file, int line, void* result, const char* expectation = "Given result should not be null");
    void assertEquals(const char* file, int line, void* expected, void* result, const char* expectation = "Expected and given result should be same.");
    void assertNotEquals(const char* file, int line, void* expected, void* result, const char* expectation = "Expected and given result should not be same.");
    void assertEquals(const char* file, int line, const char* expected, const char* result, const char* expectation = "Expected and given result strings should be same.");
    void assertNotEquals(const char* file, int line, const char* expected, const char* result, const char* expectation = "Expected and given result strings should not be same.");
};

#endif // TESTER_H