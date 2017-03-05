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

#include "fixedpoint.h"

static void write_le32(int fd, uint32_t n)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
    n = swap32(n);
#endif
    rb->write(fd, &n, sizeof(n));
}

static void write_le16(int fd, uint16_t n)
{
#ifndef ROCKBOX_LITTLE_ENDIAN
    n = swap16(n);
#endif
    rb->write(fd, &n, sizeof(n));
}

#define SIGN(x) ((x)<0?-1:1)

static void sine_wave(int16_t *buf, size_t buf_len, int freq, int samplerate)
{
    for(unsigned int i = 0; i < buf_len; ++i)
    {
        buf[i] = fp14_sin(360 * freq * i / samplerate);
    }

    /* avoid clicks */
    int last_sign = SIGN(buf[buf_len - 1]);

    for(unsigned int i = buf_len - 2; i >= 0; --i)
    {
        /* fill with zeroes until we find a zero */
        if(SIGN(buf[i]) != last_sign || buf[i] == 0)
            break;
        last_sign = SIGN(buf[i]);
        buf[i] = 0;
    }
}

/* WAV output API */
static void wav_init(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    const char *new_file = params->fname;

    params->out_fd = rb->open(new_file, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if(params->out_fd < 0)
        return;

#define WAV_FREQ 11025
#define BIT_DEPTH 16
    /* write RIFF WAVE header */

    const char header[] = {
        'R', 'I', 'F', 'F',
        0x0, 0x0, 0x0, 0x0, /* file size */
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
    };

    rb->write(params->out_fd, header, sizeof(header));
    write_le32(params->out_fd, 16); /* length of format */
    write_le16(params->out_fd, 1); /* 1 = PCM */
    write_le16(params->out_fd, 1); /* 1 channel (mono) */
    write_le32(params->out_fd, WAV_FREQ); /* sample rate */
    write_le32(params->out_fd, BIT_DEPTH * WAV_FREQ / 8); /* bytes/sec */
    write_le16(params->out_fd, BIT_DEPTH / 8); /* bytes/sample */
    write_le16(params->out_fd, BIT_DEPTH);

    rb->write(params->out_fd, "data", 4);
    write_le32(params->out_fd, 0); /* fill in later */

    /* generate dits and dahs */
    params->dit_len = (6 * WAV_FREQ / 5) / wpm;
    params->dah_len = 3 * params->dit_len;

    params->dit_buf = tlsf_malloc(sizeof(int16_t) * params->dit_len);
    params->dah_buf = tlsf_malloc(sizeof(int16_t) * params->dah_len);

    sine_wave(params->dit_buf, params->dit_len, 1000, WAV_FREQ);
    sine_wave(params->dah_buf, params->dah_len, 1000, WAV_FREQ);

    params->silence_len = 7 * params->dit_len;
    params->silence_buf = tlsf_malloc(sizeof(int16_t) * params->silence_len);
    rb->memset(params->silence_buf, 0, params->silence_len);
}

static void wav_dit(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    rb->write(params->out_fd, params->dit_buf, params->dit_len);
}

static void wav_dah(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    rb->write(params->out_fd, params->dah_buf, params->dah_len);
}

static void wav_interword_silence(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    rb->write(params->out_fd, params->silence_buf, params->silence_len);
}

static void wav_intraword_silence(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    rb->write(params->out_fd, params->silence_buf, 3 * params->silence_len / 7);
}

static void wav_intrachar_silence(void *userdata, int wpm)
{
    struct wav_params *params = userdata;
    rb->write(params->out_fd, params->silence_buf, params->silence_len / 7);
}

static void wav_finalize(void *userdata)
{
    struct wav_params *params = userdata;
    /* fill in fields */
    off_t len = rb->lseek(params->out_fd, 0, SEEK_CUR);
    rb->lseek(params->out_fd, 4, SEEK_SET);
    write_le32(params->out_fd, len);
    rb->lseek(params->out_fd, 40, SEEK_SET);
    write_le32(params->out_fd, len - 44);
    /* free buffers */
    tlsf_free(params->dit_buf);
    tlsf_free(params->dah_buf);

    rb->close(params->out_fd);
}

const struct morse_output_api wav_api = {
    wav_init,
    wav_dit,
    wav_dah,
    wav_interword_silence,
    wav_intraword_silence,
    wav_intrachar_silence,
    wav_finalize,
};
