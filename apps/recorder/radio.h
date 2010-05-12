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

#ifndef FMRADIO_H
#include "fmradio.h"
#endif
#include "screen_access.h"

#if CONFIG_TUNER
void radio_load_presets(char *filename);
void radio_init(void) INIT_ATTR;
int radio_screen(void);
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

/* skin functions */
void fms_data_load(enum screen_type screen, const char *buf, bool isfile);
void fms_skin_init(void);

/* callbacks for the radio settings */
void set_radio_region(int region);
void toggle_mono_mode(bool mono);

#define MAX_FMPRESET_LEN 27

struct fmstation
{
    int frequency; /* In Hz */
    char name[MAX_FMPRESET_LEN+1];
};

#endif /* CONFIG_TUNER */

#endif /* RADIO_H */
