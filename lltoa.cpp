#include <stdio.h>
char* lltoa(char* buff, long long value, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *buff = '\0'; return buff; }

    char* ptr = buff, *ptr1 = buff, tmp_char;
    long long tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return buff;
}