#include "helpers.h"

long millis_cnt = 0;
long millis() {
        return millis_cnt++;
}

void delay(long ms) {

}