/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __ALSA_CONTROLS_RB_H__
#define __ALSA_CONTROLS_RB_H__

#include <config.h>
#include <stddef.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>
#include <alloca.h>

/* open alsa control interface and list all controls, keep control open */
void alsa_controls_init(const char *name);
/* close alsa controls */
void alsa_controls_close(void);

/* NOTE: all the following functions panic on error. This behaviour could be changed with the
 * functions returning proper values but that would make errors happen silently */

/* check wether a control exists */
bool alsa_has_control(const char *name);
/* find a control element ID by name, return -1 of not found or index into array */
int alsa_controls_find(const char *name);
/* find a control element enum index by name, return -1 if not found */
int alsa_controls_find_enum(const char *name, const char *enum_name);
/* set a control, potentially supports several values */
void alsa_controls_set(const char *name, snd_ctl_elem_type_t type,
    unsigned nr_values, long *val);
/* get control information: number of values */
void alsa_controls_get_info(const char *name, int *out_count);
/* helper function: set a control with a single boolean value */
void alsa_controls_set_bool(const char *name, bool val);
/* helper function: set a control with a single enum value */
void alsa_controls_set_enum(const char *name, const char *enum_name);
/* helper function: set a control with one or more integers */
void alsa_controls_set_ints(const char *name, int count, long *val);
/* get a control value, potentially supports several values */
void alsa_controls_get(const char *name, snd_ctl_elem_type_t type,
    unsigned nr_values, long *val);
/* helper function: set a control with a single boolean value */
bool alsa_controls_get_bool(const char *name);

#endif /* __ALSA_CONTROLS_RB_H__ */
