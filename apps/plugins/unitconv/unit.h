#ifndef _UNIT_H_
#define _UNIT_H_
#include <stdint.h>
#include <stdbool.h>
#define UNIT 100000000LL

struct unit_t {
    const char* name;
    const char* abbr;
    int64_t ratio; /* relative to first unit in array */
    int64_t step;
    int64_t value;
    void (*callback)(struct unit_t*, int from, int to);
    bool negative;
};

#endif
