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

static char* selected_file = NULL;
static bool reload_dir = false;

static bool queue_file(void)
{
    queue_add(selected_file);
    return false;
}

static bool delete_file(void)
{
    bool exit = false;

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_REALLY_DELETE));
    lcd_puts_scroll(0,1,selected_file);
    lcd_update();

    while (!exit) {
        int btn = button_get(true);
        switch (btn) {
        case BUTTON_PLAY:
            if (!remove(selected_file)) {
                reload_dir = true;
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
            reload_dir = true;
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


/* defined in linker script */
extern unsigned char mp3buf[];
extern unsigned char mp3end[];

static int insert_space_in_file(char *fname, int num_bytes)
{
    int readlen;
    int rc;
    int orig_fd, fd;
    char tmpname[MAX_PATH];
    
    /* Use the mp3 buffer to write 0's in the file */
    memset(mp3buf, 0, num_bytes);

    snprintf(tmpname, MAX_PATH, "%s.tmp", fname);

    orig_fd = open(fname, O_RDONLY);
    if(orig_fd < 0)
    {
        return 10*orig_fd - 1;
    }

    fd = creat(tmpname, O_WRONLY);
    if(fd < 0)
    {
        close(orig_fd);
        return 10*fd - 2;
    }

    rc = write(fd, mp3buf, num_bytes);
    if(rc < 0)
    {
        close(orig_fd);
        close(fd);
        return 10*rc - 3;
    }

    rc = lseek(orig_fd, 0, SEEK_SET);
    if(rc < 0)
    {
        close(orig_fd);
        close(fd);
        return 10*rc - 4;
    }

    /* Copy the file */
    do
    {
        readlen = read(orig_fd, mp3buf, mp3end - mp3buf);
        if(readlen < 0)
        {
            close(fd);
            close(orig_fd);
            return 10*readlen - 5;
        }

        rc = write(fd, mp3buf, readlen);
        if(rc < 0)
        {
            close(fd);
            close(orig_fd);
            return 10*rc - 6;
        }
    } while(readlen > 0);
    
    close(fd);
    close(orig_fd);

    /* Remove the old file */
    rc = remove(fname);
    if(rc < 0)
    {
        return 10*rc - 7;
    }

    /* Replace the old file with the new */
    rc = rename(tmpname, fname);
    if(rc < 0)
    {
        return 10*rc - 8;
    }
    
    return 0;
}
        
static bool vbr_fix(void)
{
    unsigned char xingbuf[417];
    struct mp3entry entry;
    int fd;
    int rc;
    int flen;
    int num_frames;
    int fpos;

    if(mpeg_status())
    {
        splash(HZ*2, 0, true, "Stop the playback");
        return reload_dir;
    }
    
    lcd_clear_display();
    lcd_puts_scroll(0, 0, selected_file);
    lcd_update();

    xingupdate(0);

    rc = mp3info(&entry, selected_file);
    if(rc < 0)
        return rc * 10 - 1;
    
    fd = open(selected_file, O_RDWR);
    if(fd < 0)
        return fd * 10 - 2;

    flen = lseek(fd, 0, SEEK_END);

    xingupdate(0);

    num_frames = count_mp3_frames(fd, entry.first_frame_offset,
                                  flen, xingupdate);

    if(num_frames)
    {
        create_xing_header(fd, entry.first_frame_offset,
                           flen, xingbuf, num_frames, xingupdate, true);
        
        /* Try to fit the Xing header first in the stream. Replace the existing
           Xing header if there is one, else see if there is room between the
           ID3 tag and the first MP3 frame. */
        if(entry.vbr_header_pos)
        {
            /* Reuse existing Xing header */
            fpos = entry.vbr_header_pos;
            
            DEBUGF("Reusing Xing header at %d\n", fpos);
        }
        else
        {
            /* Any room between ID3 tag and first MP3 frame? */
            if(entry.first_frame_offset - entry.id3v2len > 417)
            {
                fpos = entry.first_frame_offset - 417;
            }
            else
            {
                /* If not, insert some space, but only if there is no ID3
                   tag in the file */
                close(fd);
                if(entry.first_frame_offset == 0)
                {
                    rc = insert_space_in_file(selected_file, 4096);
                    if(rc < 0)
                    {
                        splash(HZ*2, 0, true, "File error: %d", rc);
                        return rc * 10 - 3;
                    }

                    /* Reopen the file */
                    fd = open(selected_file, O_RDWR);
                    if(fd < 0)
                        return fd * 10 - 4;

                    fpos = 4096-417;
                }
                else
                {
                    splash(HZ*2, 0, true,
                           "There is no room for a Xing header");
                    return -3;
                }
            }
        }
        
        lseek(fd, fpos, SEEK_SET);
        write(fd, xingbuf, 417);
        close(fd);
        
        xingupdate(100);
    }
    else
    {
        /* Not a VBR file */
        DEBUGF("Not a VBR file\n");
        splash(HZ*2, 0, true, "Not a VBR file");
    }

    return false;
}
int onplay(char* file, int attr)
{
    struct menu_items menu[5]; /* increase this if you add entries! */
    int m, i=0, result;

    selected_file = file;
    
    if ((mpeg_status() & MPEG_STATUS_PLAY) && (attr & TREE_ATTR_MPA))
        menu[i++] = (struct menu_items) { str(LANG_QUEUE), queue_file };

    menu[i++] = (struct menu_items) { str(LANG_RENAME), rename_file };

    if (!(attr & ATTR_DIRECTORY))
        menu[i++] = (struct menu_items) { str(LANG_DELETE), delete_file };

    if (attr & TREE_ATTR_MPA)
        menu[i++] = (struct menu_items) { "VBRfix", vbr_fix };

    /* DIY menu handling, since we want to exit after selection */
    m = menu_init( menu, i );
    result = menu_show(m);
    if (result >= 0)
        menu[result].function();
    menu_exit(m);

    return reload_dir;
}
