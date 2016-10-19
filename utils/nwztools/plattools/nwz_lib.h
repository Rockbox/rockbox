/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef _NWZLIB_H_
#define _NWZLIB_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/input.h>
#include <fcntl.h>

#include "nwz_keys.h"

/* run a program and exit with nonzero status in case of error
 * argument list must be NULL terminated */
void nwz_run(const char *file, const char *args[], bool wait);

/* invoke /usr/bin/lcdmsg to display a message using the small font, optionally
 * clearing the screen before */
void nwz_lcdmsg(bool clear, int x, int y, const char *msg);
void nwz_lcdmsgf(bool clear, int x, int y, const char *format, ...);

/* open icx_key input device and return file descriptor */
int nwz_key_open(void);
void nwz_key_close(int fd);
/* return HOLD status: 0 or 1, or -1 on error */
int nwz_key_get_hold_status(int fd);
/* wait for an input event (and return 1), or a timeout (return 0), or error (-1)
 * set the timeout to -1 to block */
int nwz_key_wait_event(int fd, long tmo_us);
/* read an event from the device (may block unless you waited for an event before),
 * return 1 on success, <0 on error */
int nwz_key_read_event(int fd, struct input_event *evt);
/* return keycode from event */
int nwz_key_event_get_keycode(struct input_event *evt);
/* return press/released status from event */
bool nwz_key_event_is_press(struct input_event *evt);
/* return HOLD status from event */
bool nwz_key_event_get_hold_status(struct input_event *evt);
/* get keycode name */
const char *nwz_key_get_name(int keycode);

#endif /* _NWZLIB_H_ */
