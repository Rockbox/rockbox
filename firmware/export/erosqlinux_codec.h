#ifndef __EROSQLINUX_CODEC__
#define __EROSQLINUX_CODEC__

#define AUDIOHW_CAPS (LINEOUT_CAP)

AUDIOHW_SETTING(VOLUME, "dB", 0,  1, -74,   6, -25)

//#define AUDIOHW_NEEDS_INITIAL_UNMUTE

void audiohw_mute(int mute);
void erosq_set_output(int ps);
int erosq_get_outputs(void);

#endif
