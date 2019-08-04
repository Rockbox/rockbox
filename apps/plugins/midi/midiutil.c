/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
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
#include "plugin.h"
#include "midiutil.h"

int chVol[16] IBSS_ATTR;       /* Channel volume                */
int chPan[16] IBSS_ATTR;       /* Channel panning               */
int chPat[16] IBSS_ATTR;                  /* Channel patch                 */
int chPW[16] IBSS_ATTR;                   /* Channel pitch wheel, MSB only */
int chPBDepth[16] IBSS_ATTR;              /* Channel pitch bend depth */
int chPBNoteOffset[16] IBSS_ATTR;       /* Pre-computed whole semitone offset */
int chPBFractBend[16] IBSS_ATTR;        /* Fractional bend applied to delta */
unsigned char chLastCtrlMSB[16]; /* MIDI regs, used for Controller 6. */
unsigned char chLastCtrlLSB[16]; /* The non-registered ones are ignored */

struct GPatch * gusload(char *);
struct GPatch * patchSet[128];
struct GPatch * drumSet[128];

struct SynthObject voices[MAX_VOICES] IBSS_ATTR;

static void *alloc(int size)
{
    static char *offset[2] = {NULL};
    static size_t totalSize[2] = {0};
    char *ret;
    int n;

    int remainder = size % 4;

    size = size + 4-remainder;

    if (offset[0] == NULL)
    {
        offset[0] = rb->plugin_get_buffer(&totalSize[0]);
    }

    if (offset[1] == NULL)
    {
        offset[1] = rb->plugin_get_audio_buffer(&totalSize[1]);
    }

    n = (totalSize[0] > totalSize[1]) ? 1 : 0;

    if (size + 4 > (int)totalSize[n])
        n ^= 1;
    if (size + 4 > (int)totalSize[n])
    {
        midi_debug("Out of Memory");
        return NULL;
    }

    ret = offset[n] + 4;
    *((unsigned int *)offset[n]) = size;

    offset[n] += size + 4;
    totalSize[n] -= size + 4;
    return ret;
}

/* Rick's code */
/*
void *alloc(int size)
{
    static char *offset = NULL;
    static ssize_t totalSize = 0;
    char *ret;


    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer((size_t *)&totalSize);
    }

    if (size + 4 > totalSize)
    {
        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;
}
*/

#define malloc(n) my_malloc(n)
void * my_malloc(int size)
{
    return alloc(size);
}

unsigned char readChar(int file)
{
    char buf[2];
    rb->read(file, &buf, 1);
    return buf[0];
}

void * readData(int file, int len)
{
    void * dat = malloc(len);
    if (dat)
        rb->read(file, dat, len);
    return dat;
}

int eof(int fd)
{
    int curPos = rb->lseek(fd, 0, SEEK_CUR);

    int size = rb->lseek(fd, 0, SEEK_END);

    rb->lseek(fd, curPos, SEEK_SET);
    return size+1 == rb->lseek(fd, 0, SEEK_CUR);
}

/* Here is a hacked up printf command to get the output from the game. */
int midi_debug(const char *fmt, ...)
{
    static int p_xtpt = 0;
    char p_buf[50];
    va_list ap;

    va_start(ap, fmt);
    rb->vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
    va_end(ap);

    int i=0;

    /* Device LCDs display newlines funny. */
    for(i=0; p_buf[i]!=0; i++)
        if(p_buf[i] == '\n')
            p_buf[i] = ' ';

    rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
    rb->lcd_update();

    p_xtpt+=8;
    if(p_xtpt>LCD_HEIGHT-8)
    {
        p_xtpt=0;
        rb->lcd_clear_display();
    }
    return 1;
}

