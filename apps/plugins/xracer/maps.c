#include "xracer.h"
#include "maps.h"

struct road_section loop_map[9] = {
    { 0, 300, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS-1) },
    { 0, 200, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS-1) },
    { 0, 600, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS-1) },
    { 0, 200, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS-1) },
    { 0, 300, 0, 0 }
};
