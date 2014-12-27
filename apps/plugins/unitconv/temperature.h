#include "unit.h"
#define DEGREE_SYMBOL "\xc2\xb0"
void temp_f_to_c(struct unit_t*, int, int);
struct unit_t temperature_table[] = {
    {"Fahrenheit", DEGREE_SYMBOL "F", UNIT,       UNIT, 0,          0},
    {"Celsius",    DEGREE_SYMBOL "C", UNIT * 1.8, UNIT, -32 * UNIT, 0},
};
