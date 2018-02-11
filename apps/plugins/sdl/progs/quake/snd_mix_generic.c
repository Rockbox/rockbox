/* This file is included from snd_mix.c */

#include "SDL.h"

// allow standalone compilation
#ifndef QUAKE_SOUND
extern short *paintbuffer;
#define INLINE
#else
#define INLINE static inline
#endif

// C version
static inline short CLAMPADD(short a, short b)
{
    int val = (int)a + (int)b;
    if (val > 32767)
    {
        //rb->splashf(HZ, "saturate");
        return 32767;
    }
    else if (val < -32768)
    {
        //rb->splashf(HZ, "saturate");
        return -32768;
    }
    else
        return val;
}

// if not ARMv5
#ifndef __ARM_ARCH__5TEJ__
void SND_PaintChannelFrom8 (int true_lvol, int true_rvol, signed char *sfx, int count)
{
    //return;
    //LOGF("mix8\n");
        int     data;
        int             i;

        // we have 8-bit sound in sfx[], which we want to scale to
        // 16bit and take the volume into account
        for (i=0 ; i<count ; i++)
        {
            // We could use the QADD16 instruction on ARMv6+
            // or just 32-bit QADD with pre-shifted arguments
            data = sfx[i];
            paintbuffer[2*i+0] = CLAMPADD(paintbuffer[2*i+0], data * true_lvol); // need saturation
            paintbuffer[2*i+1] = CLAMPADD(paintbuffer[2*i+1], data * true_rvol);
        }
}
#endif

void SND_PaintChannelFrom16 (int true_lvol, int true_rvol, signed short *sfx, int count)
{
    //return;
    //LOGF("mix16\n");
        int data;
        int left, right;
        int     i;

        for (i=0 ; i<count ; i+=2)
        {
                data = sfx[i];
                left = ((int)data * true_lvol) >> 8;
                right = ((int)data * true_rvol) >> 8;
                paintbuffer[2*i+0] = CLAMPADD(paintbuffer[2*i+0], left); // need saturation
                paintbuffer[2*i+1] = CLAMPADD(paintbuffer[2*i+1], right);
        }
}
