#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <WString.h>

typedef void (*TTemplateErrorHandler)(const char* msg, const char* key);

class Template {
    static const String prefix;
    static const String suffix;
    static void defaultErrorHandler(const char* msg, const char* key);
public:
    static TTemplateErrorHandler errorHandler;
    static bool set(String* tpl, const char* key, String value);
    static bool set(String* tpl, const char* key, const char* value);
    static bool set(String* tpl, const char* key, long long value);
    static void check(String tpl);
};

#endif // TEMPLATE_H