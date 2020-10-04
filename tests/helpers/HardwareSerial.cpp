#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <Arduino.h>
#include "HardwareSerial.h"

#ifndef RX1
#define RX1 9
#endif

#ifndef TX1
#define TX1 10
#endif

#ifndef RX2
#define RX2 16
#endif

#ifndef TX2
#define TX2 17
#endif

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);
#endif

HardwareSerial::HardwareSerial(int uart_nr) : _uart_nr(uart_nr) {}

void HardwareSerial::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert, unsigned long timeout_ms)
{
    if(0 > _uart_nr || _uart_nr > 2) {
        return;
    }
    if(_uart_nr == 0 && rxPin < 0 && txPin < 0) {
        rxPin = 3;
        txPin = 1;
    }
    if(_uart_nr == 1 && rxPin < 0 && txPin < 0) {
        rxPin = RX1;
        txPin = TX1;
    }
    if(_uart_nr == 2 && rxPin < 0 && txPin < 0) {
        rxPin = RX2;
        txPin = TX2;
    }

    if(!baud) {
        unsigned long detectedBaudRate = 0;

        end();

        if(detectedBaudRate) {
            delay(100); // Give some time...
        }
    }
}

void HardwareSerial::updateBaudRate(unsigned long baud)
{
    
}

void HardwareSerial::end()
{
    
}

size_t HardwareSerial::setRxBufferSize(size_t new_size) {
    return 0;
}

void HardwareSerial::setDebugOutput(bool en)
{

}

int HardwareSerial::available(void)
{
    return 0;
}
int HardwareSerial::availableForWrite(void)
{
    return 0;
}

int HardwareSerial::peek(void)
{
    if (available()) {
        
    }
    return -1;
}

int HardwareSerial::read(void)
{
    if(available()) {
        
    }
    return -1;
}

void HardwareSerial::flush()
{
    
}

size_t HardwareSerial::write(uint8_t c)
{
    
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size)
{
    
    return size;
}
uint32_t  HardwareSerial::baudRate()

{
    return 0;    
}
HardwareSerial::operator bool() const
{
    return true;
}
