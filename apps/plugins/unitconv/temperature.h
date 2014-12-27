#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_
#include "unit.h"
#define DEGREE_SYMBOL "\xc2\xb0"

void temp_conv(struct unit_t *units, int from, int to);

struct unit_t temperature_table[] = {
    {"Kelvin",     "K",                UNIT, UNIT, 0, temp_conv, false},
    {"Celsius",    DEGREE_SYMBOL "C",  UNIT, UNIT, 0, temp_conv, true},
    {"Fahrenheit", DEGREE_SYMBOL "F",  UNIT, UNIT, 0, temp_conv, true},
    {"Rankine",    DEGREE_SYMBOL "R",  UNIT, UNIT, 0, temp_conv, false},
    {"Delisle",    DEGREE_SYMBOL "De", UNIT, UNIT, 0, temp_conv, true},
    {"Newton",     DEGREE_SYMBOL "N",  UNIT, UNIT, 0, temp_conv, true},
    {"Réaumur",    DEGREE_SYMBOL "Ré", UNIT, UNIT, 0, temp_conv, true},
};
#endif
