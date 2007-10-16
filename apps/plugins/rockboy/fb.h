

#ifndef __FB_H__
#define __FB_H__


#include "defs.h"



struct fb
{
#ifdef HAVE_LCD_COLOR
    struct
    {
        int l, r;
    } cc[3];
#else
    int mode;
#endif
    int enabled;
};


extern struct fb fb;


#endif




