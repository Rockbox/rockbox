#include "unit.h"

struct unit_t mass_table[] = {
    {"Grams",      "g.",     UNIT,                  UNIT,      0, NULL, false},
    {"Kilograms",  "kg.",    UNIT * 1000,           UNIT/10,   0, NULL, false},
    {"Pounds",     "lbs.",   UNIT * 453.593,        UNIT/16,   0, NULL, false},
    {"Short tons", "tons",   UNIT * 453.593 * 2000, UNIT/2000, 0, NULL, false},
    {"Long tons",  "tonnes", UNIT * 1000 * 1000,    UNIT/2240, 0, NULL, false},
};
