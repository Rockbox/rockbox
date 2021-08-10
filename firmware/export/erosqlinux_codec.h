#ifndef __EROSQLINUX_CODEC__
#define __EROSQLINUX_CODEC__

#define AUDIOHW_CAPS (LINEOUT_CAP)

#define PCM_DC_OFFSET_VALUE -1

AUDIOHW_SETTING(VOLUME, "dB", 0,  2, -74, 0, -40)

//#define AUDIOHW_NEEDS_INITIAL_UNMUTE

void audiohw_mute(int mute);
void erosq_set_output(int ps);
int erosq_get_outputs(void);

#endif
