#include "unit.h"
#define DEGREE_SYMBOL "\xc2\xb0"
    /* NAME        ABBR               RATIO        STEP  PRE            POST VALUE */

struct unit_t temperature_table[] = {
    {"Kelvin",     "K",               UNIT,        UNIT, 0,              0,  0},
    {"Celsius",    DEGREE_SYMBOL "C", UNIT,        UNIT, -272.15 * UNIT, 0,  0},
    {"Fahrenheit", DEGREE_SYMBOL "F", UNIT * 1.8f, UNIT, -272.15 * UNIT, 32, 0},
};
