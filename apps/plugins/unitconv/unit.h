#ifndef _UNIT_H_
#define _UNIT_H_

#include <stdint.h>
#define UNIT 100000000L
struct unit_t {
    const char* name;
    const char* abbr;
    int64_t ratio; /* relative to first unit in array */
    int64_t step;
    int64_t offset_pre; /* add before */
    int64_t offset_post; /* add after */
    int64_t value;
};

#endif
