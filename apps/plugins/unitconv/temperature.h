#ifndef _TEMPERATURE_H_
#define _TEMPERATURE_H_
#include "unit.h"
#define DEGREE_SYMBOL "\xc2\xb0"

void temp_conv(struct unit_t *units, int from, int to);

struct unit_t temperature_table[] = {
    {"Kelvin",     "K",                UNIT,              0,              UNIT, 0, false},
    {"Celsius",    DEGREE_SYMBOL "C",  UNIT,              UNIT * 273.15,  UNIT, 0, true},
    {"Fahrenheit", DEGREE_SYMBOL "F",  UNIT * (5.0/9.0),  UNIT * 255.372, UNIT, 0, true},
    {"Rankine",    DEGREE_SYMBOL "R",  UNIT * (5.0/9.0),  0,              UNIT, 0, false},
    {"Delisle",    DEGREE_SYMBOL "De", UNIT * (-2.0/3.0), UNIT * 373.15,  UNIT, 0, true},
    {"Newton",     DEGREE_SYMBOL "N",  UNIT * (100/33.0), UNIT * 273.15,  UNIT, 0, true},
    {"Réaumur",    DEGREE_SYMBOL "Ré", UNIT * 1.25,       UNIT * 273.15,  UNIT, 0, true},
};
#endif
