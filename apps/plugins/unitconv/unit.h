#ifndef _UNIT_H_
#define _UNIT_H_
#include <stdint.h>
#include <stdbool.h>
#define UNIT 1.0f

typedef double data_t;

/* unit conversion works as follows */
/* RATIO and OFFSET are used to convert the unit into terms of units[0] */
/* value[0] = RATIO[n] * value[n] + OFFSET[n]; */
/* this can then be converted to another unit with */
/* units[n] = (value[0] - OFFSET[n])/RATIO[n] */

struct unit_t {
    const char* name;
    const char* abbr;
    data_t ratio; /* relative to first unit in array */
    data_t offset;
    data_t step;
    data_t value;
    bool negative;
};

#endif
