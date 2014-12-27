#include <stdint.h>
#define UNIT 1000000000L
struct unit_t {
    const char* name;
    const char* abbr;
    uint64_t ratio; /* relative to first unit in array */
    uint64_t step;

    /* unused, but lets the code have a place to store calculated values */
    uint64_t value;
};
