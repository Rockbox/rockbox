#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "xracer.h"

/* some basic types */

struct point_3d {
    int x;
    int y;
    int z;
};

struct camera_t {
    struct point_3d pos;
    int depth;
};

struct point_2d {
    int x;
    int y;
    int w;
};

void render(struct camera_t *camera, struct road_segment *road, unsigned int road_length);

#endif
