#ifndef _GENERATOR_H_
#define _GENERATOR_H_

#include <stdbool.h>
#include "xracer.h"

void generate_random_road(struct road_segment *road, unsigned int road_length, bool hills, bool curves);

#endif
