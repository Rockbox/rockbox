/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
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

#include "applimits.h"
#include "dir.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "button.h"
#include "kernel.h"
#include "usb.h"
#include "tree.h"
#include "main_menu.h"
#include "sprintf.h"
#include "mpeg.h"
#include "playlist.h"
#include "menu.h"
#include "wps.h"
#include "wps-display.h"
#include "settings.h"
#include "status.h"
#include "debug.h"
#include "ata.h"
#include "rolo.h"
#include "icons.h"
#include "lang.h"
#include "viewer.h"
#include "language.h"
#include "screens.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

#define NAME_BUFFER_SIZE (AVERAGE_FILENAME_LENGTH * MAX_FILES_IN_DIR)

static char name_buffer[NAME_BUFFER_SIZE];
static int name_buffer_length;
struct entry {
    short attr; /* FAT attributes + file type flags */
    char *name;
};

static struct entry dircache[MAX_FILES_IN_DIR];
static int dircursor;
static int dirstart;
static int dirlevel;
static int filesindir;
static int dirpos[MAX_DIR_LEVELS];
static int cursorpos[MAX_DIR_LEVELS];
static char lastdir[MAX_PATH];
static char lastfile[MAX_PATH];
static char currdir[MAX_PATH];

void browse_root(void)
{
    dirbrowse("/");
}


#ifdef HAVE_LCD_BITMAP

/* pixel margins */
#define MARGIN_X (global_settings.scrollbar && \
                  filesindir > tree_max_on_screen ? SCROLLBAR_WIDTH : 0) + \
                  CURSOR_WIDTH + ICON_WIDTH
#define MARGIN_Y (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

/* position the entry-list starts at */
#define LINE_X   0 
#define LINE_Y   (global_settings.statusbar ? 1 : 0)

#define CURSOR_X (global_settings.scrollbar && \
                  filesindir > tree_max_on_screen ? 1 : 0)
#define CURSOR_Y 0 /* the cursor is not positioned in regard to
                      the margins, so this is the amount of lines
                      we add to the cursor Y position to position
                      it on a line */
#define CURSOR_WIDTH  4

#define ICON_WIDTH    6

#define SCROLLBAR_X      0
#define SCROLLBAR_Y      lcd_getymargin()
#define SCROLLBAR_WIDTH  6

extern unsigned char bitmap_icons_6x8[LastIcon][6];

#else /* HAVE_LCD_BITMAP */

#define TREE_MAX_ON_SCREEN   2
#define TREE_MAX_LEN_DISPLAY 11 /* max length that fits on screen */
#define LINE_X      2 /* X position the entry-list starts at */
#define LINE_Y      0 /* Y position the entry-list starts at */

#define CURSOR_X    0
#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_RECORDER_KEYPAD
#define TREE_NEXT  BUTTON_DOWN
#define TREE_PREV  BUTTON_UP
#define TREE_EXIT  BUTTON_LEFT
#define TREE_ENTER BUTTON_RIGHT
#define TREE_MENU  BUTTON_F1
#else
#define TREE_NEXT  BUTTON_RIGHT
#define TREE_PREV  BUTTON_LEFT
#define TREE_EXIT  BUTTON_STOP
#define TREE_ENTER BUTTON_PLAY
#define TREE_MENU  BUTTON_MENU
#endif /* HAVE_RECORDER_KEYPAD */

/* using attribute not used by FAT */
#define TREE_ATTR_MPA 0x40 /* mpeg audio file */
#define TREE_ATTR_M3U 0x80 /* playlist */
#define TREE_ATTR_WPS 0x100 /* wps config file */
#define TREE_ATTR_MOD 0x200 /* firmware file */
#define TREE_ATTR_CFG 0x400 /* config file */
#define TREE_ATTR_TXT 0x500 /* text file */
#define TREE_ATTR_FONT 0x800 /* font file */
#define TREE_ATTR_LNG  0x1000 /* binary lang file */
#define TREE_ATTR_MASK 0xffd0 /* which bits tree.c uses (above + DIR) */

static int build_playlist(int start_index)
{
    int i;
    int start=start_index;

    playlist_clear();

    for(i = 0;i < filesindir;i++)
    {
        if(dircache[i].attr & TREE_ATTR_MPA)
        {
            DEBUGF("Adding %s\n", dircache[i].name);
            playlist_add(dircache[i].name);
        }
        else
        {
            /* Adjust the start index when se skip non-MP3 entries */
            if(i < start)
                start_index--;
        }
    }

    return start_index;
}

static int compare(const void* p1, const void* p2)
{
    struct entry* e1 = (struct entry*)p1;
    struct entry* e2 = (struct entry*)p2;
    
    if (( e1->attr & ATTR_DIRECTORY ) == ( e2->attr & ATTR_DIRECTORY ))
        if (global_settings.sort_case)
            return strncmp(e1->name, e2->name, MAX_PATH);
        else
            return strncasecmp(e1->name, e2->name, MAX_PATH);
    else 
        return ( e2->attr & ATTR_DIRECTORY ) - ( e1->attr & ATTR_DIRECTORY );
}

static int showdir(char *path, int start)
{
    int icon_type = 0;
    int i;
    int tree_max_on_screen;
    bool dir_buffer_full;

#ifdef HAVE_LCD_BITMAP
    int line_height;
    int fw, fh;
    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
    line_height = fh;
#else
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif

    /* new dir? cache it */
    if (strncmp(path,lastdir,sizeof(lastdir))) {
        DIR *dir = opendir(path);
        if(!dir)
            return -1; /* not a directory */

        name_buffer_length = 0;
        dir_buffer_full = false;
        
        for ( i=0; i<MAX_FILES_IN_DIR; i++ ) {
            int len;
            struct dirent *entry = readdir(dir);
            struct entry* dptr = &dircache[i];
            if (!entry)
                break;

            len = strlen(entry->d_name);

            /* skip directories . and .. */
            if ((entry->attribute & ATTR_DIRECTORY) &&
                (((len == 1) && 
                  (!strncmp(entry->d_name, ".", 1))) ||
                 ((len == 2) && 
                  (!strncmp(entry->d_name, "..", 2))))) {
                i--;
                continue;
            }

            /* Skip FAT volume ID */
            if (entry->attribute & ATTR_VOLUME_ID) {
                i--;
                continue;
            }

            /* filter out dotfiles and hidden files */
            if (global_settings.dirfilter != SHOW_ALL &&
                ((entry->d_name[0]=='.') ||
                 (entry->attribute & ATTR_HIDDEN))) {
                i--;
                continue;
            }

            dptr->attr = entry->attribute;

            /* mark mp? and m3u files as such */
            if ( !(dptr->attr & ATTR_DIRECTORY) && (len > 4) ) {
                if (!strcasecmp(&entry->d_name[len-4], ".mp3") ||
                    (!strcasecmp(&entry->d_name[len-4], ".mp2")) ||
                    (!strcasecmp(&entry->d_name[len-4], ".mpa")))
                    dptr->attr |= TREE_ATTR_MPA;
                else if (!strcasecmp(&entry->d_name[len-4], ".m3u"))
                    dptr->attr |= TREE_ATTR_M3U;
                else if (!strcasecmp(&entry->d_name[len-4], ".cfg"))
                    dptr->attr |= TREE_ATTR_CFG;
                else if (!strcasecmp(&entry->d_name[len-4], ".wps"))
                    dptr->attr |= TREE_ATTR_WPS;
                else if (!strcasecmp(&entry->d_name[len-4], ".txt"))
                    dptr->attr |= TREE_ATTR_TXT;
                else if (!strcasecmp(&entry->d_name[len-4], ".lng"))
                    dptr->attr |= TREE_ATTR_LNG;
#ifdef HAVE_RECORDER_KEYPAD
                else if (!strcasecmp(&entry->d_name[len-4], ".fnt"))
                    dptr->attr |= TREE_ATTR_FONT;
                else if (!strcasecmp(&entry->d_name[len-4], ".ajz"))
#else
                else if (!strcasecmp(&entry->d_name[len-4], ".mod"))
#endif
                    dptr->attr |= TREE_ATTR_MOD;
            }

            /* filter out all non-playlist files */
            if ( global_settings.dirfilter == SHOW_PLAYLIST &&
                 (!(dptr->attr &
                    (ATTR_DIRECTORY|TREE_ATTR_M3U))) ) {
                i--;
                continue;
            }
            
            /* filter out non-music files */
            if ( global_settings.dirfilter == SHOW_MUSIC &&
                 (!(dptr->attr &
                    (ATTR_DIRECTORY|TREE_ATTR_MPA|TREE_ATTR_M3U))) ) {
                i--;
                continue;
            }

            /* filter out non-supported files */
            if ( global_settings.dirfilter == SHOW_SUPPORTED &&
                 (!(dptr->attr & TREE_ATTR_MASK)) ) {
                i--;
                continue;
            }

            if (len > NAME_BUFFER_SIZE - name_buffer_length - 1) {
                /* Tell the world that we ran out of buffer space */
                dir_buffer_full = true;
                break;
            }
            dptr->name = &name_buffer[name_buffer_length];
            strcpy(dptr->name,entry->d_name);
            name_buffer_length += len + 1;
        }
        filesindir = i;
        closedir(dir);
        strncpy(lastdir,path,sizeof(lastdir));
        lastdir[sizeof(lastdir)-1] = 0;
        qsort(dircache,filesindir,sizeof(struct entry),compare);

        if ( dir_buffer_full || filesindir == MAX_FILES_IN_DIR ) {
#ifdef HAVE_LCD_CHARCELLS
            lcd_double_height(false);
#endif
            lcd_clear_display();
            lcd_puts(0,0,str(LANG_SHOWDIR_ERROR_BUFFER));
            lcd_puts(0,1,str(LANG_SHOWDIR_ERROR_FULL));
            lcd_update();
            sleep(HZ*2);
            lcd_clear_display();
        }
    }

    if (start == -1)
    {
        int diff_files;

        /* use lastfile to determine start (default=0) */
        start = 0;

        for (i=0; i<filesindir; i++)
        {
            if (!strcasecmp(dircache[i].name, lastfile))
            {
                start = i;
                break;
            }
        }

        diff_files = filesindir - start;
        if (diff_files < tree_max_on_screen)
        {
            int oldstart = start;

            start -= (tree_max_on_screen - diff_files);
            if (start < 0)
                start = 0;

            dircursor = oldstart - start;
        }

        dirstart = start;
    }

    lcd_stop_scroll();
#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif
    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and icon */
    lcd_setfont(FONT_UI);
#endif

    for ( i=start; i < start+tree_max_on_screen; i++ ) {
        int len;

        if ( i >= filesindir )
            break;

        len = strlen(dircache[i].name);

        switch ( dircache[i].attr & TREE_ATTR_MASK ) {
            case ATTR_DIRECTORY:
                icon_type = Folder;
                break;

            case TREE_ATTR_M3U:
                icon_type = Playlist;
                break;

            case TREE_ATTR_MPA:
                icon_type = File;
                break;

            case TREE_ATTR_WPS:
                icon_type = Wps;
                break;

            case TREE_ATTR_CFG:
                icon_type = Config;
                break;

            case TREE_ATTR_TXT:
                icon_type = Text;
                break;

            case TREE_ATTR_LNG:
                icon_type = Language;
                break;

            case TREE_ATTR_MOD:
                icon_type = Mod_Ajz;
                break;

#ifdef HAVE_LCD_BITMAP
            case TREE_ATTR_FONT:
                icon_type = Font;
                break;
#endif
            default:
                icon_type = 0;
        }

        if (icon_type) {
#ifdef HAVE_LCD_BITMAP
            int offset=0;
            if ( line_height > 8 )
                offset = (line_height - 8) / 2;
            lcd_bitmap(bitmap_icons_6x8[icon_type], 
                       CURSOR_X * 6 + CURSOR_WIDTH, 
                       MARGIN_Y+(i-start)*line_height + offset,
                       6, 8, true);
#else
            lcd_define_pattern((i-start)*8,tree_icons_5x7[icon_type],8);
            lcd_putc(LINE_X-1, i-start, i-start);
#endif
        }

        /* if music filter is on, cut off the extension */
        if (global_settings.dirfilter == SHOW_MUSIC && 
            (dircache[i].attr & (TREE_ATTR_M3U|TREE_ATTR_MPA)))
        {
            char temp = dircache[i].name[len-4];
            dircache[i].name[len-4] = 0;
            lcd_puts(LINE_X, i-start, dircache[i].name);
            dircache[i].name[len-4] = temp;
        }
        else
            lcd_puts(LINE_X, i-start, dircache[i].name);
    }

#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && filesindir > tree_max_on_screen) 
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  LCD_HEIGHT - SCROLLBAR_Y, filesindir, start,
                  start + tree_max_on_screen, VERTICAL);
#endif
    status_draw();
    return filesindir;
}

bool ask_resume(void)
{
#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif

    /* always resume? */
    if ( global_settings.resume == RESUME_ON )
        return true;

    if ( global_settings.resume == RESUME_ASK_ONCE) {
        global_settings.resume_index = -1;
        settings_save();
    }

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_RESUME_ASK));
#ifdef HAVE_LCD_CHARCELLS
    lcd_puts(0,1,str(LANG_RESUME_CONFIRM_PLAYER));
#else
    lcd_puts(0,1,str(LANG_RESUME_CONFIRM_RECORDER));
    lcd_puts(0,2,str(LANG_RESUME_CANCEL_RECORDER));
#endif
    lcd_update();

    if (button_get(true) == BUTTON_PLAY)
        return true;
    return false;
}

void start_resume(void)
{
    if ( global_settings.resume &&
         global_settings.resume_index != -1 ) {
        int len = strlen(global_settings.resume_file);

        DEBUGF("Resume file %s\n",global_settings.resume_file);
        DEBUGF("Resume index %X offset %X\n",
               global_settings.resume_index,
               global_settings.resume_offset);
        DEBUGF("Resume shuffle %s seed %X\n",
               global_settings.playlist_shuffle?"on":"off",
               global_settings.resume_seed);

        /* playlist? */
        if (!strcasecmp(&global_settings.resume_file[len-4], ".m3u")) {
            char* slash;

            /* check that the file exists */
            int fd = open(global_settings.resume_file, O_RDONLY);
            if(fd<0)
                return;
            close(fd);

            if (!ask_resume())
                return;
            
            slash = strrchr(global_settings.resume_file,'/');
            if (slash) {
                *slash=0;
                play_list(global_settings.resume_file,
                          slash+1,
                          global_settings.resume_index,
                          true, /* the index is AFTER shuffle */
                          global_settings.resume_offset,
                          global_settings.resume_seed,
                          global_settings.resume_first_index);
                *slash='/';
            }
            else {
                /* check that the dir exists */
                DIR* dir = opendir(global_settings.resume_file);
                if(!dir)
                    return;
                closedir(dir);

                if (!ask_resume())
                    return;

                play_list("/",
                          global_settings.resume_file,
                          global_settings.resume_index,
                          true,
                          global_settings.resume_offset,
                          global_settings.resume_seed,
                          global_settings.resume_first_index);
            }
        }
        else {
            if (!ask_resume())
                return;

            if (showdir(global_settings.resume_file, 0) < 0 )
                return;

            lastdir[0] = '\0';

            build_playlist(global_settings.resume_index);
            play_list(global_settings.resume_file,
                      NULL, 
                      global_settings.resume_index,
                      true,
                      global_settings.resume_offset,
                      global_settings.resume_seed,
                      global_settings.resume_first_index);
        }

        status_set_playmode(STATUS_PLAY);
        status_draw();
        wps_show();
    }
}

void set_current_file(char *path)
{
    char *name;
    unsigned int i;

    /* separate directory from filename */
    name = strrchr(path+1,'/');
    if (name)
    {
        *name = 0;
        strcpy(currdir, path);
        *name = '/';
        name++;
    }
    else
    {
        strcpy(currdir, "/");
        name = path+1;
    }

    strcpy(lastfile, name);

    dircursor    =  0;
    dirstart     = -1;

    if (strncmp(currdir,lastdir,sizeof(lastdir)))
    {
        dirlevel            =  0;
        dirpos[dirlevel]    = -1;
        cursorpos[dirlevel] =  0;
        
        /* use '/' to calculate dirlevel */
        for (i=1; i<strlen(path)+1; i++)
        {
            if (path[i] == '/')
            {
                dirlevel++;
                dirpos[dirlevel]    = -1;
                cursorpos[dirlevel] =  0;
            }
        }
    }
}

#ifdef HAVE_RECORDER_KEYPAD
bool pageupdown(int* ds, int* dc, int numentries, int tree_max_on_screen )
{
    bool exit = false;
    bool used = false;
    
    int dirstart = *ds;
    int dircursor = *dc;

    while (!exit) {
        switch (button_get(true)) {
            case BUTTON_UP:
            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_UP | BUTTON_REPEAT:
                used = true;
                if ( dirstart ) {
                    dirstart -= tree_max_on_screen;
                    if ( dirstart < 0 )
                        dirstart = 0;
                }
                else
                    dircursor = 0;
                break;
                
            case BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REPEAT:
                used = true;
                if ( dirstart < numentries - tree_max_on_screen ) {
                    dirstart += tree_max_on_screen;
                    if ( dirstart > 
                         numentries - tree_max_on_screen )
                        dirstart = numentries - tree_max_on_screen;
                }
                else
                    dircursor = numentries - dirstart - 1;
                break;
                
#ifdef SIMULATOR
            case BUTTON_ON:
#else
            case BUTTON_ON | BUTTON_REL:
            case BUTTON_ON | BUTTON_UP | BUTTON_REL:
            case BUTTON_ON | BUTTON_DOWN | BUTTON_REL:
#endif
                exit = true;
                break;
        }
        if ( used ) {
            showdir(currdir, dirstart);
            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
            lcd_update();
        }
    }
    *ds = dirstart;
    *dc = dircursor;

    return used;
}
#endif

static void storefile(char* filename, char* setting, int maxlen)
{
    int len = strlen(filename);
    int extlen = 0;
    char* ptr = filename + len;

    while (*ptr != '.') {
        extlen++;
        ptr--;
    }

    if (strcmp(ROCKBOX_DIR, currdir) || (len-extlen > maxlen))
        return;

    strncpy(setting, filename, len-extlen);
    setting[len-extlen]=0;

    settings_save();
}

bool dirbrowse(char *root)
{
    int numentries=0;
    char buf[MAX_PATH];
    int i;
    int lasti=-1;
    int button;
    int tree_max_on_screen;
    bool reload_root = false;
    int lastfilter = global_settings.dirfilter;
    bool lastsortcase = global_settings.sort_case;
#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    lcd_getstringsize("A", &fw, &fh);
    tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#else
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif

    dircursor=0;
    dirstart=0;
    dirlevel=0;

    memcpy(currdir,root,sizeof(currdir));

    start_resume();

    numentries = showdir(currdir, dirstart);
    if (numentries == -1) 
        return -1;  /* currdir is not a directory */

    put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);

    while(1) {
        struct entry* file = &dircache[dircursor+dirstart];

        bool restore = false;
        bool need_update = false;

        button = button_get_w_tmo(HZ/5);
        switch ( button ) {
            case TREE_EXIT:
            case TREE_EXIT | BUTTON_REPEAT:
                i=strlen(currdir);
                if (i>1) {
                    while (currdir[i-1]!='/')
                        i--;
                    strcpy(buf,&currdir[i]);
                    if (i==1)
                        currdir[i]=0;
                    else
                        currdir[i-1]=0;

                    dirlevel--;
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                        dirstart = dirpos[dirlevel];
                        dircursor = cursorpos[dirlevel];
                    }
                    else
                        dirstart = dircursor = 0;

                    if (dirstart == -1)
                        strcpy(lastfile, buf);

                    restore = true;
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                status_draw();
                restore = true;
                break;

            case BUTTON_OFF | BUTTON_REL:
#else
            case BUTTON_STOP | BUTTON_REL:
#endif
                settings_save();
                break;
                

            case TREE_ENTER:
            case TREE_ENTER | BUTTON_REPEAT:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_PLAY:
            case BUTTON_PLAY | BUTTON_REPEAT: 
#endif
                if ( !numentries )
                    break;
                if ((currdir[0]=='/') && (currdir[1]==0)) {
                    snprintf(buf,sizeof(buf),"%s%s",currdir, file->name);
                } else {
                    snprintf(buf,sizeof(buf),"%s/%s",currdir, file->name);
                }

                if (file->attr & ATTR_DIRECTORY) {
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                        dirpos[dirlevel] = dirstart;
                        cursorpos[dirlevel] = dircursor;
                    }
                    dirlevel++;
                    dircursor=0;
                    dirstart=0;
                } else {
                    int seed = current_tick;
                    bool play = false;
                    int start_index=0;

                    lcd_stop_scroll();
                    switch ( file->attr & TREE_ATTR_MASK ) {
                        case TREE_ATTR_M3U:
                            if ( global_settings.resume )
                                snprintf(global_settings.resume_file,
                                         MAX_PATH, "%s/%s",
                                         currdir, file->name);
                            play_list(currdir, file->name, 0, false, 0,
                                      seed, 0);
                            start_index = 0;
                            play = true;
                            break;

                        case TREE_ATTR_MPA:
                            if ( global_settings.resume )
                                strncpy(global_settings.resume_file,
                                        currdir, MAX_PATH);
                            start_index = build_playlist(dircursor+dirstart);

                            /* it is important that we get back the index in
                               the (shuffled) list and stor that */
                            start_index = play_list(currdir, NULL,
                                                    start_index, false,
                                                    0, seed, 0);
                            play = true;
                            break;

                            /* wps config file */
                        case TREE_ATTR_WPS:
                            snprintf(buf, sizeof buf, "%s/%s", 
                                     currdir, file->name);
                            wps_load(buf,true);
                            storefile(file->name, global_settings.wps_file,
                                      MAX_FILENAME);
                            restore = true;
                            break;

                        case TREE_ATTR_CFG:
                            snprintf(buf, sizeof buf, "%s/%s",
                                     currdir, file->name);
                            settings_load_config(buf);
                            restore = true;
                            break;

                        case TREE_ATTR_TXT:
                            snprintf(buf, sizeof buf, "%s/%s",
                                     currdir, file->name);
                            viewer_run(buf);
                            restore = true;
                            break;

                        case TREE_ATTR_LNG:
                            snprintf(buf, sizeof buf, "%s/%s",
                                     currdir, file->name);
                            if(!lang_load(buf)) {
                                storefile(file->name,
                                          global_settings.lang_file,
                                          MAX_FILENAME);
                                          
                                lcd_clear_display();
#ifdef HAVE_LCD_CHARCELLS
                                lcd_puts(0, 0, str(LANG_LANGUAGE_LOADED));
#else
                                lcd_getstringsize(str(LANG_LANGUAGE_LOADED),
                                                  &fw, &fh);
                                if(fw>LCD_WIDTH)
                                    fw=0;
                                else
                                    fw=LCD_WIDTH/2 - fw/2;
                                
                                lcd_putsxy(fw, LCD_HEIGHT/2 - fh/2,
                                           str(LANG_LANGUAGE_LOADED));
#endif
                                lcd_update();
                                sleep(HZ);            
                                restore = true;
                            }
                            break;

#ifdef HAVE_LCD_BITMAP
                        case TREE_ATTR_FONT:
                            snprintf(buf, sizeof buf, "%s/%s",
                                     currdir, file->name);
                            font_load(buf);
                            storefile(file->name, global_settings.font_file,
                                      MAX_FILENAME);

                            lcd_getstringsize("A", &fw, &fh);
                            tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
                            /* make sure cursor is on screen */
                            while ( dircursor > tree_max_on_screen ) {
                                dircursor--;
                                dirstart++;
                            }
                            restore = true;
                            break;
#endif

#ifndef SIMULATOR
                            /* firmware file */
                        case TREE_ATTR_MOD:
                            snprintf(buf, sizeof buf, "%s/%s", 
                                     currdir, file->name);
                            rolo_load(buf);
                            break;
#endif
                    }

                    if ( play ) {
                        if ( global_settings.resume ) {
                            /* the resume_index must always be the index in the
                               shuffled list in case shuffle is enabled */
                            global_settings.resume_index = start_index;
                            global_settings.resume_offset = 0;
                            global_settings.resume_first_index =
                                playlist_first_index();
                            global_settings.resume_seed = seed;
                            settings_save();
                        }

                        status_set_playmode(STATUS_PLAY);
                        status_draw();
                        lcd_stop_scroll();
                        if ( wps_show() == SYS_USB_CONNECTED ) {
                            reload_root = true;
                        }
#ifdef HAVE_LCD_BITMAP
                        tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#endif
                    }
                }
                restore = true;
                break;

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
            case BUTTON_VOL_UP:
                if(filesindir) {
                    if(dircursor) {
                        put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, false);
                        dircursor--;
                        put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                    }
                    else {
                        if (dirstart) {
                            dirstart--;
                            numentries = showdir(currdir, dirstart);
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        }
                        else {
                            if (numentries < tree_max_on_screen) {
                                put_cursorxy(CURSOR_X, CURSOR_Y + dircursor,
                                             false);
                                dircursor = numentries - 1;
                                put_cursorxy(CURSOR_X, CURSOR_Y + dircursor,
                                             true);
                            }
                            else {
                                dirstart = numentries - tree_max_on_screen;
                                dircursor = tree_max_on_screen - 1;
                                numentries = showdir(currdir, dirstart);
                                put_cursorxy(CURSOR_X, CURSOR_Y +
                                             tree_max_on_screen - 1, true);
                            }
                        }
                    }
                    need_update = true;
                }
                break;

            case TREE_NEXT:
            case TREE_NEXT | BUTTON_REPEAT:
            case BUTTON_VOL_DOWN:
                if(filesindir)
                {
                    if (dircursor + dirstart + 1 < numentries ) {
                        if(dircursor+1 < tree_max_on_screen) {
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor,
                                         false);
                            dircursor++;
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            dirstart++;
                            numentries = showdir(currdir, dirstart);
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        }
                    }
                    else {
                        if(numentries < tree_max_on_screen) {
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor,
                                         false);
                            dirstart = dircursor = 0;
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            dirstart = dircursor = 0;
                            numentries = showdir(currdir, dirstart);
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        }
                    }
                    need_update = true;
                }
                break;

            case TREE_MENU:
                lcd_stop_scroll();
                if (main_menu())
                    reload_root = true;
                restore = true;
                break;

            case BUTTON_ON:
#ifdef HAVE_RECORDER_KEYPAD
                if (pageupdown(&dirstart, &dircursor, numentries, 
                               tree_max_on_screen))
                {
                    /* start scroll */
                    restore = true;
                }
                else
#endif
                {
                    if (mpeg_status() & MPEG_STATUS_PLAY)
                    {
                        lcd_stop_scroll();
                        if (wps_show() == SYS_USB_CONNECTED)
                            reload_root = true;
#ifdef HAVE_LCD_BITMAP
                        tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#endif
                        restore = true;
                    }
                }
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F2:
                if (f2_screen())
                    reload_root = true;
                restore = true;
                break;

            case BUTTON_F3:
                if (f3_screen())
                    reload_root = true;

#ifdef HAVE_LCD_BITMAP
                tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#endif
                restore = true;
                break;
#endif

            case SYS_USB_CONNECTED:
                status_set_playmode(STATUS_STOP);
                usb_screen();
                reload_root = true;
                break;

            case BUTTON_NONE:
                status_draw();
                break;
        }

        if ( button )
            ata_spin();
        
        /* do we need to rescan dir? */
        if (reload_root ||
            lastfilter != global_settings.dirfilter ||
            lastsortcase != global_settings.sort_case)
        {
            if ( reload_root ) {
                strcpy(currdir, "/");
                dirlevel = 0;
                reload_root = false;
            }
            dircursor = 0;
            dirstart = 0;
            lastdir[0] = 0;
            lastfilter = global_settings.dirfilter;
            lastsortcase = global_settings.sort_case;
            restore = true;
        }

        if ( restore ) {
            /* restore display */
            /* We need to adjust if the number of lines on screen have
               changed because of a status bar change */
            if(CURSOR_Y+LINE_Y+dircursor>tree_max_on_screen) {
                dirstart++;
                dircursor--;
            }
#ifdef HAVE_LCD_BITMAP
            /* the sub-screen might've ruined the margins */
            lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and
                                                  icon */
#endif
            numentries = showdir(currdir, dirstart);
            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
            
            need_update = true;
        }

        if ( numentries ) {
            i = dirstart+dircursor;

            /* if MP3 filter is on, cut off the extension */
            if(lasti!=i || restore) {
                lasti=i;
                lcd_stop_scroll();
                if (global_settings.dirfilter == SHOW_MUSIC && 
                    (dircache[i].attr & (TREE_ATTR_M3U|TREE_ATTR_MPA)))
                {
                    int len = strlen(dircache[i].name);
                    char temp = dircache[i].name[len-4];
                    dircache[i].name[len-4] = 0;
                    lcd_puts_scroll(LINE_X, dircursor, dircache[i].name);
                    dircache[i].name[len-4] = temp;
                }
                else
                    lcd_puts_scroll(LINE_X, dircursor, dircache[i].name);

                need_update = true;
            }
        }

        if(need_update) {
            lcd_update();

            need_update = false;
        }
    }

    return false;
}

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../firmware/rockbox-mode.el")
 * end:
 */
