#include <stdbool.h>
#include <stdint.h>

#include "generator.h"
#include "map.h"
#include "util.h"

static bool add_section(struct road_segment *road, unsigned int max_road_length, unsigned int *back, struct road_section *map)
{
    bool ret = true;
    switch(map->type)
    {
    case 0:
        /* constant segment */
        add_road(road, max_road_length, *back, map->len, map->curve, map->slope);
        *back += map->len;
        break;
    case 1:
        /* up-hill */
        add_uphill(road, max_road_length, *back, map->slope, map->len, map->curve);
        *back += map->len + 2*map->slope;
        break;
    case 2:
        add_downhill(road, max_road_length, *back, map->slope, map->len, map->curve);
        *back += map->len + 2*map->slope;
        break;
    default:
        warning("invalid type id");
        ret = false;
        break;
    }
    return ret;
}

unsigned load_map(struct road_segment *road, unsigned int max_road_length, struct road_section *map, unsigned int map_length)
{
    gen_reset();
    unsigned int back = 0;
    for(unsigned int i=0;i<map_length;++i)
    {
        add_section(road, max_road_length, &back, &map[i]);
    }
    return back;
}

unsigned load_external_map(struct road_segment *road, unsigned int max_road_length, const char *filename)
{
    int fd = rb->open(filename, O_RDONLY, 0666);
    if(fd < 0)
    {
        warning("Map file could not be opened");
        return -1;
    }

    /* check the header */
    static const unsigned char cmp_header[8] = {'X', 'M', 'a', 'P',
                                                (MAP_VERSION & 0xFF00 >> 8),
                                                (MAP_VERSION & 0x00FF >> 0),
                                                0xFF, 0xFF };
    unsigned char header[8];
    int ret = rb->read(fd, header, sizeof(header));
    if(ret != sizeof(header) || rb->memcmp(header, cmp_header, sizeof(header)) != 0)
    {
        warning("Malformed map header");
        return -1;
    }

    /* read data */
    unsigned char data_buf[15];
    unsigned int back = 0;
    bool fail = false;

    gen_reset();

    do {

        ret = rb->read(fd, data_buf, sizeof(data_buf));
        if(ret != sizeof(data_buf))
        {
            break;
        }

        struct road_section section =
            {
                data_buf[0],
                READ_BE_UINT32(&data_buf[1]),
                READ_BE_UINT32(&data_buf[5]),
                READ_BE_UINT32(&data_buf[9])
            };

        bool status = add_section(road, max_road_length, &back, &section);

        if(!status)
        {
            warning("Unknown map section type");
            fail = true;
        }

    } while(ret == sizeof(data_buf));

    /* last number of bytes read was not zero, this indicates failure */
    if(ret)
    {
        warning("Data too long");
        fail = true;
    }
    if(fail)
        return -1;
    else
        return back;
}
