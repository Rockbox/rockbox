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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

static struct plugin_api* rb;

int buf_size;
static char *filename;
static int num_entries;
static char **pointers;
static char *stringbuffer;
static char crlf[2] = "\r\n";

/* Compare function for sorting backwards */
static int compare(const void* p1, const void* p2)
{
    char *s1 = *(char **)p1;
    char *s2 = *(char **)p2;

    return rb->strcasecmp(s2, s1);
}

static void sort_buffer(void)
{
    rb->qsort(pointers, num_entries, sizeof(char *), compare);
}

int read_buffer(int offset)
{
    int fd;
    char *buf_ptr;
    char *tmp_ptr;
    int readsize;
    
    fd = rb->open(filename, O_RDONLY);
    if(fd < 0)
        return 10 * fd - 1;

    /* Fill the buffer from the file */
    rb->lseek(fd, offset, SEEK_SET);
    readsize = rb->read(fd, stringbuffer, buf_size);
    rb->close(fd);

    if(readsize < 0)
        return readsize * 10 - 2;

    /* Temporary fix until we can do merged sorting */
    if(readsize == buf_size)
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
    rb->snprintf(tmpfilename, MAX_PATH+1, "%s.tmp", filename);
    fd = rb->creat(tmpfilename, 0);
    if(fd < 0)
        return 10 * fd - 1;

    /* Write the sorted strings, with appended CR/LF, to the temp file,
       in reverse order */
    for(i = num_entries-1;i >= 0;i--) {
        rc = rb->write(fd, pointers[i], rb->strlen(pointers[i]));
        if(rc < 0) {
            rb->close(fd);
            return 10 * rc - 2;
        }

        rc = rb->write(fd, crlf, 2);
        if(rc < 0) {
            rb->close(fd);
            return 10 * rc - 3;
        }
    }

    rb->close(fd);

    /* Remove the original file */
    rc = rb->remove(filename);
    if(rc < 0) {
        return 10 * rc - 4;
    }

    /* Replace the old file with the new */
    rc = rb->rename(tmpfilename, filename);
    if(rc < 0) {
        return 10 * rc - 5;
    }
    return 0;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    char *buf;
    int rc;
    TEST_PLUGIN_API(api);

    filename = (char *)parameter;

    rb = api;

    buf = rb->plugin_get_mp3_buffer(&buf_size); /* start munching memory */

    stringbuffer = buf;
    pointers = (char **)(buf + buf_size - sizeof(int));

    rb->lcd_clear_display();
    rb->splash(0, true, "Loading...");
    
    rc = read_buffer(0);
    if(rc == 0) {
        rb->lcd_clear_display();
        rb->splash(0, true, "Sorting...");
        sort_buffer();
        
        rb->lcd_clear_display();
        rb->splash(0, true, "Writing...");
        
        rc = write_file();
        if(rc < 0) {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Can't write file: %d", rc);
        } else {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Done");
        }
    } else {
        if(rc < 0) {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "Can't read file: %d", rc);
        } else {
            rb->lcd_clear_display();
            rb->splash(HZ, true, "The file is too big");
        }
    }
    
    return PLUGIN_OK;
}
