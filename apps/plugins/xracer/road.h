#ifndef _ROAD_H_
#define _ROAD_H_

struct road_segment {
    int idx;
    int p1_z;
    int p2_z;
    int p1_y;
    int p2_y;
    int curve;
    unsigned color;
    unsigned border_color;
    int lanes;
    unsigned lane_color;
    unsigned grass_color;
};

#endif
