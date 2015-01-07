#include "xracer.h"
#include "maps.h"

struct road_section loop_map[10] = {
    { 0, 300, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS) },
    { 0, 200, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS) },
    { 0, 600, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS) },
    { 0, 200, 0, 0 },
    { 0, 100, 0, 1 << (FRACBITS) },
    { 0, 300, 0, 0 },
    { 0, DRAW_DIST, 0, 0}
};
