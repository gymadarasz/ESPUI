#include "helpers.h"

unsigned long millis_cnt = 0;
unsigned long millis() {
        return millis_cnt++;
}

void delay(long ms) {

}