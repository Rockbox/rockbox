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
    splash(HZ*2, 0, true, "File error: %d", rc);
}

static const unsigned char empty_id3_header[] =
{
    'I', 'D', '3', 0x04, 0x00, 0x00,
    0x00, 0x00, 0x1f, 0x76 /* Size is 4096 minus 10 bytes for the header */
};

static bool vbr_fix(void)
{
    unsigned char xingbuf[417];
    struct mp3entry entry;
    int fd;
    int rc;
    int flen;
    int num_frames;
    int fpos;
    int numbytes;

    if(mpeg_status()) {
        splash(HZ*2, 0, true, str(LANG_VBRFIX_STOP_PLAY));
        return reload_dir;
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
        create_xing_header(fd, entry.first_frame_offset,
                           flen, xingbuf, num_frames, xingupdate, true);
        
        /* Try to fit the Xing header first in the stream. Replace the existing
           Xing header if there is one, else see if there is room between the
           ID3 tag and the first MP3 frame. */
        if(entry.vbr_header_pos) {
            /* Reuse existing Xing header */
            fpos = entry.vbr_header_pos;
            
            DEBUGF("Reusing Xing header at %d\n", fpos);
                
            rc = lseek(fd, entry.vbr_header_pos, SEEK_SET);
            if(rc < 0) {
                close(fd);
                fileerror(rc);
                return true;
            }
            
            rc = write(fd, xingbuf, 417);
            if(rc < 0) {
                close(fd);
                fileerror(rc);
                return true;
            }
            
            close(fd);
        } else {
            /* Any room between ID3 tag and first MP3 frame? */
            if(entry.first_frame_offset - entry.id3v2len > 417) {
                DEBUGF("Using existing space between ID3 and first frame\n");
                rc = lseek(fd, entry.first_frame_offset - 417, SEEK_SET);
                if(rc < 0) {
                    close(fd);
                    fileerror(rc);
                    return true;
                }
                
                rc = write(fd, xingbuf, 417);
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
                    DEBUGF("Inserting 417 bytes\n");
                    numbytes = 417;
                } else {
                    DEBUGF("Inserting 4096+417 bytes\n");
                    numbytes = 4096 + 417;
                    
                    memset(mp3buf + 0x100000, 0, numbytes);

                    /* Insert the ID3 header */
                    memcpy(mp3buf + 0x100000, empty_id3_header,
                           sizeof(empty_id3_header));
                }

                /* Copy the Xing header */
                memcpy(mp3buf + 0x100000 + numbytes - 417, xingbuf, 417);
                
                rc = insert_data_in_file(selected_file,
                                         entry.first_frame_offset,
                                         mp3buf + 0x100000, numbytes);
                
                if(rc < 0) {
                    fileerror(rc);
                    return true;
                }
            }
        }
        
        xingupdate(100);
    }
    else
    {
        /* Not a VBR file */
        DEBUGF("Not a VBR file\n");
        splash(HZ*2, 0, true, str(LANG_VBRFIX_NOT_VBR));
    }

    return false;
}

int onplay(char* file, int attr)
{
    struct menu_items menu[5]; /* increase this if you add entries! */
    int m, i=0, result;

    selected_file = file;
    
    if ((mpeg_status() & MPEG_STATUS_PLAY) && (attr & TREE_ATTR_MPA))
    {
        menu[i].desc = str(LANG_QUEUE);
        menu[i].function = queue_file;
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

    if (attr & TREE_ATTR_MPA)
    {
        menu[i].desc = str(LANG_VBRFIX);
        menu[i].function = vbr_fix;
        i++;
    }

    /* DIY menu handling, since we want to exit after selection */
    m = menu_init( menu, i );
    result = menu_show(m);
    if (result >= 0)
        menu[result].function();
    menu_exit(m);

    return reload_dir;
}
