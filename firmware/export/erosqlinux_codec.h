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

#define EROSQ_OUTPUT_BLUETOOTH 4

void audiohw_mute(int mute);
void erosq_set_output(int ps);
int erosq_get_outputs(void);
/* BD_ADDR as AA:BB:CC:DD:EE:FF before erosq_set_bluetooth_route(1). */
void erosq_set_bluetooth_peer(const char *bdaddr);
void erosq_set_bluetooth_route(int on);
void erosq_apply_bluetooth_route(void);
bool erosq_bluetooth_route_applied(void);
bool erosq_is_bluetooth_route_active(void);
void erosq_notify_pcm_running(void);

#endif
