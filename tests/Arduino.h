#ifndef ARDUINO_H
#define ARDUINO_H

// Arduino specific mocks

long millis_cnt = 0;
long millis() {
    return millis_cnt++;
}

#endif // ARDUINO_H