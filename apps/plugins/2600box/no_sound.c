

/* A set of sound stubs for those without sound */
/* $Id: no_sound.c,v 1.4 1996/11/24 16:55:40 ahornby Exp $ */
#include "types.h"


void
sound_init (void)
{
}

void
sound_close (void)
{
}

void
sound_freq (int channel, BYTE freq)
{
}

void
sound_volume (int channel, BYTE vol)
{
}

void
sound_waveform (int channel, BYTE value)
{
}

void
sound_flush (void)
{
}

void
sound_update (void)
{
}
