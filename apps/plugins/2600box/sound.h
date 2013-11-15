#ifndef X2600_SOUND_H
#define X2600_SOUND_H

/* $Id: sound.h,v 1.4 1996/09/04 22:37:48 ahornby Exp $ */
#include "types.h"
void
sound_init(void);

void
sound_close(void);

void 
sound_freq(int channel, BYTE freq);

void
sound_volume(int channel, BYTE vol);

void
sound_waveform(int channel, BYTE value);

void
sound_update(void);

#endif
