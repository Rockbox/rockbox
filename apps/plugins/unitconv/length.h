#include "unit.h"
struct unit_t length_table[] = {
    {"Millimeters",    "mm.", UNIT,           0, UNIT, 0, false},
    {"Centimeters",    "cm.", UNIT * 100,     0, UNIT, 0, false},
    {"Meters",         "m.",  UNIT * 1000,   0, UNIT, 0, false},
    {"Kilometers",     "km.", UNIT * 1000000, 0, UNIT, 0, false},
    {"Feet",           "ft.", UNIT * 304.8,   0, UNIT, 0, false},
    {"Yards",          "yd.", UNIT * 914.4,   0, UNIT, 0, false},
    {"Miles",          "mi.", UNIT * 1609344, 0, UNIT, 0, false},
    {"Nautical Miles", "nm.", UNIT * 1852000, 0, UNIT, 0, false},
};
