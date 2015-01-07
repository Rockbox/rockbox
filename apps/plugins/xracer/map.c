#include <stdint.h>

#include "generator.h"
#include "map.h"
#include "util.h"

unsigned load_map(struct road_segment *road, unsigned int max_road_length, struct road_section *map, unsigned int map_length)
{
    gen_reset();
    unsigned int back = 0;
    for(unsigned int i=0;i<map_length;++i)
    {
        switch(map[i].type)
        {
        case 0:
            /* constant segment */
            add_road(road, max_road_length, back, map[i].len, map[i].curve, map[i].slope);
            back += map[i].len;
            break;
        case 1:
            /* up-hill */
            add_uphill(road, max_road_length, back, map[i].slope, map[i].len, map[i].curve);
            back += map[i].len + 2*map[i].slope;
            break;
        case 2:
            add_downhill(road, max_road_length, back, map[i].slope, map[i].len, map[i].curve);
            back += map[i].len + 2*map[i].slope;
            break;
        default:
            warning("invalid type id");
            break;
        }
    }
    return back;
}
