#ifndef __ROCKER_CODEC__
#define __ROCKER_CODEC__

#define AUDIOHW_CAPS 0
AUDIOHW_SETTING(VOLUME, "dB", 1, 5, -115*10,  0, -30*10)
#endif

//#define AUDIOHW_MUTE_ON_STOP
//#define AUDIOHW_NEEDS_INITIAL_UNMUTE

/* Note:  Due to Kernel bug, we can't use MUTE_ON_PAUSE with backlight fading */

void audiohw_mute(int mute);
