#ifndef _UNIT_H_
#define _UNIT_H_

#include <stdint.h>
#define UNIT 100000000L
struct unit_t {
    const char* name;
    const char* abbr;
    uint64_t ratio; /* relative to first unit in array */
    uint64_t step;
    int64_t offset;
    int64_t value;
};

#endif
