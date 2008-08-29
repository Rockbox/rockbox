#ifndef DUMMIES_H_INCLUDED
#define DUMMIES_H_INCLUDED

#include <stdio.h>

#include "settings.h"
#include "gwps.h"
#include "lang.h"
#include "powermgmt.h"
#include "font.h"
#include "playlist.h"

#include "defs.h"

extern struct font sysfont;
extern struct user_settings global_settings;
extern struct wps_state wps_state;
extern struct gui_wps gui_wps[NB_SCREENS];
extern struct wps_data wps_datas[NB_SCREENS];
extern struct cuesheet *curr_cue;
extern struct cuesheet *temp_cue;
extern struct system_status global_status;
extern struct gui_syncstatusbar statusbars;
extern struct playlist_info current_playlist;
extern int battery_percent;
extern struct mp3entry current_song, next_song;
extern int _audio_status;

charger_input_state_type charger_input_state;
#if CONFIG_CHARGING >= CHARGING_MONITOR
extern charge_state_type charge_state;
#endif

#if defined(CPU_PP) && defined(BOOTLOADER)
/* We don't enable interrupts in the iPod bootloader, so we need to fake
the current_tick variable */
#define current_tick (signed)(USEC_TIMER/10000)
#else
extern volatile long current_tick;
#endif

void dummies_init();

#endif /*DUMMIES_H_INCLUDED*/
