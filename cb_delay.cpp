#include <Arduino.h>
#include "cb_delay.h"

void cb_delay(unsigned long ms, cb_delay_func_t callback) {
    ms += millis();
    while(millis() <= ms) if (callback) callback(); 
}