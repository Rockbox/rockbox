#include "unit.h"
struct unit_t length_table[] = {
    {"Millimeters",    "mm.", UNIT,           UNIT, 0, 0, 0},
    {"Centimeters",    "cm.", UNIT * 100,     UNIT, 0, 0, 0},
    {"Meters",         "m.",  UNIT * 10000,   UNIT, 0, 0, 0},
    {"Kilometers",     "km.", UNIT * 1000000, UNIT, 0, 0, 0},
    {"Feet",           "ft.", UNIT * 304.8,   UNIT, 0, 0, 0},
    {"Yards",          "yd.", UNIT * 914.4,   UNIT, 0, 0, 0},
    {"Miles",          "mi.", UNIT * 1609344, UNIT, 0, 0, 0},
    {"Nautical Miles", "nm.", UNIT * 1852000, UNIT, 0, 0, 0},
};
