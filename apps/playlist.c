/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include <file.h>
#include "sprintf.h"
#include "debug.h"
#include "mpeg.h"
#include "lcd.h"
#include "kernel.h"
#include "settings.h"
#include "status.h"
#include "applimits.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "widgets.h"
#endif

#include "lang.h"

static struct playlist_info playlist;

#define PLAYLIST_BUFFER_SIZE (AVERAGE_FILENAME_LENGTH*MAX_FILES_IN_DIR)

static unsigned char playlist_buffer[PLAYLIST_BUFFER_SIZE];
static int playlist_end_pos = 0;

static char now_playing[MAX_PATH+1];

void playlist_clear(void)
{
    playlist_end_pos = 0;
    playlist_buffer[0] = 0;
}

int playlist_add(char *filename)
{
    int len = strlen(filename);

    if(len+2 > PLAYLIST_BUFFER_SIZE - playlist_end_pos)
        return -1;

    strcpy(&playlist_buffer[playlist_end_pos], filename);
    playlist_end_pos += len;
    playlist_buffer[playlist_end_pos++] = '\n';
    playlist_buffer[playlist_end_pos] = '\0';
    return 0;
}

static int get_next_index(int steps)
{
    int current_index = playlist.index;
    int next_index    = -1;

    switch (global_settings.repeat_mode)
    {
        case REPEAT_OFF:
            if (current_index < playlist.first_index)
                current_index += playlist.amount;
            current_index -= playlist.first_index;

            next_index = current_index+steps;
            if ((next_index < 0) || (next_index >= playlist.amount))
                next_index = -1;
            else
                next_index = (next_index+playlist.first_index)%playlist.amount;
            break;

        case REPEAT_ONE:
            next_index = current_index;
            break;

        case REPEAT_ALL:
        default:
            next_index = (current_index+steps) % playlist.amount;
            while (next_index < 0)
                next_index += playlist.amount;
            break;
    }

    return next_index;
}

/* the mpeg thread might ask us */
int playlist_amount(void)
{
    return playlist.amount;
}

int playlist_first_index(void)
{
    return playlist.first_index;
}

char *playlist_name(char *buf, int buf_size)
{
    char *sep;

    snprintf(buf, buf_size, "%s", playlist.filename+playlist.dirlen);

    if (0 == buf[0])
        return NULL;

    /* Remove extension */
    sep = strrchr(buf, '.');
    if (NULL != sep)
        *sep = 0;
    
    return buf;
}

int playlist_next(int steps)
{
    playlist.index = get_next_index(steps);

    return playlist.index;
}

char* playlist_peek(int steps)
{
    int seek;
    int max;
    int fd;
    int i;
    char *buf;
    char dir_buf[MAX_PATH+1];
    char *dir_end;
    int index;

    index = get_next_index(steps);
    if (index >= 0)
        seek = playlist.indices[index];
    else
        return NULL;

    if(playlist.in_ram)
    {
        buf = playlist_buffer + seek;
        max = playlist_end_pos - seek;
    }
    else
    {
        fd = open(playlist.filename, O_RDONLY);
        if(-1 != fd)
        {
            buf = playlist_buffer;
            lseek(fd, seek, SEEK_SET);
            max = read(fd, buf, MAX_PATH);
            close(fd);
        }
        else
            return NULL;
    }

    /* Zero-terminate the file name */
    seek=0;
    while((buf[seek] != '\n') &&
          (buf[seek] != '\r') &&
          (seek < max))
        seek++;

    /* Now work back killing white space */
    while((buf[seek-1] == ' ') || 
          (buf[seek-1] == '\t'))
        seek--;

    buf[seek]=0;
      
    /* replace backslashes with forward slashes */
    for ( i=0; i<seek; i++ )
        if ( buf[i] == '\\' )
            buf[i] = '/';

    if('/' == buf[0]) {
        strcpy(now_playing, &buf[0]);
    }
    else {
        strncpy(dir_buf, playlist.filename, playlist.dirlen-1);
        dir_buf[playlist.dirlen-1] = 0;

        /* handle dos style drive letter */
        if ( ':' == buf[1] ) {
            strcpy(now_playing, &buf[2]);
        }
        else if ( '.' == buf[0] && '.' == buf[1] && '/' == buf[2] ) {
            /* handle relative paths */
            seek=3;
            while(buf[seek] == '.' &&
                  buf[seek+1] == '.' &&
                  buf[seek+2] == '/')
                seek += 3;
            for (i=0; i<seek/3; i++) {
                dir_end = strrchr(dir_buf, '/');
                if (dir_end)
                    *dir_end = '\0';
                else
                    break;
            }
            snprintf(now_playing, MAX_PATH+1, "%s/%s", dir_buf, &buf[seek]);
        }
        else if ( '.' == buf[0] && '/' == buf[1] ) {
            snprintf(now_playing, MAX_PATH+1, "%s/%s", dir_buf, &buf[2]);
        }
        else {
            snprintf(now_playing, MAX_PATH+1, "%s/%s", dir_buf, buf);
        }
    }

    buf = now_playing;

    /* remove bogus dirs from beginning of path
       (workaround for buggy playlist creation tools) */
    while (buf)
    {
        fd = open(buf, O_RDONLY);
        if (fd > 0)
        {
            close(fd);
            break;
        }

        buf = strchr(buf+1, '/');
    }

    if (buf)
        return buf;

    return now_playing;
}

/*
 * This function is called to start playback of a given playlist.  This
 * playlist may be stored in RAM (when using full-dir playback).
 *
 * Return: the new index (possibly different due to shuffle)
 */
int play_list(char *dir,         /* "current directory" */
              char *file,        /* playlist */
              int start_index,   /* index in the playlist */
              bool shuffled_index, /* if TRUE the specified index is for the
                                       playlist AFTER the shuffle */
              int start_offset,  /* offset in the file */
              int random_seed,   /* used for shuffling */
              int first_index )  /* first index of playlist */
{
    char *sep="";
    int dirlen;
    empty_playlist();

    playlist.index = start_index;
    playlist.first_index = first_index;

#ifdef HAVE_LCD_BITMAP
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    /* If file is NULL, the list is in RAM */
    if(file) {
        lcd_clear_display();
        lcd_puts(0,0,str(LANG_PLAYLIST_LOAD));
        status_draw();
        lcd_update();
        playlist.in_ram = false;
    } else {
        /* Assign a dummy filename */
        file = "";
        playlist.in_ram = true;
    }

    dirlen = strlen(dir);

    /* If the dir does not end in trailing slash, we use a separator.
       Otherwise we don't. */
    if('/' != dir[dirlen-1]) {
        sep="/";
        dirlen++;
    }
    
    playlist.dirlen = dirlen;

    snprintf(playlist.filename, sizeof(playlist.filename),
             "%s%s%s", 
             dir, sep, file);

    /* add track indices to playlist data structure */
    add_indices_to_playlist();
    
    if(global_settings.playlist_shuffle) {
        if(!playlist.in_ram) {
            lcd_puts(0,0,"Shuffling...");
            status_draw();
            lcd_update();
            randomise_playlist( random_seed );
        }
        else {
            int i;

            /* store the seek position before the shuffle */
            int seek_pos = playlist.indices[start_index];

            /* now shuffle around the indices */
            randomise_playlist( random_seed );

            if(!shuffled_index && global_settings.play_selected) {
                /* The given index was for the unshuffled list, so we need
                   to figure out the index AFTER the shuffle has been made.
                   We scan for the seek position we remmber from before. */

                for(i=0; i< playlist.amount; i++) {
                    if(seek_pos == playlist.indices[i]) {
                        /* here's the start song! yiepee! */
                        playlist.index = i;
                        playlist.first_index = i;
                        break; /* now stop searching */
                    }
                }
                /* if we for any reason wouldn't find the index again, it
                   won't set the index again and we should start at index 0
                   instead */
            }
        }
    }

    if(!playlist.in_ram) {
        lcd_puts(0,0,str(LANG_PLAYLIST_PLAY));
        status_draw();
        lcd_update();
    }
    /* also make the first song get playing */
    mpeg_play(start_offset);

    return playlist.index;
}

/*
 * remove any filename and indices associated with the playlist
 */
void empty_playlist(void)
{
    playlist.filename[0] = '\0';
    playlist.index = 0;
    playlist.amount = 0;
}

/*
 * calculate track offsets within a playlist file
 */
void add_indices_to_playlist(void)
{
    int nread;
    int fd = -1;
    int i = 0;
    int count = 0;
    int next_tick = current_tick + HZ;
    bool store_index;

    unsigned char *p = playlist_buffer;
    char line[16];

    if(!playlist.in_ram) {
        fd = open(playlist.filename, O_RDONLY);
        if(-1 == fd)
            return; /* failure */
    }

    store_index = true;

    while(1)
    {
        if(playlist.in_ram) {
            nread = playlist_end_pos;
        } else {
            nread = read(fd, playlist_buffer, PLAYLIST_BUFFER_SIZE);
            /* Terminate on EOF */
            if(nread <= 0)
                break;
        }
        
        p = playlist_buffer;

        for(count=0; count < nread; count++,p++) {

            /* Are we on a new line? */
            if((*p == '\n') || (*p == '\r'))
            {
                store_index = true;
            } 
            else if(store_index) 
            {
                store_index = false;

                if(playlist.in_ram || (*p != '#'))
                {
                    /* Store a new entry */
                    playlist.indices[ playlist.amount ] = i+count;
                    playlist.amount++;
                    if ( playlist.amount >= MAX_PLAYLIST_SIZE ) {
                        if(!playlist.in_ram)
                            close(fd);

                        lcd_clear_display();
                        lcd_puts(0,0,str(LANG_PLAYINDICES_PLAYLIST));
                        lcd_puts(0,1,str(LANG_PLAYINDICES_BUFFER));
                        lcd_update();
                        sleep(HZ*2);
                        lcd_clear_display();

                        return;
                    }

                    /* Update the screen if it takes very long */
                    if(!playlist.in_ram) {
                        if ( current_tick >= next_tick ) {
                            next_tick = current_tick + HZ;
                            snprintf(line, sizeof line,
                                     str(LANG_PLAYINDICES_AMOUNT), 
                                     playlist.amount);
                            lcd_puts(0,1,line);
                            status_draw();
                            lcd_update();
                        }
			        }
                }
            }
        }

        i+= count;

        if(playlist.in_ram)
            break;
    }
    if(!playlist.in_ram) {
        snprintf(line, sizeof line, str(LANG_PLAYINDICES_AMOUNT),
			       	playlist.amount);
        lcd_puts(0,1,line);
        status_draw();
        lcd_update();
        close(fd);
    }
}

/*
 * randomly rearrange the array of indices for the playlist
 */
void randomise_playlist( unsigned int seed )
{
    int count;
    int candidate;
    int store;
    
    /* seed with the given seed */
    srand( seed );

    /* randomise entire indices list */
    for(count = playlist.amount - 1; count >= 0; count--)
    {
        /* the rand is from 0 to RAND_MAX, so adjust to our value range */
        candidate = rand() % (count + 1);

        /* now swap the values at the 'count' and 'candidate' positions */
        store = playlist.indices[candidate];
        playlist.indices[candidate] = playlist.indices[count];
        playlist.indices[count] = store;
    }

    mpeg_flush_and_reload_tracks();
}

static int compare(const void* p1, const void* p2)
{
    int* e1 = (int*) p1;
    int* e2 = (int*) p2;

    return *e1 - *e2;
}

/*
 * Sort the array of indices for the playlist. If start_current is true then
 * set the index to the new index of the current song.
 */
void sort_playlist(bool start_current)
{
    int i;
    int current = playlist.indices[playlist.index];

    if (playlist.amount > 0)
    {
        qsort(&playlist.indices, playlist.amount, sizeof(playlist.indices[0]), compare);
    }

    if (start_current)
    {
        /* Set the index to the current song */
        for (i=0; i<playlist.amount; i++)
        {
            if (playlist.indices[i] == current)
            {
                playlist.index = i;
                break;
            }
        }
    }

    mpeg_flush_and_reload_tracks();
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../firmware/rockbox-mode.el")
 * end:
 */
