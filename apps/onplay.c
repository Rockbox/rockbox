/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "debug.h"
#include "sprintf.h"
#include "lcd.h"
#include "dir.h"
#include "file.h"
#include "mpeg.h"
#include "menu.h"
#include "lang.h"
#include "playlist.h"
#include "button.h"
#include "kernel.h"
#include "keyboard.h"
#include "mp3data.h"
#include "id3.h"
#include "screens.h"
#include "tree.h"
#include "buffer.h"
#include "settings.h"
#include "status.h"
#include "playlist_viewer.h"
#include "onplay.h"

static char* selected_file = NULL;
static int selected_file_attr = 0;
static int onplay_result = ONPLAY_OK;

/* For playlist options */
struct playlist_args {
    int position;
    bool queue;
};

static bool add_to_playlist(int position, bool queue)
{
    bool new_playlist = !(mpeg_status() & MPEG_STATUS_PLAY);

    if (new_playlist)
        playlist_create(NULL, NULL);

    if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        playlist_insert_track(NULL, selected_file, position, queue);
    else if (selected_file_attr & ATTR_DIRECTORY)
    {
        bool recurse = false;

        if (global_settings.recursive_dir_insert != RECURSE_ASK)
            recurse = (bool)global_settings.recursive_dir_insert;
        else
        {
            /* Ask if user wants to recurse directory */
            bool exit = false;
            
            lcd_clear_display();
            lcd_puts_scroll(0,0,str(LANG_RECURSE_DIRECTORY_QUESTION));
            lcd_puts_scroll(0,1,selected_file);
            
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0,3,str(LANG_CONFIRM_WITH_PLAY_RECORDER));
            lcd_puts(0,4,str(LANG_CANCEL_WITH_ANY_RECORDER)); 
#endif
            
            lcd_update();
            
            while (!exit) {
                int btn = button_get(true);
                switch (btn) {
                case BUTTON_PLAY:
                    recurse = true;
                    exit = true;
                    break;
                default:
                    /* ignore button releases */
                    if (!(btn & BUTTON_REL))
                        exit = true;
                    break;
                }
            }
        }

        playlist_insert_directory(NULL, selected_file, position, queue,
            recurse);
    }
    else if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_M3U)
        playlist_insert_playlist(NULL, selected_file, position, queue);

    if (new_playlist && (playlist_amount() > 0))
    {
        /* nothing is currently playing so begin playing what we just
           inserted */
        if (global_settings.playlist_shuffle)
            playlist_shuffle(current_tick, -1);
        playlist_start(0,0);
        status_set_playmode(STATUS_PLAY);
        status_draw(false);
        onplay_result = ONPLAY_START_PLAY;
    }

    return false;
}

static bool view_playlist(void)
{
    bool was_playing = mpeg_status() & MPEG_STATUS_PLAY;
    bool result;

    result = playlist_viewer_ex(selected_file);

    if (!was_playing && (mpeg_status() & MPEG_STATUS_PLAY) &&
        onplay_result == ONPLAY_OK)
        /* playlist was started from viewer */
        onplay_result = ONPLAY_START_PLAY;

    return result;
}

/* Sub-menu for playlist options */
static bool playlist_options(void)
{
    struct menu_items menu[7]; 
    struct playlist_args args[7]; /* increase these 2 if you add entries! */
    int m, i=0, pstart=0, result;
    bool ret = false;

    if ((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_M3U)
    {
        menu[i].desc = str(LANG_VIEW);
        menu[i].function = view_playlist;
        i++;
        pstart++;
    }

    if (mpeg_status() & MPEG_STATUS_PLAY)
    {
        menu[i].desc = str(LANG_INSERT);
        args[i].position = PLAYLIST_INSERT;
        args[i].queue = false;
        i++;
        
        menu[i].desc = str(LANG_INSERT_FIRST);
        args[i].position = PLAYLIST_INSERT_FIRST;
        args[i].queue = false;
        i++;
        
        menu[i].desc = str(LANG_INSERT_LAST);
        args[i].position = PLAYLIST_INSERT_LAST;
        args[i].queue = false;
        i++;
        
        menu[i].desc = str(LANG_QUEUE);
        args[i].position = PLAYLIST_INSERT;
        args[i].queue = true;
        i++;
        
        menu[i].desc = str(LANG_QUEUE_FIRST);
        args[i].position = PLAYLIST_INSERT_FIRST;
        args[i].queue = true;
        i++;
        
        menu[i].desc = str(LANG_QUEUE_LAST);
        args[i].position = PLAYLIST_INSERT_LAST;
        args[i].queue = true;
        i++;
    }
    else if (((selected_file_attr & TREE_ATTR_MASK) == TREE_ATTR_MPA) ||
             (selected_file_attr & ATTR_DIRECTORY))
    {
        menu[i].desc = str(LANG_INSERT);
        args[i].position = PLAYLIST_INSERT;
        args[i].queue = false;
        i++;
    }

    m = menu_init( menu, i );
    result = menu_show(m);
    if (result >= 0 && result < pstart)
        ret = menu[result].function();
    else if (result >= pstart)
        ret = add_to_playlist(args[result].position, args[result].queue);
    menu_exit(m);

    return ret;
}

static bool delete_file(void)
{
    bool exit = false;

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_REALLY_DELETE));
    lcd_puts_scroll(0,1,selected_file);

#ifdef HAVE_LCD_BITMAP
    lcd_puts(0,3,str(LANG_CONFIRM_WITH_PLAY_RECORDER));
    lcd_puts(0,4,str(LANG_CANCEL_WITH_ANY_RECORDER)); 
#endif

    lcd_update();

    while (!exit) {
        int btn = button_get(true);
        switch (btn) {
        case BUTTON_PLAY:
            if (!remove(selected_file)) {
                onplay_result = ONPLAY_RELOAD_DIR;
                lcd_clear_display();
                lcd_puts(0,0,str(LANG_DELETED));
                lcd_puts_scroll(0,1,selected_file);
                lcd_update();
                sleep(HZ);
                exit = true;
            }
            break;

        default:
            /* ignore button releases */
            if (!(btn & BUTTON_REL))
                exit = true;
            break;
        }
    }
    return false;
}

static bool rename_file(void)
{
    char newname[MAX_PATH];
    char* ptr = strrchr(selected_file, '/') + 1;
    int pathlen = (ptr - selected_file);
    strncpy(newname, selected_file, sizeof newname);
    if (!kbd_input(newname + pathlen, (sizeof newname)-pathlen)) {
        if (!strlen(selected_file+pathlen) ||
            (rename(selected_file, newname) < 0)) {
            lcd_clear_display();
            lcd_puts(0,0,str(LANG_RENAME));
            lcd_puts(0,1,str(LANG_FAILED));
            lcd_update();
            sleep(HZ*2);
        }
        else
            onplay_result = ONPLAY_RELOAD_DIR;
    }

    return false;
}

static void xingupdate(int percent)
{
    char buf[32];

    snprintf(buf, 32, "%d%%", percent);
    lcd_puts(0, 1, buf);
    lcd_update();
}


static int insert_data_in_file(char *fname, int fpos, char *buf, int num_bytes)
{
    int readlen;
    int rc;
    int orig_fd, fd;
    char tmpname[MAX_PATH];
    
    snprintf(tmpname, MAX_PATH, "%s.tmp", fname);

    orig_fd = open(fname, O_RDONLY);
    if(orig_fd < 0) {
        return 10*orig_fd - 1;
    }

    fd = creat(tmpname, O_WRONLY);
    if(fd < 0) {
        close(orig_fd);
        return 10*fd - 2;
    }

    /* First, copy the initial portion (the ID3 tag) */
    if(fpos) {
        readlen = read(orig_fd, mp3buf, fpos);
        if(readlen < 0) {
            close(fd);
            close(orig_fd);
            return 10*readlen - 3;
        }
        
        rc = write(fd, mp3buf, readlen);
        if(rc < 0) {
            close(fd);
            close(orig_fd);
            return 10*rc - 4;
        }
    }
    
    /* Now insert the data into the file */
    rc = write(fd, buf, num_bytes);
    if(rc < 0) {
        close(orig_fd);
        close(fd);
        return 10*rc - 5;
    }

    /* Copy the file */
    do {
        readlen = read(orig_fd, mp3buf, mp3end - mp3buf);
        if(readlen < 0) {
            close(fd);
            close(orig_fd);
            return 10*readlen - 7;
        }

        rc = write(fd, mp3buf, readlen);
        if(rc < 0) {
            close(fd);
            close(orig_fd);
            return 10*rc - 8;
        }
    } while(readlen > 0);
    
    close(fd);
    close(orig_fd);

    /* Remove the old file */
    rc = remove(fname);
    if(rc < 0) {
        return 10*rc - 9;
    }

    /* Replace the old file with the new */
    rc = rename(tmpname, fname);
    if(rc < 0) {
        return 10*rc - 9;
    }
    
    return 0;
}

static void fileerror(int rc)
{
    splash(HZ*2, true, "File error: %d", rc);
}

static const unsigned char empty_id3_header[] =
{
    'I', 'D', '3', 0x04, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x76 /* Size is 4096 minus 10 bytes for the header */
};

static bool vbr_fix(void)
{
    unsigned char xingbuf[1500];
    struct mp3entry entry;
    int fd;
    int rc;
    int flen;
    int num_frames;
    int numbytes;
    int framelen;
    int unused_space;

    if(mpeg_status()) {
        splash(HZ*2, true, str(LANG_VBRFIX_STOP_PLAY));
        return onplay_result;
    }
    
    lcd_clear_display();
    lcd_puts_scroll(0, 0, selected_file);
    lcd_update();

    xingupdate(0);

    rc = mp3info(&entry, selected_file);
    if(rc < 0) {
        fileerror(rc);
        return true;
    }
    
    fd = open(selected_file, O_RDWR);
    if(fd < 0) {
        fileerror(fd);
        return true;
    }

    flen = lseek(fd, 0, SEEK_END);

    xingupdate(0);

    num_frames = count_mp3_frames(fd, entry.first_frame_offset,
                                  flen, xingupdate);

    if(num_frames) {
        /* Note: We don't need to pass a template header because it will be
           taken from the mpeg stream */
        framelen = create_xing_header(fd, entry.first_frame_offset,
                                      flen, xingbuf, num_frames,
                                      0, xingupdate, true);
        
        /* Try to fit the Xing header first in the stream. Replace the existing
           VBR header if there is one, else see if there is room between the
           ID3 tag and the first MP3 frame. */
        if(entry.first_frame_offset - entry.id3v2len >=
           (unsigned int)framelen) {
            DEBUGF("Using existing space between ID3 and first frame\n");

            /* Seek to the beginning of the unused space */
            rc = lseek(fd, entry.id3v2len, SEEK_SET);
            if(rc < 0) {
                close(fd);
                fileerror(rc);
                return true;
            }

            unused_space =
                entry.first_frame_offset - entry.id3v2len - framelen;
            
            /* Fill the unused space with 0's (using the MP3 buffer)
               and write it to the file */
            if(unused_space)
            {
                memset(mp3buf, 0, unused_space);
                rc = write(fd, mp3buf, unused_space);
                if(rc < 0) {
                    close(fd);
                    fileerror(rc);
                    return true;
                }
            }

            /* Then write the Xing header */
            rc = write(fd, xingbuf, framelen);
            if(rc < 0) {
                close(fd);
                fileerror(rc);
                return true;
            }
            
            close(fd);
        } else {
            /* If not, insert some space. If there is an ID3 tag in the
               file we only insert just enough to squeeze the Xing header
               in. If not, we insert an additional empty ID3 tag of 4K. */
            
            close(fd);
            
            /* Nasty trick alert! The insert_data_in_file() function
               uses the MP3 buffer when copying the data. We assume
               that the ID3 tag isn't longer than 1MB so the xing
               buffer won't be overwritten. */
            
            if(entry.first_frame_offset) {
                DEBUGF("Inserting %d bytes\n", framelen);
                numbytes = framelen;
            } else {
                DEBUGF("Inserting 4096+%d bytes\n", framelen);
                numbytes = 4096 + framelen;
                
                memset(mp3buf + 0x100000, 0, numbytes);
                
                /* Insert the ID3 header */
                memcpy(mp3buf + 0x100000, empty_id3_header,
                       sizeof(empty_id3_header));
            }
            
            /* Copy the Xing header */
            memcpy(mp3buf + 0x100000 + numbytes - framelen, xingbuf, framelen);
            
            rc = insert_data_in_file(selected_file,
                                     entry.first_frame_offset,
                                     mp3buf + 0x100000, numbytes);
            
            if(rc < 0) {
                fileerror(rc);
                return true;
            }
        }
        
        xingupdate(100);
    }
    else
    {
        /* Not a VBR file */
        DEBUGF("Not a VBR file\n");
        splash(HZ*2, true, str(LANG_VBRFIX_NOT_VBR));
    }

    return false;
}

bool create_dir(void)
{
    char dirname[MAX_PATH];
    char *cwd;
    int rc;
    int pathlen;

    cwd = getcwd(NULL, 0);
    memset(dirname, 0, sizeof dirname);
    
    snprintf(dirname, sizeof dirname, "%s/",
             cwd[1] ? cwd : "");

    pathlen = strlen(dirname);
    rc = kbd_input(dirname + pathlen, (sizeof dirname)-pathlen);
    if(rc < 0)
        return false;

    rc = mkdir(dirname, 0);
    if(rc < 0) {
        splash(HZ, true, "%s %s", str(LANG_CREATE_DIR), str(LANG_FAILED));
    } else {
        onplay_result = ONPLAY_RELOAD_DIR;
    }

    return true;
}

int onplay(char* file, int attr)
{
    struct menu_items menu[5]; /* increase this if you add entries! */
    int m, i=0, result;

    onplay_result = ONPLAY_OK;

    if(file)
    {
        selected_file = file;
        selected_file_attr = attr;
        
        if (((attr & TREE_ATTR_MASK) == TREE_ATTR_MPA) ||
            (attr & ATTR_DIRECTORY) ||
            ((attr & TREE_ATTR_MASK) == TREE_ATTR_M3U))
        {
            menu[i].desc = str(LANG_PLAYINDICES_PLAYLIST);
            menu[i].function = playlist_options;
            i++;
        }
        
        menu[i].desc = str(LANG_RENAME);
        menu[i].function = rename_file;
        i++;
        
        if (!(attr & ATTR_DIRECTORY))
        {
            menu[i].desc = str(LANG_DELETE);
            menu[i].function = delete_file;
            i++;
        }
        
        if ((attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
        {
            menu[i].desc = str(LANG_VBRFIX);
            menu[i].function = vbr_fix;
            i++;
        }
    }

    menu[i].desc = str(LANG_CREATE_DIR);
    menu[i].function = create_dir;
    i++;

    /* DIY menu handling, since we want to exit after selection */
    m = menu_init( menu, i );
    result = menu_show(m);
    if (result >= 0)
        menu[result].function();
    menu_exit(m);

    return onplay_result;
}
