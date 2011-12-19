/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Linus Nielsen Feltzing
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef RADIO_H
#define RADIO_H

#include "config.h"
#ifndef FMRADIO_H
#include "fmradio.h"
#endif
#include "screen_access.h"
#include "bmp.h"

enum {
    RADIO_SCAN_MODE = 0,
    RADIO_PRESET_MODE,
};

#if CONFIG_TUNER
void radio_load_presets(char *filename);
void radio_save_presets(void);
void radio_init(void) INIT_ATTR;
void radio_screen(void);
void radio_start(void);
void radio_pause(void);
void radio_stop(void);
bool radio_hardware_present(void);
bool in_radio_screen(void);

bool radio_scan_mode(void); /* true for scan mode, false for preset mode */
bool radio_is_stereo(void);
int radio_current_frequency(void);
int radio_current_preset(void);
int radio_preset_count(void);
const struct fmstation *radio_get_preset(int preset);

/* callbacks for the radio settings */
void set_radio_region(int region);
void toggle_mono_mode(bool mono);

#define MAX_FMPRESET_LEN 27

struct fmstation
{
    int frequency; /* In Hz */
    char name[MAX_FMPRESET_LEN+1];
};
const char* radio_get_preset_name(int preset);
#if 0 /* disabled in draw_progressbar() */
void presets_draw_markers(struct screen *screen, int x, int y, int w, int h);
#endif

#ifdef HAVE_ALBUMART
void radioart_init(bool entering_screen);
int radio_get_art_hid(struct dim *requested_dim);
#endif

void next_station(int direction);


enum fms_exiting {
    FMS_EXIT,
    FMS_ENTER
};

/* only radio.c should be using these! */
int fms_do_button_loop(bool update_screen);
void fms_fix_displays(enum fms_exiting toggle_state);

#endif /* CONFIG_TUNER */

#endif /* RADIO_H */
