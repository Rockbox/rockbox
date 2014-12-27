#ifndef _MASS_H_
#define _HASS_H_

#include "unit.h"

struct unit_t mass_table[] = {
    {"Grams",             "g.",     UNIT,                    0, UNIT,    0, false},
    {"Kilograms",         "kg.",    UNIT * 1000,             0, UNIT/10, 0, false},
    {"Pounds",            "lbs.",   UNIT * 453.59237,        0, UNIT/16, 0, false},
    {"Short tons",        "tons",   UNIT * 453.59237 * 2000, 0, UNIT,    0, false},
    {"Long tons",         "tonnes", UNIT * 1000 * 1000,      0, UNIT,    0, false},
    {"Atomic mass units", "amu",    UNIT * 1.660538921e-27,  0, UNIT,    0, false},
};

#endif
