/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Jörg Hohensohn
 *
 * This module collects the Talkbox and voice UI functions.
 * (Talkbox reads directory names from mp3 clips called thumbnails,
 *  the voice UI lets menus and screens "talk" from a voicefont in memory.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include "file.h"
#include "buffer.h"
#include "kernel.h"
#include "mp3_playback.h"
#include "mpeg.h"
#include "talk.h"
extern void bitswap(unsigned char *data, int length); /* no header for this */

/***************** Constants *****************/

#define VOICEFONT_FILENAME "/.rockbox/voicefont"


/***************** Data types *****************/

struct clip_entry /* one entry of the index table */
{
    int offset; /* offset from start of voicefont file */
    int size; /* size of the clip */
};

struct voicefont /* file format of our "voicefont" */
{
    int version; /* version of the voicefont */
    int headersize; /* size of the header, =offset to index */
    int id_max; /* number of clips contained */
    struct clip_entry index[]; /* followed by the index table */
    /* and finally the bitswapped mp3 clips, not visible here */
};


/***************** Globals *****************/

static unsigned char* p_thumbnail; /* buffer for thumbnail */
static long size_for_thumbnail; /* leftover buffer size for it */
static struct voicefont* p_voicefont; /* loaded voicefont */
static bool has_voicefont; /* a voicefont file is present */
static bool is_playing; /* we're currently playing */



/***************** Private implementation *****************/

static int load_voicefont(void)
{
    int fd;
    int size;

    p_voicefont = NULL; /* indicate no voicefont if we fail below */

    fd = open(VOICEFONT_FILENAME, O_RDONLY);
    if (fd < 0) /* failed to open */
    {
        p_voicefont = NULL; /* indicate no voicefont */
        has_voicefont = false; /* don't try again */
        return 0;
    }

    size = read(fd, mp3buf, mp3end - mp3buf);
    if (size > 1000
        && ((struct voicefont*)mp3buf)->headersize
           == offsetof(struct voicefont, index))
    {
        p_voicefont = (struct voicefont*)mp3buf;

        /* thumbnail buffer is the remaining space behind */
        p_thumbnail = mp3buf + size;
        p_thumbnail += (int)p_thumbnail % 2; /* 16-bit align */
        size_for_thumbnail = mp3end - p_thumbnail;
        
        /* max. DMA size, fixme */
        if (size_for_thumbnail > 0xFFFF)
            size_for_thumbnail = 0xFFFF;
    }
    else
    {
        has_voicefont = false; /* don't try again */
    }
    close(fd);

    return size;
}


/* called in ISR context if mp3 data got consumed */
void mp3_callback(unsigned char** start, int* size)
{
    (void)start; /* unused parameter, avoid warning */
    *size = 0; /* end of data */
    is_playing = false;
    mp3_play_stop(); /* fixme: should be done by caller */
}

/***************** Public implementation *****************/

void talk_init(void)
{
    has_voicefont = true; /* unless we fail later, assume we have one */
    talk_buffer_steal();
}


/* somebody else claims the mp3 buffer, e.g. for regular play/record */
int talk_buffer_steal(void)
{
    p_voicefont = NULL; /* indicate no voicefont (trashed) */
    p_thumbnail = mp3buf; /*  whole space for thumbnail */
    size_for_thumbnail = mp3end - mp3buf;
    /* max. DMA size, fixme */
    if (size_for_thumbnail > 0xFFFF)
        size_for_thumbnail = 0xFFFF;
    return 0;
}


/* play a voice ID from voicefont */
int talk_id(int id, bool block)
{
    int clipsize;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_voicefont == NULL && has_voicefont)
        load_voicefont(); /* reload needed */

    if (p_voicefont == NULL) /* still no voices? */
        return -1;

    if (id >= p_voicefont->id_max)
        return -1;

    clipsize = p_voicefont->index[id].size;
    if (clipsize == 0) /* clip not included in voicefont */
        return -1;
    
    is_playing = true;
    mp3_play_data(mp3buf + p_voicefont->index[id].offset, 
        clipsize, mp3_callback);
    mp3_play_pause(true); /* kickoff audio */

    while(block && is_playing)
        sleep(1);

    return 0;
}


/* play a thumbnail from file */
int talk_file(char* filename, bool block)
{
    int fd;
    int size;

    if (mpeg_status()) /* busy, buffer in use */
        return -1; 

    if (p_thumbnail == NULL || size_for_thumbnail <= 0)
        return -1;

    fd = open(filename, O_RDONLY);
    if (fd < 0) /* failed to open */
    {
        return 0;
    }

    size = read(fd, p_thumbnail, size_for_thumbnail);
    close(fd);

    /* ToDo: find audio, skip ID headers and trailers */

    if (size)
    {
        bitswap(p_thumbnail, size);
        is_playing = true;
        mp3_play_data(p_thumbnail, size, mp3_callback);
        mp3_play_pause(true); /* kickoff audio */

        while(block && is_playing)
            sleep(1);
    }

    return size;
}
