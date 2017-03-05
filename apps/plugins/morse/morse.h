/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Franklin Wei
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

#include "tlsf.h"

struct morse_output_api {
    void (*init)(void *userdata, int wpm); /* only called if requested */
    void (*dit)(void *userdata, int wpm);
    void (*dah)(void *userdata, int wpm);
    void (*interword_silence)(void *userdata, int wpm);
    void (*intraword_silence)(void *userdata, int wpm);
    void (*intrachar_silence)(void *userdata, int wpm);
    void (*fini)(void *userdata); /* called by morse_finalize() */
};

struct wav_params {
    const char *fname;

    int out_fd;

    int16_t *dit_buf, *dah_buf;
    size_t dit_len, dah_len;

    int16_t *silence_buf; /* 7 dits long */
    size_t silence_len;
};

/* returns false if interrupted by user action */
bool morse_play(const char *str, int wpm, const struct morse_output_api *api, bool call_init, void *userdata);
void morse_finalize(const struct morse_output_api *api, void *userdata);

/* APIs */
extern const struct morse_output_api rbaudio_api, wav_api;
