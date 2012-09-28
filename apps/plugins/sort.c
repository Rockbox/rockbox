/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Linus Nielsen Feltzing
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

/****************************************************************************
 * Buffer handling:
 *
 * We allocate the MP3 buffer for storing the text to be sorted, and then
 * search the buffer for newlines and store an array of character pointers
 * to the strings.
 *
 * The pointer array grows from the top of the buffer and downwards:
 *
 * |-------------|
 * | pointers[2] |--------|
 * | pointers[1] |------| |
 * | pointers[0] |----| | |
 * |-------------|    | | |
 * |             |    | | |
 * |             |    | | |
 * | free space  |    | | |
 * |             |    | | |
 * |             |    | | |
 * |-------------|    | | |
 * |             |    | | |
 * | line 3\0    |<---| | |
 * | line 2\0    |<-----| |
 * | line 1\0    |<-------|
 * |-------------|
 *
 * The advantage of this method is that we optimize the buffer usage.
 *
 * The disadvantage is that the string pointers will be loaded in reverse
 * order. We therefore sort the strings in reverse order as well, so we
 * don't have to sort an already sorted buffer.
 ****************************************************************************/

/***************************************************************************
 * TODO: Implement a merge sort for files larger than the buffer
 ****************************************************************************/



size_t buf_size;
static char *filename;
static int num_entries;
static char **pointers;
static char *stringbuffer;
static char crlf[2] = "\r\n";
static int bomsize;

/* Compare function for sorting backwards */
static int compare(const void* p1, const void* p2)
{
    char *s1 = *(char **)p1;
    char *s2 = *(char **)p2;

    return strcasecmp(s2, s1);
}

static void sort_buffer(void)
{
    qsort(pointers, num_entries, sizeof(char *), compare);
}

static int read_buffer(int offset)
{
    int fd;
    char *buf_ptr;
    char *tmp_ptr;
    int readsize;

    fd = open_utf8(filename, O_RDONLY);
    if(fd < 0)
        return 10 * fd - 1;

    bomsize = lseek(fd, 0, SEEK_CUR);
    offset += bomsize;

    /* Fill the buffer from the file */
    lseek(fd, offset, SEEK_SET);
    readsize = read(fd, stringbuffer, buf_size);
    close(fd);

    if(readsize < 0)
        return readsize * 10 - 2;

    /* Temporary fix until we can do merged sorting */
    if(readsize == (int)buf_size)
        return buf_size; /* File too big */

    buf_ptr = stringbuffer;
    num_entries = 0;

    do {
        tmp_ptr = buf_ptr;
        while(*buf_ptr != '\n' && buf_ptr < (char *)pointers) {
            /* Terminate the string with CR... */
            if(*buf_ptr == '\r')
                *buf_ptr = 0;
            buf_ptr++;
        }
        /* ...and with LF */
        if(*buf_ptr == '\n')
            *buf_ptr = 0;
        else {
            return tmp_ptr - stringbuffer; /* Buffer is full, return
                                              the point to resume at */
        }

        pointers--;
        *pointers = tmp_ptr;
        num_entries++;
        buf_ptr++;
    } while(buf_ptr < stringbuffer + readsize);

    return 0;
}

static int write_file(void)
{
    char tmpfilename[MAX_PATH+1];
    int fd;
    int i;
    int rc;

    /* Create a temporary file */
    snprintf(tmpfilename, MAX_PATH+1, "%s.tmp", filename);
    if (bomsize)
        fd = open_utf8(tmpfilename, O_WRONLY|O_CREAT|O_TRUNC);
    else
        fd = open(tmpfilename, O_WRONLY|O_CREAT|O_TRUNC, 0666);

    if(fd < 0)
        return 10 * fd - 1;

    /* Write the sorted strings, with appended CR/LF, to the temp file,
       in reverse order */
    for(i = num_entries-1;i >= 0;i--) {
        rc = write(fd, pointers[i], strlen(pointers[i]));
        if(rc < 0) {
            close(fd);
            return 10 * rc - 2;
        }

        rc = write(fd, crlf, 2);
        if(rc < 0) {
            close(fd);
            return 10 * rc - 3;
        }
    }

    close(fd);

    /* Remove the original file */
    rc = remove(filename);
    if(rc < 0) {
        return 10 * rc - 4;
    }

    /* Replace the old file with the new */
    rc = rename(tmpfilename, filename);
    if(rc < 0) {
        return 10 * rc - 5;
    }
    return 0;
}

enum plugin_status plugin_start(const void* parameter)
{
    char *buf;
    int rc;
    if(!parameter) return PLUGIN_ERROR;

    filename = (char *)parameter;

    buf = plugin_get_audio_buffer(&buf_size); /* start munching memory */

    stringbuffer = buf;
    pointers = (char **)(buf + buf_size - sizeof(int));

    lcd_clear_display();
    splash(0, "Loading...");

    rc = read_buffer(0);
    if(rc == 0) {
        lcd_clear_display();
        splash(0, "Sorting...");
        sort_buffer();

        lcd_clear_display();
        splash(0, "Writing...");

        rc = write_file();
        if(rc < 0) {
            lcd_clear_display();
            splashf(HZ, "Can't write file: %d", rc);
        } else {
            lcd_clear_display();
            splash(HZ, "Done");
        }
    } else {
        if(rc < 0) {
            lcd_clear_display();
            splashf(HZ, "Can't read file: %d", rc);
        } else {
            lcd_clear_display();
            splash(HZ, "The file is too big");
        }
    }

    return PLUGIN_OK;
}
