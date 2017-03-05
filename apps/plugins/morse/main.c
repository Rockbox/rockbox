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

#include "plugin.h"

#include "morse.h"

const char *koch_method = "KMRSUAPTLOWINJEF0YVG5Q9ZH38B427C1D6X";

const char *char_format(char *buf, size_t sz, int n, const char *unit)
{
    (void) unit;
    if(n == 2)
        return "K and M";
    else
    {
        rb->snprintf(buf, sz, "All the above and %c", koch_method[n - 1]);
        return buf;
    }
}

static void learn_koch(void)
{
    int wpm = 15;

    rb->set_int("Words per Minute", "", UNIT_INT, &wpm, NULL, 1, 10, 25, NULL);

    rb->srand(*rb->current_tick);

    int range = 2;
    rb->set_int("Characters", "", UNIT_INT, &range, NULL, 1, 2, 36, char_format);

    int n = 0;
    for(;;)
    {
        char str[2];
        str[0] = koch_method[rb->rand() % range];
        str[1] = '\0';
        if(!morse_play(str, wpm, &rbaudio_api, false, NULL))
            return;

        rbaudio_api.intraword_silence(NULL, wpm);

        if(!(n = (n+1)%5))
            rbaudio_api.interword_silence(NULL, wpm);
    }
}

static enum plugin_status play_file(const char *fname)
{
    int wpm = 15;

    int in_fd = rb->open(fname, O_RDONLY);
    if(in_fd < 0)
        return PLUGIN_ERROR;

    char new_file[MAX_PATH];
    rb->snprintf(new_file, sizeof(new_file), "%s.wav", fname);
    bool init = true;

    struct wav_params params;
    params.fname = new_file;

    while(1)
    {
        char buf[1024];
        int rc;
        if((rc = rb->read(in_fd, buf, sizeof(buf) - 1)) <= 0)
            break;

        buf[rc] = '\0';
        morse_play(buf, wpm, &wav_api, init, &params);
        init = false;
    }

    morse_finalize(&wav_api, &params);

    rb->close(in_fd);

    return PLUGIN_OK;
}

char *giant_buffer = NULL;
size_t giant_buffer_len = 0;

static void reset_tlsf(void)
{
    /* reset tlsf by nuking the signature */
    /* will make any already-allocated memory point to garbage */
    memset(giant_buffer, 0, 4);
    init_memory_pool(giant_buffer_len, giant_buffer);
}

enum plugin_status plugin_start(const void *param)
{
    giant_buffer = rb->plugin_get_buffer(&giant_buffer_len);
    reset_tlsf();

    if(!param)
        learn_koch();
    else
        play_file(param);
    return PLUGIN_OK;
}
