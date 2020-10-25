#ifndef __ROCKER_CODEC__
#define __ROCKER_CODEC__

#define AUDIOHW_CAPS 0
AUDIOHW_SETTING(VOLUME, "dB", 1, 5, -102*10,  0, -30*10)
#endif

#define AUDIOHW_MUTE_ON_PAUSE

void audiohw_mute(int mute);
