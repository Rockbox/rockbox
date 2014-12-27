#ifndef _LENGTH_H_
#define _LENGTH_H_

#include "unit.h"
struct unit_t length_table[] = {
    {"Millimeters",        "mm.",         UNIT,                   0, UNIT, 0, false},
    {"Centimeters",        "cm.",         UNIT * 100,             0, UNIT, 0, false},
    {"Meters",             "m.",          UNIT * 1000,            0, UNIT, 0, false},
    {"Kilometers",         "km.",         UNIT * 1000000,         0, UNIT, 0, false},
    {"Feet",               "ft.",         UNIT * 304.8,           0, UNIT, 0, false},
    {"Yards",              "yd.",         UNIT * 914.4,           0, UNIT, 0, false},
    {"Miles",              "mi.",         UNIT * 1609344,         0, UNIT, 0, false},
    {"Nautical miles",     "nm.",         UNIT * 1852000,         0, UNIT, 0, false},
    {"Fathoms",            "fath.",       UNIT * 304.8 * 6,       0, UNIT, 0, false},
    {"Astronomical units", "AU",          UNIT * 149597870700000, 0, UNIT, 0, false},
    {"Standard league",    "st. leag.",   UNIT * 304.8 * 15840,   0, UNIT, 0, false},
    {"Nautical league",    "naut. leag.", UNIT * 304.8 * 18240,   0, UNIT, 0, false},
};

#endif
