#include "plugin.h"

#include "xracer.h"
#include "generator.h"

/* add_road(): adds a section of road segments between s and s+len,
   each with the properties specified */

void add_road(struct road_segment *road, int s, int len, int curve, int hill)
{
    static int last_y=0;
    for(int i = s; i < s+len; ++i)
    {
        struct road_segment tmp;
        tmp.idx  = i;
        tmp.p1_z = i * SEGMENT_LENGTH;
        tmp.p2_z = (i + 1) * SEGMENT_LENGTH;

        /* change the lines below for hills */
        tmp.p1_y = last_y;
        tmp.p2_y = last_y+hill;
        last_y += hill;

        /* change this line for curves */
        tmp.curve = curve;

        tmp.color = (int)((float)i/RUMBLE_LENGTH)%2?ROAD_COLOR1:ROAD_COLOR2;
        tmp.border_color = (int)((float)i/RUMBLE_LENGTH)%2?BORDER_COLOR1:BORDER_COLOR2;
        tmp.lanes = (int)((float)i/RUMBLE_LENGTH)%2?LANES:1;
        tmp.lane_color = LANE_COLOR;
        tmp.grass_color = (int)((float)i/RUMBLE_LENGTH)%2?GRASS_COLOR1:GRASS_COLOR2;
        tmp.looped = false;

        road[i] = tmp;
    }
}

/* enter_uphill(): adds a road section with slopes 0,1...slope-1 */
/* assumes slope is positive, for negative slopes, use *_downhill */
void enter_uphill(struct road_segment *road, int s, int slope, int curve)
{
    for(int i=0; i<slope; ++i)
    {
        add_road(road, s + i, 1, curve, i);
    }
}

/* exit_hill(): opposite of enter_hill, adds a road section with slopes slope-1...0 */
/* assumes slope is positive, for negative slopes, use *_downhill */
void exit_uphill(struct road_segment *road, int s, int slope, int curve)
{
    int n = slope;
    for(int i = 0; i<n; ++i, --slope)
    {
        add_road(road, s + i, 1, curve, slope);
    }
}

/* assumes slope is positive, for negative slopes, use *_downhill */
void add_uphill(struct road_segment *road, int s, int slope, int hold, int curve)
{
    enter_uphill(road, s, slope, curve);
    add_road(road, s+slope, hold, curve, slope);
    exit_uphill(road, s+slope+hold, slope, curve);
}

/* enter_downhill: adds a road section with slopes 0,-1,...slope+1 */
/* assumes slope is negative, for positive sloeps use *_uphill */
void enter_downhill(struct road_segment *road, int s, int slope, int curve)
{
    int n = 0;
    for(int i=0; i<-slope;++i, --n)
    {
        add_road(road, s + i, 1, curve, n);
        LOGF("enter_downhill: adding slope %d at idx %d", n, s + i);
    }
}

/* adds a road section with slopes slope, slope+1,...,-1 */
void exit_downhill(struct road_segment *road, int s, int slope, int curve)
{
    int n = slope;
    for(int i=0; i<-slope; ++i, ++n)
    {
        add_road(road, s + i, 1, curve, n);
    }
}

/* generate_random_road(): generates a random track road_length segments long */

void generate_random_road(struct road_segment *road, unsigned int road_length, bool hills, bool curves)
{
    rb->srand(*rb->current_tick);
    int len;
    for(unsigned int i = 0; i < road_length; i += len)
    {
        /* get an EVEN number */
        len = (rb->rand()% 20 + 10) * 2;
        if(i + len >= road_length)
            len = road_length - i;

        int r = rb->rand() % 12;

        if(r<=1)
        {
            enter_uphill(road, i, 5, 0);
            add_road(road, i+5, len-10, 0, ( hills ? 5 : 0));
            exit_uphill(road, i+len-5, 5, 0);
        }
        else if(r<=3)
        {
            enter_downhill(road, i, -5, 0);
            add_road(road, i+5, len-10, 0, (hills ? -5 : 0));
            exit_downhill(road, i+len-5, -5, 0);
        }
        else if(r<=7)
            add_road(road, i, len, 0, 0);
        else if(r<=9)
            add_road(road, i, len, (curves ? -1 : 0), 0);
        else if(r<12)
            add_road(road, i, len, (curves ? 1 : 0), 0);
    }
}
