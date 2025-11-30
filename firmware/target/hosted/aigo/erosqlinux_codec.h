#ifndef __EROSQLINUX_CODEC__
#define __EROSQLINUX_CODEC__

#define AUDIOHW_CAPS (LINEOUT_CAP)

/* a small DC offset prevents play/pause clicking due to the DAC auto-muting */
#define PCM_DC_OFFSET_VALUE -1

/*
 * Note: Maximum volume is set one step below unity in order to
 *       avoid overflowing pcm samples due to our DC Offset.
 *
 *       The DAC's output is hot enough this should not be an issue.
 */
AUDIOHW_SETTING(VOLUME, "dB", 0,  2, -74, -2, -40)

//#define AUDIOHW_NEEDS_INITIAL_UNMUTE

void audiohw_mute(int mute);
void erosq_set_output(int ps);
int erosq_get_outputs(void);

#endif
