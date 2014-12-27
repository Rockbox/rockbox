#include "unit.h"
struct unit_t mass_table[] = {
    {"Grams",      "g.",     UNIT,                  UNIT,      0, 0},
    {"Kilograms",  "kg.",    UNIT * 1000,           UNIT/10,   0, 0},
    {"Pounds",     "lbs.",   UNIT * 453.593,        UNIT/16,   0, 0},
    {"Short tons", "tons",   UNIT * 453.593 * 2000, UNIT/2000, 0, 0},
    {"Long tons",  "tonnes", UNIT * 1000 * 1000,    UNIT/2240, 0, 0},
};
