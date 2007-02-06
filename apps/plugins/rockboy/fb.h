

#ifndef __FB_H__
#define __FB_H__


#include "defs.h"



struct fb
{
    fb_data *ptr;
    struct
    {
        int l, r;
    } cc[3];
    int enabled;
#if !defined(HAVE_LCD_COLOR)
    int mode;
#endif
};


extern struct fb fb;


#endif




