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
#include "file.h"
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

#define QUEUE_FILE ROCKBOX_DIR "/.queue_file"
#define PLAYLIST_BUFFER_SIZE (AVERAGE_FILENAME_LENGTH*MAX_FILES_IN_DIR)

static unsigned char playlist_buffer[PLAYLIST_BUFFER_SIZE];
static int playlist_end_pos = 0;

static char now_playing[MAX_PATH+1];

/*
 * remove any files and indices associated with the playlist
 */
static void empty_playlist(bool queue_resume)
{
    int fd;

    playlist.filename[0] = '\0';
    playlist.index = 0;
    playlist.queue_index = 0;
    playlist.last_queue_index = 0;
    playlist.amount = 0;
    playlist.num_queued = 0;
    playlist.start_queue = 0;

    if (!queue_resume)
    {
        /* start with fresh queue file when starting new playlist */
        remove(QUEUE_FILE);
        fd = creat(QUEUE_FILE, 0);
        if (fd > 0)
            close(fd);
    }
}

/* update queue list after resume */
static void add_indices_to_queuelist(int seek)
{
    int nread;
    int fd = -1;
    int i = seek;
    int count = 0;
    bool store_index;
    char buf[MAX_PATH];

    unsigned char *p = buf;

    fd = open(QUEUE_FILE, O_RDONLY);
    if(fd < 0)
        return;

    nread = lseek(fd, seek, SEEK_SET);
    if (nread < 0)
        return;

    store_index = true;

    while(1)
    {
        nread = read(fd, buf, MAX_PATH);
        if(nread <= 0)
            break;

        p = buf;
        
        for(count=0; count < nread; count++,p++) {           
            if(*p == '\n')
                store_index = true;
            else if(store_index) 
            {
                store_index = false;
                
                playlist.queue_indices[playlist.last_queue_index] = i+count;
                playlist.last_queue_index =
                    (playlist.last_queue_index + 1) % MAX_QUEUED_FILES;
                playlist.num_queued++;
            }
        }
        
        i += count;
    }
}

static int get_next_index(int steps, bool *queue)
{
    int current_index = playlist.index;
    int next_index    = -1;

    if (global_settings.repeat_mode == REPEAT_ONE)
    {
        /* this is needed for repeat one to work with queue mode */
        steps = 0;
    }
    else if (steps >= 0)
    {
        /* Queue index starts from 0 which needs to be accounted for.  Also,
           after resume, this handles case where we want to begin with
           playlist */
        steps -= playlist.start_queue;
    }

    if (steps >= 0 && playlist.num_queued > 0 &&
        playlist.num_queued - steps > 0)
        *queue = true;
    else
    {
        *queue = false;
        if (playlist.num_queued)
        {
            if (steps >= 0)
            {
                /* skip the queued tracks */
                steps -= (playlist.num_queued - 1);
            }
            else if (!playlist.start_queue)
            {
                /* previous from queue list needs to go to current track in
                   playlist */
                steps += 1;
            }
        }
    }

    switch (global_settings.repeat_mode)
    {
        case REPEAT_OFF:
            if (*queue)
                next_index = (playlist.queue_index+steps) % MAX_QUEUED_FILES;
            else
            {
                if (current_index < playlist.first_index)
                    current_index += playlist.amount;
                current_index -= playlist.first_index;
                
                next_index = current_index+steps;
                if ((next_index < 0) || (next_index >= playlist.amount))
                    next_index = -1;
                else
                    next_index = (next_index+playlist.first_index) %
                        playlist.amount;
            }
            break;

        case REPEAT_ONE:
            /* if we are still in playlist when repeat one is set, don't go to
               queue list */
            if (*queue && !playlist.start_queue)
                next_index = playlist.queue_index;
            else
            {
                next_index = current_index;
                *queue = false;
            }
            break;

        case REPEAT_ALL:
        default:
            if (*queue)
                next_index = (playlist.queue_index+steps) % MAX_QUEUED_FILES;
            else
            {
                next_index = (current_index+steps) % playlist.amount;
                while (next_index < 0)
                    next_index += playlist.amount;
            }
            break;
    }

    return next_index;
}

int playlist_amount(void)
{
    return playlist.amount + playlist.num_queued;
}

int playlist_first_index(void)
{
    return playlist.first_index;
}

/* Get resume info for current playing song.  If return value is -1 then
   settings shouldn't be saved (eg. when playing queued song and save queue
   disabled) */
int playlist_get_resume_info(int *resume_index, int *queue_resume,
                             int *queue_resume_index)
{
    int result = 0;

    *resume_index = playlist.index;

    if (playlist.num_queued > 0)
    {
        if (global_settings.save_queue_resume)
        {
            *queue_resume_index =
                playlist.queue_indices[playlist.queue_index];
            /* have we started playing the queue list yet? */
            if (playlist.start_queue)
                *queue_resume = QUEUE_BEGIN_PLAYLIST;
            else
                *queue_resume = QUEUE_BEGIN_QUEUE;
        }
        else if (!playlist.start_queue)
        {
            *queue_resume = QUEUE_OFF;
            result = -1;
        }
    }
    else
        *queue_resume = QUEUE_OFF;

    return result;
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

/* Add track to queue file */
int queue_add(char *filename)
{
    int fd, seek, result;
    int len = strlen(filename);

    if(playlist.num_queued >= MAX_QUEUED_FILES)
        return -1;

    fd = open(QUEUE_FILE, O_WRONLY);
    if (fd < 0)
        return -1;

    seek = lseek(fd, 0, SEEK_END);
    if (seek < 0)
    {
        close(fd);
        return -1;
    }

    /* save the file name with a trailing \n. QUEUE_FILE can be used as a 
       playlist if desired */
    filename[len] = '\n';
    result = write(fd, filename, len+1);
    if (result < 0)
    {
        close(fd);
        return -1;
    }
    filename[len] = '\0';

    close(fd);

    if (playlist.num_queued <= 0)
        playlist.start_queue = 1;

    playlist.queue_indices[playlist.last_queue_index] = seek;
    playlist.last_queue_index =
        (playlist.last_queue_index + 1) % MAX_QUEUED_FILES;
    playlist.num_queued++;

    /* the play order has changed */
    mpeg_flush_and_reload_tracks();

    return playlist.num_queued;
}

int playlist_next(int steps)
{
    bool queue;
    int index = get_next_index(steps, &queue);

    if (queue)
    {
        /* queue_diff accounts for bad songs in queue list */
        int queue_diff = index - playlist.queue_index;
        if (queue_diff < 0)
            queue_diff += MAX_QUEUED_FILES;

        playlist.num_queued -= queue_diff;
        playlist.queue_index = index;
        playlist.start_queue = 0;
    }
    else
    {
        playlist.index = index;
        if (playlist.num_queued > 0 && !playlist.start_queue)
        {
            if (steps >= 0)
            {
                /* done with queue list */
                playlist.queue_index = playlist.last_queue_index;
                playlist.num_queued = 0;
            }
            else
            {
                /* user requested previous.  however, don't forget about queue
                   list */
                playlist.start_queue = 1;
            }
        }
    }

    return index;
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
    bool queue;

    index = get_next_index(steps, &queue);
    if (index < 0)
        return NULL;

    if (queue)
    {
        seek = playlist.queue_indices[index];
        fd = open(QUEUE_FILE, O_RDONLY);
        if(-1 != fd)
        {
            buf = dir_buf;
            lseek(fd, seek, SEEK_SET);
            max = read(fd, buf, MAX_PATH);
            close(fd);
        }
        else
            return NULL;
    }
    else
    {
        seek = playlist.indices[index];
        
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

    if (!buf)
    {
        /* Even though this is an invalid file, we still need to pass a file
           name to the caller because NULL is used to indicate end of 
           playlist */
        return now_playing;
    }

    return buf;
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
              int first_index,   /* first index of playlist */
              int queue_resume, /* resume queue list? */
              int queue_resume_index ) /* queue list seek pos */
{
    char *sep="";
    int dirlen;
    empty_playlist(queue_resume);

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

    if (queue_resume)
    {
        /* update the queue indices */
        add_indices_to_queuelist(queue_resume_index);

        if (queue_resume == QUEUE_BEGIN_PLAYLIST)
        {
            /* begin with current playlist index */
            playlist.start_queue = 1;
            playlist.index++; /* so we begin at the correct track */
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
