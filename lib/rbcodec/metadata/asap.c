/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dominik Wenger
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>


#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"
#include "debug.h"
#include "platform.h"
#define MAX_SONGS  32

static char readchar(int fd, char failchr)
{
    char ch;
    if (read(fd,&ch,sizeof(ch)) == sizeof(ch))
        return ch;
    return failchr;
}

static bool parse_dec(int *retval, const char *p, int minval, int maxval)
{
    int r = 0;
    do {
        char c = *p;
        if (c >= '0' && c <= '9')
            r = 10 * r + c - '0';
        else
            return false;
        if (r > maxval)
            return false;
    } while (*++p != '\0');
    if (r < minval)
        return false;
    *retval = r;
    return true;
}

static bool parse_text(char *retval, const char *p)
{
    int i;
    if (*p != '"')
        return false;
    p++;
    if (p[0] == '<' && p[1] == '?' && p[2] == '>' && p[3] == '"')
        return true;
    i = 0;
    while (*p != '"') {
        if (i >= 127)
            return false;
        if (*p == '\0')
            return false;
        retval[i++] = *p++;
    }
    retval[i] = '\0';
    return true;
}

static int ASAP_ParseDuration(const char *s)
{
    #define chk_digit(rtn) if (*s < '0' || *s > '9') {return (rtn);}
    #define set_digit(mult)   r += (mult) * (*s++ - '0')
    #define parse_digit(m) chk_digit(r) set_digit(m)

    int r = 0;
    chk_digit(-1);
    set_digit(1);
    if (*s >= '0' && *s <= '9')
        r = 10 * r + *s++ - '0';
    if (*s == ':') {
        s++;
        if (*s < '0' || *s > '5')
            return -1;
        r = 60 * r + (*s++ - '0') * 10;
        chk_digit(-1);
        set_digit(1);
    }

    r *= 1000;
    if (*s != '.')
        return r;
    s++;
    parse_digit(100);
    parse_digit(10);
    parse_digit(1);

    return r;
}

static bool read_asap_string(char* source, char** buf, char** buffer_end, char** dest)
{
    if(parse_text(*buf,source) == false)
        return false;

    /* set dest pointer */
    *dest = *buf;

    /* move buf ptr */
    *buf += strlen(*buf)+1;

    /* check size */
    if(*buf >= *buffer_end)
    {
        DEBUGF("Buffer full\n");
        return false;
    }
    return true;
}

static bool parse_sap_header(int fd, struct mp3entry* id3, int file_len)
{
    #define EOH_CHAR (0xFF)
    int module_index = 0;
    int sap_signature = -1;
    int duration_index = 0;
    unsigned char cur_char = 0;
    int i;

    /* set defaults */
    int numSongs = 1;
    int defSong = 0;
    int durations[MAX_SONGS];
    for (i = 0; i < MAX_SONGS; i++)
        durations[i] = -1;

    /* use id3v2 buffer for our strings */
    char* buffer = id3->id3v2buf;
    char* buffer_end = id3->id3v2buf + ID3V2_BUF_SIZE;

    /* parse file */
    while (1)
    {
        char line[256];
        char *p;

        if (module_index + 8 >= file_len)
            return false;
        cur_char = readchar(fd, EOH_CHAR);
        /* end of header */
        if (cur_char == EOH_CHAR)
            break;

        i = 0;
        while (cur_char != 0x0d)
        {
            line[i++] = cur_char;
            module_index++;
            if (module_index >= file_len || (unsigned)i >= sizeof(line) - 1)
                return false;
            cur_char = readchar(fd, 0x0d);
        }
        if (++module_index >= file_len )
            return false;
        cur_char = readchar(fd, EOH_CHAR);
        if ( cur_char != 0x0a)
            return false;

        line[i] = '\0';
        for (p = line; *p != '\0'; p++) {
            if (*p == ' ') {
                *p++ = '\0';
                break;
            }
        }
        static const char *tg_options[] = {"SAP", "AUTHOR", "NAME", "DATE",
                                            "SONGS", "DEFSONG", "TIME", NULL};
        /* parse tags */
        int tg_op = string_option(line, tg_options, false);
        if (tg_op == 0) /*SAP*/
            sap_signature = 1;
        if (sap_signature == -1)
            return false;

        if (tg_op == 1) /*AUTHOR*/
        {
            if(read_asap_string(p, &buffer, &buffer_end, &id3->artist) == false)
                return false;
        }
        else if(tg_op == 2) /*NAME*/
        {
            if(read_asap_string(p, &buffer, &buffer_end, &id3->title) == false)
                return false;
        }
        else if(tg_op == 3) /*DATE*/
        {
            if(read_asap_string(p, &buffer, &buffer_end, &id3->year_string) == false)
                return false;
        }
        else if (tg_op == 4) /*SONGS*/
        {
            if (parse_dec(&numSongs, p, 1, MAX_SONGS) == false )
                return false;
        }
        else if (tg_op == 5) /*DEFSONG*/
        {
            if (parse_dec(&defSong, p, 0, MAX_SONGS) == false)
                return false;
        }
        else if (tg_op == 6) /*TIME*/
        {
            int durationTemp = ASAP_ParseDuration(p);
            if (durationTemp < 0 || duration_index >= MAX_SONGS)
                return false;
            durations[duration_index++] = durationTemp;
        }
    }

    /* set length: */
    int length =  durations[defSong];
    if (length < 0)
        length = 180 * 1000;
    id3->length = length;

    lseek(fd, 0, SEEK_SET);
    return true;
}


bool get_asap_metadata(int fd, struct mp3entry* id3)
{

    int filelength = filesize(fd);

    if(parse_sap_header(fd, id3, filelength) == false)
    {
        DEBUGF("parse sap header failed.\n");
        return false;
    }

    id3->bitrate = 706;
    id3->frequency = 44100;

    id3->vbr = false;
    id3->filesize = filelength;
    id3->genre_string = id3_get_num_genre(36);

    return true;
}
