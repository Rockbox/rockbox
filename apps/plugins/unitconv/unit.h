#ifndef _UNIT_H_
#define _UNIT_H_
#include <stdint.h>
#include <stdbool.h>
#define UNIT 100000LL

typedef __int128 data_t;

struct unit_t {
    const char* name;
    const char* abbr;
    data_t ratio; /* relative to first unit in array */
    data_t step;
    data_t value;
    void (*callback)(struct unit_t*, int from, int to);
    bool negative;
};

#endif
