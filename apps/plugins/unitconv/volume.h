#ifndef _VOLUME_H_
#define _VOLUME_H_

#include "unit.h"

#define CUBE_SYMBOL "\302\263"

struct unit_t volume_table[] = {
    {"Cubic millimeters", "mm" CUBE_SYMBOL, UNIT,                          0, UNIT, 0, false},
    {"Cubic centimeters", "cm" CUBE_SYMBOL, UNIT * 1000,                   0, UNIT, 0, false},
    /* 1 ml = 1 cm^3 */
    {"Milliliters",       "mL",             UNIT * 1000,                   0, UNIT, 0, false},
    {"Liters",            "L",              UNIT * 1000 * 1000,            0, UNIT, 0, false},
    {"Cubic inches",      "in" CUBE_SYMBOL, UNIT * 1000 * 16.387064,       0, UNIT, 0, false},
    {"U.S. gallons",      "gal.",           UNIT * 1000 * 16.387064 * 231, 0, UNIT, 0, false}
};

#endif
