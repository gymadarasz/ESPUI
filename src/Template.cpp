#include <stdio.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "lltoa.h"
#include "Template.h"

const String Template::prefix("{{ ");
const String Template::suffix(" }}");

TTemplateErrorHandler Template::errorHandler = defaultErrorHandler;

void Template::defaultErrorHandler(const char* msg, const char* key) {
    Serial.printf(msg, key);
}

void Template::setErrorHandler(TTemplateErrorHandler handler) {
    errorHandler = handler;
}

bool Template::has(String* tpl, const char* key) {
    String search = prefix + key + suffix;
    return tpl->indexOf(search) >= 0;
}

bool Template::set(String* tpl, const char* key, String value) {
    if (!has(tpl, key)) {
        errorHandler("ERROR: Template key not found: '%s'\n", key);
        return false;
    }
    String search = prefix + key + suffix;
    tpl->replace(search, value);
    return true;
}

bool Template::set(String* tpl, const char* key, const char* value) {
    return set(tpl, key, String(value));
}

bool Template::set(String* tpl, const char* key, long long value) {
    char buff[32];
    return set(tpl, key, lltoa(buff, value));
}

bool Template::check(String tpl) {
    size_t prefixAt = tpl.indexOf(prefix);
    size_t suffixAt = tpl.indexOf(suffix);
    if (prefixAt >= 0 && suffixAt > prefixAt) {
        String substr = tpl.substring(prefixAt, suffixAt + suffix.length());
        errorHandler("ERROR: Template variable is unset: %s\n", substr.c_str());
        return false;
    }
    return true;
}
