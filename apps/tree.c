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
#include "language.h"
#include "screens.h"
#include "keyboard.h"
#include "onplay.h"
#include "buffer.h"
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#define BOOTFILE "ajbrec.ajz"
#else
#define BOOTFILE "archos.mod"
#endif

/* Boot value of global_settings.max_files_in_dir */
static int max_files_in_dir;

static char *name_buffer;
static int name_buffer_size;    /* Size of allocated buffer */
static int name_buffer_length;  /* Currently used amount */

static struct entry *dircache;

static int dircursor;
static int dirstart;
static int dirlevel;
static int filesindir;
static int dirpos[MAX_DIR_LEVELS];
static int cursorpos[MAX_DIR_LEVELS];
static char lastdir[MAX_PATH];
static char lastfile[MAX_PATH];
static char currdir[MAX_PATH];
static char currdir_save[MAX_PATH];
static bool reload_dir = false;
static int boot_size = 0;
static int boot_cluster;
static bool boot_changed = false;

static bool dirbrowse(char *root, int *dirfilter);

void browse_root(void)
{
#ifndef SIMULATOR
    dirbrowse("/", &global_settings.dirfilter);
#else
    if (!dirbrowse("/", &global_settings.dirfilter)) {
        DEBUGF("No filesystem found. Have you forgotten to create it?\n");
    }
#endif
}


#ifdef HAVE_LCD_BITMAP

/* pixel margins */
#define MARGIN_X (global_settings.scrollbar && \
                  filesindir > tree_max_on_screen ? SCROLLBAR_WIDTH : 0) + \
                  CURSOR_WIDTH + (global_settings.show_icons && ICON_WIDTH > 0 ? ICON_WIDTH :0)
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
#define CURSOR_WIDTH  (global_settings.invert_cursor ? 0 : 4)

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

static int build_playlist(int start_index)
{
    int i;
    int start=start_index;

    for(i = 0;i < filesindir;i++)
    {
        if(dircache[i].attr & TREE_ATTR_MPA)
        {
            DEBUGF("Adding %s\n", dircache[i].name);
            if (playlist_add(dircache[i].name) < 0)
                break;
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

static void showfileline(int line, int direntry, bool scroll, int *dirfilter)
{
    char* name = dircache[direntry].name;
    int xpos = LINE_X;

#ifdef HAVE_LCD_CHARCELLS
    if (!global_settings.show_icons)
        xpos--;
#endif

    /* if any file filter is on, strip the extension */
    if (*dirfilter != SHOW_ALL && 
        !(dircache[direntry].attr & ATTR_DIRECTORY))
    {
        char* dotpos = strrchr(name, '.');
        char temp=0;
        if (dotpos) {
            temp = *dotpos;
            *dotpos = 0;
        }
        if(scroll)
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)
                lcd_puts_scroll_style(xpos, line, name, STYLE_INVERT);
            else
#endif
                lcd_puts_scroll(xpos, line, name);
        else
            lcd_puts(xpos, line, name);
        if (temp)
            *dotpos = temp;
    }
    else {
        if(scroll)
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor)
                lcd_puts_scroll_style(xpos, line, name, STYLE_INVERT);
            else
#endif
                lcd_puts_scroll(xpos, line, name);
        else
            lcd_puts(xpos, line, name);
    }
}

/* load sorted directory into dircache.  returns NULL on failure. */
struct entry* load_and_sort_directory(char *dirname, int *dirfilter,
                                      int *num_files, bool *buffer_full)
{
    int i;

    DIR *dir = opendir(dirname);
    if(!dir)
        return NULL; /* not a directory */
    
    name_buffer_length = 0;
    *buffer_full = false;
    
    for ( i=0; i < max_files_in_dir; i++ ) {
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
        if (*dirfilter != SHOW_ALL &&
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
            else if (!strcasecmp(&entry->d_name[len-5], ".rock"))
                dptr->attr |= TREE_ATTR_ROCK;
            else if (!strcasecmp(&entry->d_name[len-4], ".ucl"))
                dptr->attr |= TREE_ATTR_UCL; }
        
        /* memorize/compare details about the boot file */
        if ((currdir[1] == 0) && !strcmp(entry->d_name, BOOTFILE)) {
            if (boot_size) {
                if ((entry->size != boot_size) ||
                    (entry->startcluster != boot_cluster))
                    boot_changed = true;
            }
            boot_size = entry->size;
            boot_cluster = entry->startcluster;
        }

        /* filter out non-visible files */
        if ((*dirfilter == SHOW_PLAYLIST &&
             !(dptr->attr & (ATTR_DIRECTORY|TREE_ATTR_M3U))) ||
            (*dirfilter == SHOW_MUSIC &&
             !(dptr->attr & (ATTR_DIRECTORY|TREE_ATTR_MPA|TREE_ATTR_M3U))) ||
            (*dirfilter == SHOW_SUPPORTED && !(dptr->attr & TREE_ATTR_MASK)) ||
            (*dirfilter == SHOW_WPS && !(dptr->attr & TREE_ATTR_WPS)) ||
            (*dirfilter == SHOW_CFG && !(dptr->attr & TREE_ATTR_CFG)) ||
            (*dirfilter == SHOW_LNG && !(dptr->attr & TREE_ATTR_LNG)) ||
            (*dirfilter == SHOW_MOD && !(dptr->attr & TREE_ATTR_MOD)) ||
            (*dirfilter == SHOW_FONT && !(dptr->attr & TREE_ATTR_FONT)) ||
            (*dirfilter == SHOW_PLUGINS && !(dptr->attr & TREE_ATTR_ROCK)))
        {
            i--;
            continue;
        }
        
        if (len > name_buffer_size - name_buffer_length - 1) {
            /* Tell the world that we ran out of buffer space */
            *buffer_full = true;
            break;
        }
        dptr->name = &name_buffer[name_buffer_length];
        strcpy(dptr->name,entry->d_name);
        name_buffer_length += len + 1;
    }
    *num_files = i;
    closedir(dir);
    strncpy(lastdir,dirname,sizeof(lastdir));
    lastdir[sizeof(lastdir)-1] = 0;
    qsort(dircache,i,sizeof(struct entry),compare);

    return dircache;
}

static int showdir(char *path, int start, int *dirfilter)
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
    if (strncmp(path,lastdir,sizeof(lastdir)) || reload_dir) {
        if (!load_and_sort_directory(path, dirfilter,
                &filesindir, &dir_buffer_full))
            return -1;

        if ( dir_buffer_full || filesindir == max_files_in_dir ) {
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

    /* The cursor might point to an invalid line, for example if someone
       deleted the last file in the dir */
    if(filesindir)
    {
        while(start + dircursor >= filesindir)
        {
            if(start)
                start--;
            else
                if(dircursor)
                    dircursor--;
        }
        dirstart = start;
    }

#ifdef HAVE_LCD_CHARCELLS
    lcd_stop_scroll();
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
            case TREE_ATTR_UCL:
                icon_type = Flashfile;
                break;
#endif
 
            case TREE_ATTR_ROCK:
                icon_type = Plugin;
                break;

#ifdef HAVE_LCD_BITMAP
            case TREE_ATTR_FONT:
                icon_type = Font;
                break;
#endif
            default:
#ifdef HAVE_LCD_BITMAP
                icon_type = 0;
#else
                icon_type = Unknown;
#endif
        }

        if (icon_type && global_settings.show_icons) {
#ifdef HAVE_LCD_BITMAP
            int offset=0;
            if ( line_height > 8 )
                offset = (line_height - 8) / 2;
            lcd_bitmap(bitmap_icons_6x8[icon_type], 
                       CURSOR_X * 6 + CURSOR_WIDTH, 
                       MARGIN_Y+(i-start)*line_height + offset,
                       6, 8, true);
#else
            lcd_putc(LINE_X-1, i-start, icon_type);
#endif
        }

        showfileline(i-start, i, false, dirfilter); /* no scroll */
    }

#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && (filesindir > tree_max_on_screen)) 
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  LCD_HEIGHT - SCROLLBAR_Y, filesindir, start,
                  start + tree_max_on_screen, VERTICAL);
#endif
    status_draw(true);
    return filesindir;
}

static bool ask_resume(bool ask_once)
{
    bool stop = false;
    
#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif

    /* always resume? */
    if ( global_settings.resume == RESUME_ON )
        return true;

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_RESUME_ASK));
#ifdef HAVE_LCD_CHARCELLS
    status_draw(false);
    lcd_puts(0,1,str(LANG_RESUME_CONFIRM_PLAYER));
#else
    lcd_puts(0,1,str(LANG_CONFIRM_WITH_PLAY_RECORDER));
    lcd_puts(0,2,str(LANG_CANCEL_WITH_ANY_RECORDER));
#endif
    lcd_update();

    while (!stop) {
        switch (button_get(true)) {
            case BUTTON_PLAY:
            case BUTTON_RC_PLAY:
                return true;

                /* ignore the ON button, since it might
                   still be pressed since booting */
            case BUTTON_ON:
            case BUTTON_ON | BUTTON_REL:
                break;
                
            case SYS_USB_CONNECTED:
                usb_screen();
                stop = true;
                break;

            default:
                stop = true;
                break;
        }
    }

    if ( global_settings.resume == RESUME_ASK_ONCE && ask_once) {
        global_settings.resume_index = -1;
        settings_save();
    }

    return false;
}

/* load tracks from specified directory to resume play */
void resume_directory(char *dir)
{
    bool buffer_full;

    if (!load_and_sort_directory(dir, &global_settings.dirfilter, &filesindir,
            &buffer_full))
        return;
    lastdir[0] = 0;
    
    build_playlist(0);
}

/* Returns the current working directory and also writes cwd to buf if
   non-NULL.  In case of error, returns NULL. */
char *getcwd(char *buf, int size)
{
    if (!buf)
        return currdir;
    else if (size > 0)
    {
        strncpy(buf, currdir, size);
        return buf;
    }
    else
        return NULL;
}

/* Force a reload of the directory next time directory browser is called */
void reload_directory(void)
{
    reload_dir = true;
}

static void start_resume(bool ask_once)
{
    if ( global_settings.resume &&
         global_settings.resume_index != -1 ) {
        DEBUGF("Resume index %X offset %X\n",
               global_settings.resume_index,
               global_settings.resume_offset);

        if (!ask_resume(ask_once))
            return;
        
        if (playlist_resume() != -1)
        {
            playlist_start(global_settings.resume_index,
                global_settings.resume_offset);

            status_set_playmode(STATUS_PLAY);
            status_draw(true);
            wps_show();
        }
        else
            return;
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

static bool handle_on(int *ds, int *dc, int numentries, int tree_max_on_screen, int *dirfilter)
{
    bool exit = false;
    bool used = false;

    int dirstart = *ds;
    int dircursor = *dc;
    char buf[MAX_PATH];

#ifdef HAVE_LCD_BITMAP
    int fw, fh;
    lcd_getstringsize("A", &fw, &fh);
#endif

    while (!exit) {
        switch (button_get(true)) {
            case TREE_PREV:
            case BUTTON_RC_LEFT:
            case BUTTON_ON | TREE_PREV:
            case BUTTON_ON | TREE_PREV | BUTTON_REPEAT:
                used = true;
                if ( dirstart ) {
                    dirstart -= tree_max_on_screen;
                    if ( dirstart < 0 )
                        dirstart = 0;
                }
                else
                    dircursor = 0;
                break;
                
            case TREE_NEXT:
            case BUTTON_RC_RIGHT:
            case BUTTON_ON | TREE_NEXT:
            case BUTTON_ON | TREE_NEXT | BUTTON_REPEAT:
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


            case BUTTON_PLAY:
            case BUTTON_RC_PLAY:
            case BUTTON_ON | BUTTON_PLAY: {
                int onplay_result;

                if (currdir[1])
                    snprintf(buf, sizeof buf, "%s/%s",
                             currdir, dircache[dircursor+dirstart].name);
                else
                    snprintf(buf, sizeof buf, "/%s",
                             dircache[dircursor+dirstart].name);
                onplay_result = onplay(buf,
                    dircache[dircursor+dirstart].attr);
                switch (onplay_result)
                {
                    case ONPLAY_OK:
                        used = true;
                        break;
                    case ONPLAY_RELOAD_DIR:
                        reload_dir = 1;
                        used = true;
                        break;
                    case ONPLAY_START_PLAY:
                        used = false; /* this will enable the wps */
                        break;
                }
                exit = true;
                break;
            }    
            case BUTTON_ON | BUTTON_REL:
            case BUTTON_ON | TREE_PREV | BUTTON_REL:
            case BUTTON_ON | TREE_NEXT | BUTTON_REL:
                exit = true;
                break;
        }
        if ( used && !exit ) {
#ifdef HAVE_LCD_BITMAP
            int xpos,ypos;
#endif
            showdir(currdir, dirstart, dirfilter);
#ifdef HAVE_LCD_BITMAP
            if (global_settings.invert_cursor) {
                xpos = lcd_getxmargin();
                ypos = (CURSOR_Y + dircursor) * fh + lcd_getymargin();
                lcd_invertrect(xpos, ypos, LCD_WIDTH-xpos, fh);
            }
            else
#endif
                put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
            lcd_update();
        }
    }
    *ds = dirstart;
    *dc = dircursor;

    return used;
}

static bool dirbrowse(char *root, int *dirfilter)
{
    int numentries=0;
    char buf[MAX_PATH];
    int i;
    int lasti=-1;
    int button;
    int tree_max_on_screen;
    bool reload_root = false;
    int lastfilter = *dirfilter;
    bool lastsortcase = global_settings.sort_case;
    int lastdircursor=-1;
    bool need_update = true;
    bool exit_func = false;
    bool update_all = false; /* set this to true when the whole file list
                                has been refreshed on screen */

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

    if (*dirfilter < NUM_FILTER_MODES)
    start_resume(true);

    numentries = showdir(currdir, dirstart, dirfilter);
    if (numentries == -1) 
        return false;  /* currdir is not a directory */
    update_all = true;

    put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);

    while(1) {
        struct entry* file = &dircache[dircursor+dirstart];

        bool restore = false;

        button = button_get_w_tmo(HZ/5);

#ifndef SIMULATOR
        if (boot_changed) {
            lcd_clear_display();
            lcd_puts(0,0,str(LANG_BOOT_CHANGED));
            lcd_puts(0,1,str(LANG_REBOOT_NOW));
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0,3,str(LANG_CONFIRM_WITH_PLAY_RECORDER));
            lcd_puts(0,4,str(LANG_CANCEL_WITH_ANY_RECORDER));
            lcd_update();
#endif
            if (button_get(true) == BUTTON_PLAY)
                rolo_load("/" BOOTFILE);
            restore = true;
            boot_changed = false;
        }
#endif

        switch ( button ) {
            case TREE_EXIT:
            case BUTTON_RC_STOP:
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
                if (*dirfilter > NUM_FILTER_MODES)
                    exit_func = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                status_draw(false);
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
            case BUTTON_RC_PLAY:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_PLAY:
            case BUTTON_PLAY | BUTTON_REPEAT: 
#endif
                if ( !numentries )
                    break;
                if (currdir[1])
                    snprintf(buf,sizeof(buf),"%s/%s",currdir, file->name);
                else
                    snprintf(buf,sizeof(buf),"/%s",file->name);

                if (file->attr & ATTR_DIRECTORY) {
                    memcpy(currdir,buf,sizeof(currdir));
                    if ( dirlevel < MAX_DIR_LEVELS ) {
                        dirpos[dirlevel] = dirstart;
                        cursorpos[dirlevel] = dircursor;
                    }
                    dirlevel++;
                    dircursor=0;
                    dirstart=0;
                }
                else {
                    int seed = current_tick;
                    bool play = false;
                    int start_index=0;

                    lcd_stop_scroll();
                    switch ( file->attr & TREE_ATTR_MASK ) {
                        case TREE_ATTR_M3U:
                            if (playlist_create(currdir, file->name) != -1)
                            {
                                if (global_settings.playlist_shuffle)
                                    playlist_shuffle(seed, -1);
                                start_index = 0;
                                playlist_start(start_index,0);
                                play = true;
                            }
                            break;
                        
                        case TREE_ATTR_MPA:
                            if (playlist_create(currdir, NULL) != -1)
                            {
                                start_index =
                                    build_playlist(dircursor+dirstart);
                                if (global_settings.playlist_shuffle)
                                {
                                    start_index =
                                        playlist_shuffle(seed,start_index);
                                    
                                    /* when shuffling dir.: play all files
                                       even if the file selected by user is
                                       not the first one */
                                    if (!global_settings.play_selected)
                                        start_index = 0;
                                }
                                
                                playlist_start(start_index, 0);
                                play = true;
                            }
                            break;

                            /* wps config file */
                        case TREE_ATTR_WPS:
                            wps_load(buf,true);
                            set_file(buf, global_settings.wps_file,
                                     MAX_FILENAME);
                            restore = true;
                            break;

                        case TREE_ATTR_CFG:
                            if (!settings_load_config(buf))
                                break;
                            lcd_clear_display();
                            lcd_puts(0,0,str(LANG_SETTINGS_LOADED1));
                            lcd_puts(0,1,str(LANG_SETTINGS_LOADED2));
#ifdef HAVE_LCD_BITMAP
                            lcd_update();

                            /* maybe we have a new font */
                            lcd_getstringsize("A", &fw, &fh);
                            tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
                            /* make sure cursor is on screen */
                            while ( dircursor > tree_max_on_screen ) 
                            {
                                dircursor--;
                                dirstart++;
                            }
#endif
                            sleep(HZ/2);
                            restore = true;
                            break;

                        case TREE_ATTR_TXT:
                            plugin_load("/.rockbox/rocks/viewer.rock",buf);
                            restore = true;
                            break;

                        case TREE_ATTR_LNG:
                            if(!lang_load(buf)) {
                                set_file(buf, global_settings.lang_file,
                                         MAX_FILENAME);
                                splash(HZ, 0, true, str(LANG_LANGUAGE_LOADED));
                                restore = true;
                            }
                            break;

#ifdef HAVE_LCD_BITMAP
                        case TREE_ATTR_FONT:
                            font_load(buf);
                            set_file(buf, global_settings.font_file,
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
                            rolo_load(buf);
                            break;

                            /* ucl flash file */
                        case TREE_ATTR_UCL:
                            plugin_load("/.rockbox/rocks/rockbox_flash.rock",buf);
                            break; 
#endif

                            /* plugin file */
                        case TREE_ATTR_ROCK:
                            if (plugin_load(buf,NULL) == PLUGIN_USB_CONNECTED)
                                reload_root = true;
                            else
                                restore = true;
                            break;
                    }

                    if ( play ) {
                        if ( global_settings.resume ) {
                            /* the resume_index must always be the index in the
                               shuffled list in case shuffle is enabled */
                            global_settings.resume_index = start_index;
                            global_settings.resume_offset = 0;
                            settings_save();
                        }

                        status_set_playmode(STATUS_PLAY);
                        status_draw(false);
                        lcd_stop_scroll();
                        if ( wps_show() == SYS_USB_CONNECTED ) {
                            reload_root = true;
                        }
#ifdef HAVE_LCD_BITMAP
                        tree_max_on_screen = (LCD_HEIGHT - MARGIN_Y) / fh;
#endif
                    }
                    else if (*dirfilter > NUM_FILTER_MODES)
                        exit_func = true;
                }
                restore = true;
                break;

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
            case BUTTON_RC_LEFT:
                if(filesindir) {
                    if(dircursor) {
                        put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, false);
                        dircursor--;
                        put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                    }
                    else {
                        if (dirstart) {
                            dirstart--;
                            numentries = showdir(currdir, dirstart, dirfilter);
                            update_all=true;
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
                                numentries = showdir(currdir, dirstart, dirfilter);
                                update_all = true;
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
            case BUTTON_RC_RIGHT:
                if(filesindir)
                {
                    if (dircursor + dirstart + 1 < numentries ) {
                        if(dircursor+1 < tree_max_on_screen) {
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, false);
                            dircursor++;
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            dirstart++;
                            numentries = showdir(currdir, dirstart, dirfilter);
                            update_all = true;
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        }
                    }
                    else {
                        if(numentries < tree_max_on_screen) {
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, false);
                            dirstart = dircursor = 0;
                            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
                        } 
                        else {
                            dirstart = dircursor = 0;
                            numentries = showdir(currdir, dirstart, dirfilter);
                            update_all=true;
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
                if (handle_on(&dirstart, &dircursor, numentries,
                              tree_max_on_screen, dirfilter))
                {
                    /* start scroll */
                    restore = true;
                }
                else
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
                    else
                    {
                        start_resume(false);
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
                status_draw(false);
                break;
        }

        if ( button )
            ata_spin();
        
        /* do we need to rescan dir? */
        if (reload_dir || reload_root ||
            lastfilter != *dirfilter ||
            lastsortcase != global_settings.sort_case)
        {
            if ( reload_root ) {
                strcpy(currdir, "/");
                dirlevel = 0;
                reload_root = false;
            }
            if (! reload_dir )
            {
                dircursor = 0;
                dirstart = 0;
                lastdir[0] = 0;
            }

            lastfilter = *dirfilter;
            lastsortcase = global_settings.sort_case;
            restore = true;
        }

        if (exit_func)
            break;

        if (restore || reload_dir) {
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
            lcd_setfont(FONT_UI);
#endif
            numentries = showdir(currdir, dirstart, dirfilter);
            update_all = true;
            put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);
            
            need_update = true;
            reload_dir = false;
        }

        if ( numentries && need_update) {
            i = dirstart+dircursor;

            /* if MP3 filter is on, cut off the extension */
            if(lasti!=i || restore) {
                lcd_stop_scroll();

                /* So if lastdircursor and dircursor differ, and then full
                   screen was not refreshed, restore the previous line */
                if ((lastdircursor != dircursor) && !update_all ) {
                    showfileline(lastdircursor, lasti, false, dirfilter); /* no scroll */
                }
                lasti=i;
                lastdircursor=dircursor;

                showfileline(dircursor, i, true, dirfilter); /* scroll please */
                need_update = true;
            }
        }

        if(need_update) {
            lcd_update();

            need_update = false;
            update_all = false;
        }
    }

    return true;
}

static int plsize = 0;
static bool add_dir(char* dirname, int fd)
{
    bool abort = false;
    char buf[MAX_PATH/2]; /* saving a little stack... */
    DIR* dir;
    
    /* check for user abort */
#ifdef HAVE_PLAYER_KEYPAD
    if (button_get(false) == BUTTON_STOP)
#else
    if (button_get(false) == BUTTON_OFF)
#endif
        return true;
    
    dir = opendir(dirname);
    if(!dir)
        return true;

    while (true) {
        struct dirent *entry;

        entry = readdir(dir);
        if (!entry)
            break;
        if (entry->attribute & ATTR_DIRECTORY) {
            if (!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, ".."))
                continue;
            if (dirname[1])
                snprintf(buf, sizeof buf, "%s/%s", dirname, entry->d_name);
            else
                snprintf(buf, sizeof buf, "/%s", entry->d_name);
            if (add_dir(buf,fd)) {
                abort = true;
                break;
            }
        }
        else {
            int x = strlen(entry->d_name);
            if ((!strcasecmp(&entry->d_name[x-4], ".mp3")) ||
                (!strcasecmp(&entry->d_name[x-4], ".mp2")) ||
                (!strcasecmp(&entry->d_name[x-4], ".mpa")))
            {
                write(fd, dirname, strlen(dirname));
                write(fd, "/", 1);
                write(fd, entry->d_name, x);
                write(fd, "\n", 1);

                plsize++;
                snprintf(buf, sizeof buf, "%d", plsize);
#ifdef HAVE_LCD_BITMAP
                lcd_puts(0,4,buf);
                lcd_update();
#else
                x = 10;
                if (plsize > 999)
                    x=7;
                else {
                    if (plsize > 99)
                        x=8;
                    else {
                        if (plsize > 9)
                            x=9;
                    }
                }
                lcd_puts(x,0,buf);
#endif
            }
        }
    }
    closedir(dir);

    return abort;
}

bool create_playlist(void)
{
    int fd;
    char filename[MAX_PATH];

    snprintf(filename, sizeof filename, "%s.m3u",
             currdir[1] ? currdir : "/root");

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_CREATING));
    lcd_puts_scroll(0,1,filename);
    lcd_update();

    fd = creat(filename,0);
    if (fd < 0)
        return false;

    plsize = 0;
    add_dir(currdir[1] ? currdir : "/", fd);
    close(fd);
    sleep(HZ);
    
    return true;
}

bool rockbox_browse(char *root, int dirfilter)
{
    bool rc;
    int dircursor_save = dircursor;
    int dirstart_save = dirstart;
    int dirlevel_save = dirlevel;
    int dirpos_save = dirpos[0];
    int cursorpos_save = cursorpos[0];

    memcpy(currdir_save, currdir, sizeof(currdir));
    rc = dirbrowse(root, &dirfilter);
    memcpy(currdir, currdir_save, sizeof(currdir));

    reload_dir = true;
    dirstart = dirstart_save;
    cursorpos[0] = cursorpos_save;
    dirlevel = dirlevel_save;
    dircursor = dircursor_save;
    dirpos[0] = dirpos_save;

    return false;
}

void tree_init(void)
{
    /* We copy the settings value in case it is changed by the user. We can't
       use it until the next reboot. */
    max_files_in_dir = global_settings.max_files_in_dir;
    name_buffer_size = AVERAGE_FILENAME_LENGTH * max_files_in_dir;
    
    name_buffer = buffer_alloc(name_buffer_size);
    dircache = buffer_alloc(max_files_in_dir * sizeof(struct entry));
}
