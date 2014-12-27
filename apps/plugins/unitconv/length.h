#include "unit.h"
struct unit_t length_table[] = {
    {"Millimeters",    "mm.", UNIT,           UNIT, 0, NULL, false},
    {"Centimeters",    "cm.", UNIT * 100,     UNIT, 0, NULL, false},
    {"Meters",         "m.",  UNIT * 10000,   UNIT, 0, NULL, false},
    {"Kilometers",     "km.", UNIT * 1000000, UNIT, 0, NULL, false},
    {"Feet",           "ft.", UNIT * 304.8,   UNIT, 0, NULL, false},
    {"Yards",          "yd.", UNIT * 914.4,   UNIT, 0, NULL, false},
    {"Miles",          "mi.", UNIT * 1609344, UNIT, 0, NULL, false},
    {"Nautical Miles", "nm.", UNIT * 1852000, UNIT, 0, NULL, false},
};
