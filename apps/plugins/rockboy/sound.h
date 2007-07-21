#ifndef __SOUND_H__
#define __SOUND_H__

#include "defs.h"

struct sndchan
{
    /* S1, S2, S3, S4 */
    int on, len, skip, cont;
    unsigned int pos;

    /* S1, S2, S4 */
    int enlen, envol, endir, enlenreload;
    
    /* S1, S2 */
    const byte *wave;
    
    /* S1 only */
    int swlen, swlenreload, swsteps, swstep, swdir;

    /* S3 only */
    int outputlevel;

    /* S4 only */
    int shiftskip, shiftpos, shiftright, shiftleft;
    int nsteps, clock;

};

struct snd
{
    int level1, level2, balance;
    bool gbDigitalSound;
    int rate;
    int quality;
    struct sndchan ch[4];
};

extern struct snd snd;

#if defined(ICODE_ATTR) && defined(CPU_ARM)
#undef ICODE_ATTR
#define ICODE_ATTR
#endif

byte sound_read(byte r) ICODE_ATTR;
void sound_write(byte r, byte b) ICODE_ATTR;
void sound_dirty(void);
void sound_reset(void);
void sound_mix(void) ICODE_ATTR;

#endif
