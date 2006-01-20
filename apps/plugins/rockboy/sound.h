

#ifndef __SOUND_H__
#define __SOUND_H__

#include "defs.h"

struct sndchan
{
	int on;
	unsigned pos;
	int cnt, encnt, swcnt;
	int len, enlen, swlen;
	int swfreq;
	int freq;
	int envol, endir;
};


struct snd
{
	int rate;
	struct sndchan ch[4];
	byte wave[16];
};


extern struct snd snd;

#if defined(ICODE_ATTR) && defined(CPU_ARM)
#undef ICODE_ATTR
#define ICODE_ATTR
#endif

byte sound_read(byte r) ICODE_ATTR;
void sound_write(byte r, byte b)ICODE_ATTR;
void sound_dirty(void)ICODE_ATTR;
void sound_off(void);
void sound_reset(void);
void sound_mix(void)ICODE_ATTR;
void s1_init(void);
void s2_init(void);
void s3_init(void);
void s4_init(void);

#endif
	
