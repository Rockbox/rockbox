

#ifndef __FB_H__
#define __FB_H__


#include "defs.h"



struct fb
{
#ifndef HAVE_LCD_COLOR
    int mode;
#endif
    int enabled;
};


extern struct fb fb;


#endif




