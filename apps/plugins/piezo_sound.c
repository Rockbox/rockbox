/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"

enum plugin_status plugin_start(const void *param) {
    (void) param;

    size_t sz;
    uint8_t *buf = rb->plugin_get_audio_buffer(&sz);

    int fd = rb->open("/sound.raw", O_RDONLY);

    ssize_t actual = rb->read(fd, buf, sz);

#define HI 0x60f
#define LO 0x60e

    for(;;) {
        for(size_t idx = 0; idx < actual; idx++) {
            uint8_t sample = buf[idx];

            int period = 125; // 8000 Hz
            int stop = USEC_TIMER + period;

            while(USEC_TIMER < stop) {
                for(int i = 0; i < 255; i++) {
                    if(i < sample)
                        GPIOCMD = HI;
                    else
                        GPIOCMD = LO;
                }
            }
        }
        rb->splashf(HZ, "loop");
    }
}
