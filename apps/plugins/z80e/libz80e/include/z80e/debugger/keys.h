#ifndef DEBUGGER_KEYS_H
#define DEBUGGER_KEYS_H

#include <stdint.h>

typedef struct {
    char *key;
    uint8_t value;
} key_string_t;

// TODO: Add aliases
static const key_string_t key_strings[] = {
    { "DOWN", 0x00 },
    { "LEFT", 0x01 },
    { "RIGHT", 0x02 },
    { "UP", 0x03 },
    { "ENTER", 0x10 },
    { "+", 0x11 },
    { "-", 0x12 },
    { "*", 0x13 },
    { "/", 0x14 },
    { "^", 0x15 },
    { "CLEAR", 0x16 },
    { "(-)", 0x20 },
    { "3", 0x21 },
    { "6", 0x22 },
    { "9", 0x23 },
    { ")", 0x24 },
    { "TAN", 0x25 },
    { "VARS", 0x26 },
    { ".", 0x30 },
    { "2", 0x31 },
    { "5", 0x32 },
    { "8", 0x33 },
    { "(", 0x34 },
    { "COS", 0x35 },
    { "PRGM", 0x36 },
    { "STAT", 0x37 },
    { "0", 0x40 },
    { "1", 0x41 },
    { "4", 0x42 },
    { "7", 0x43 },
    { ",", 0x44 },
    { "SIN", 0x45 },
    { "APPS", 0x46 },
    { "XTON", 0x47 },
    { "STO", 0x51 },
    { "LN", 0x52 },
    { "LOG", 0x53 },
    { "X2", 0x54 },
    { "X-1", 0x55 },
    { "MATH", 0x56 },
    { "ALPHA", 0x57 },
    { "GRAPH", 0x60 },
    { "TRACE", 0x61 },
    { "ZOOM", 0x62 },
    { "WIND", 0x63 },
    { "Y=", 0x64 },
    { "2nd", 0x65 },
    { "MODE", 0x66 },
    { "DEL", 0x67 }
};

#endif
