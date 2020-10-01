#ifndef __XDUOOLINUX_CODEC__
#define __XDUOOLINUX_CODEC__

#define AUDIOHW_CAPS (FILTER_ROLL_OFF_CAP)
AUDIOHW_SETTING(VOLUME, "dB", 0, 1, -127,  0, -30)
AUDIOHW_SETTING(FILTER_ROLL_OFF, "", 0, 1, 0, 4, 0)
#endif

void audiohw_mute(int mute);
void audiohw_set_output(void);
