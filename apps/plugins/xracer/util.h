#include "xracer.h"

/* NASTY! */
#define ROUND(x) ((int)((float)x+.5))

/* finds the address of a segment corresponding to a z coordinate */
#define FIND_SEGMENT(z, r, l) (&r[(int)((float)z/SEGMENT_LENGTH)%l])
