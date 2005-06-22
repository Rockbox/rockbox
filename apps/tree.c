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
#include "audio.h"
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
#include "bookmark.h"
#include "onplay.h"
#include "buffer.h"
#include "plugin.h"
#include "power.h"
#include "action.h"
#include "talk.h"
#include "filetypes.h"
#include "misc.h"
#include "filetree.h"
#include "dbtree.h"
#include "recorder/recording.h"
#include "rtc.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

/* a table for the know file types */
const struct filetype filetypes[] = {
    { ".mp3", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".mp2", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".mpa", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
#if CONFIG_HWCODEC == MASNONE
    /* Temporary hack to allow playlist creation */
    { ".mp1", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".ogg", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".wma", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".wav", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".flac", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".ac3", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".a52", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".mpc", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".wv",  TREE_ATTR_MPA, File, VOICE_EXT_MPA },
#endif
    { ".m3u", TREE_ATTR_M3U, Playlist, LANG_PLAYLIST },
    { ".cfg", TREE_ATTR_CFG, Config, VOICE_EXT_CFG },
    { ".wps", TREE_ATTR_WPS, Wps, VOICE_EXT_WPS },
    { ".lng", TREE_ATTR_LNG, Language, LANG_LANGUAGE },
    { ".rock",TREE_ATTR_ROCK,Plugin, VOICE_EXT_ROCK },
#ifdef HAVE_LCD_BITMAP
    { ".fnt", TREE_ATTR_FONT,Font, VOICE_EXT_FONT },
#endif
    { ".bmark",TREE_ATTR_BMARK, Bookmark, VOICE_EXT_BMARK },
#ifdef BOOTFILE_EXT
    { BOOTFILE_EXT, TREE_ATTR_MOD, Mod_Ajz, VOICE_EXT_AJZ },
#endif /* #ifndef SIMULATOR */
};

static struct tree_context tc;

bool boot_changed = false;

char lastfile[MAX_PATH];
static char lastdir[MAX_PATH];
static int lasttable, lastextra, lastfirstpos;
static int max_files = 0;

static bool reload_dir = false;

static bool start_wps = false;
static bool dirbrowse(void);

bool check_rockboxdir(void)
{
    DIR *dir = opendir(ROCKBOX_DIR);
    if(!dir)
    {
        lcd_clear_display();
        splash(HZ*2, true, str(LANG_NO_ROCKBOX_DIR));
        lcd_clear_display();
        splash(HZ*2, true, str(LANG_INSTALLATION_INCOMPLETE));
        return false;
    }
    closedir(dir);
    return true;
}

void browse_root(void)
{
    filetype_init();
    check_rockboxdir();

    strcpy(tc.currdir, "/");
    
#ifndef SIMULATOR
    dirbrowse();

#else
    if (!dirbrowse()) {
        DEBUGF("No filesystem found. Have you forgotten to create it?\n");
    }
#endif
}

void tree_get_filetypes(const struct filetype** types, int* count)
{
    *types = filetypes;
    *count = sizeof(filetypes) / sizeof(*filetypes);
}

struct tree_context* tree_get_context(void)
{
    return &tc;
}

#ifdef HAVE_LCD_BITMAP

/* pixel margins */
#define MARGIN_X (global_settings.scrollbar && \
                  tc.filesindir > tree_max_on_screen ? SCROLLBAR_WIDTH : 0) + \
                  CURSOR_WIDTH + (global_settings.show_icons && ICON_WIDTH > 0 ? ICON_WIDTH :0)
#define MARGIN_Y (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

/* position the entry-list starts at */
#define LINE_X   0
#define LINE_Y   (global_settings.statusbar ? 1 : 0)

#define CURSOR_X (global_settings.scrollbar && \
                  tc.filesindir > tree_max_on_screen ? 1 : 0)
#define CURSOR_Y 0 /* the cursor is not positioned in regard to
                      the margins, so this is the amount of lines
                      we add to the cursor Y position to position
                      it on a line */
#define CURSOR_WIDTH  (global_settings.invert_cursor ? 0 : 4)

#define ICON_WIDTH    6

#define SCROLLBAR_X      0
#define SCROLLBAR_Y      lcd_getymargin()
#define SCROLLBAR_WIDTH  6

#else /* HAVE_LCD_BITMAP */

#define TREE_MAX_ON_SCREEN   2
#define TREE_MAX_LEN_DISPLAY 11 /* max length that fits on screen */
#define LINE_X      2 /* X position the entry-list starts at */
#define LINE_Y      0 /* Y position the entry-list starts at */

#define CURSOR_X    0
#define CURSOR_Y    0 /* not really used for players */

#endif /* HAVE_LCD_BITMAP */

/* talkbox hovering delay, to avoid immediate disk activity */
#define HOVER_DELAY (HZ/2)

static void showfileline(int line, char* name, int attr, bool scroll)
{
    int xpos = LINE_X;
    char* dotpos = NULL;

#ifdef HAVE_LCD_CHARCELLS
    if (!global_settings.show_icons)
        xpos--;
#endif

    /* if any file filter is on, strip the extension */
    if (*tc.dirfilter != SHOW_ID3DB &&
        *tc.dirfilter != SHOW_ALL &&
        !(attr & ATTR_DIRECTORY))
    {
        dotpos = strrchr(name, '.');
        if (dotpos) {
            *dotpos = 0;
        }
    }
    
    if(scroll) {
#ifdef HAVE_LCD_BITMAP
        lcd_setfont(FONT_UI);
        if (global_settings.invert_cursor)
            lcd_puts_scroll_style(xpos, line, name, STYLE_INVERT);
        else
#endif
            lcd_puts_scroll(xpos, line, name);
    } else
        lcd_puts(xpos, line, name);

    /* Restore the dot before the extension if it was removed */
    if (dotpos)
        *dotpos = '.';
}

#ifdef HAVE_LCD_BITMAP
static int recalc_screen_height(void)
{
    int fw, fh;
    int height = LCD_HEIGHT;

    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    if(global_settings.statusbar)
        height -= STATUSBAR_HEIGHT;

#if CONFIG_KEYPAD == RECORDER_PAD
    if(global_settings.buttonbar)
        height -= BUTTONBAR_HEIGHT;
#endif        

    return height / fh;
}
#endif

static int showdir(void)
{
    struct entry *dircache = tc.dircache;
    int i;
    int tree_max_on_screen;
    int start = tc.dirstart;
    bool id3db = *tc.dirfilter == SHOW_ID3DB;
    bool newdir = false;
#ifdef HAVE_LCD_BITMAP
    const char* icon;
    int line_height;
    int fw, fh;
    lcd_setfont(FONT_UI);
    lcd_getstringsize("A", &fw, &fh);
    tree_max_on_screen = recalc_screen_height();
    line_height = fh;
#else
    int icon;
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif

    /* new file dir? load it */
    if (id3db) {
        if (tc.currtable != lasttable ||
            tc.currextra != lastextra ||
            tc.firstpos  != lastfirstpos)
        {
            if (db_load(&tc) < 0)
                return -1;
            lasttable = tc.currtable;
            lastextra = tc.currextra;
            lastfirstpos = tc.firstpos;
            newdir = true;
        }
    }
    else {
        if (strncmp(tc.currdir, lastdir, sizeof(lastdir)) || reload_dir) {
            if (ft_load(&tc, NULL) < 0)     
                return -1;
            strcpy(lastdir, tc.currdir);
            newdir = true;
        }
    }

    if (newdir && !id3db &&
        (tc.dirfull || tc.filesindir == global_settings.max_files_in_dir) )
    {
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

    if (start == -1)
    {
        int diff_files;

        /* use lastfile to determine start (default=0) */
        start = 0;

        for (i=0; i < tc.filesindir; i++)
        {
            struct entry *dircache = tc.dircache;

            if (!strcasecmp(dircache[i].name, lastfile))
            {
                start = i;
                break;
            }
        }

        diff_files = tc.filesindir - start;
        if (diff_files < tree_max_on_screen)
        {
            int oldstart = start;

            start -= (tree_max_on_screen - diff_files);
            if (start < 0)
                start = 0;

            tc.dircursor = oldstart - start;
        }

        tc.dirstart = start;
    }

    /* The cursor might point to an invalid line, for example if someone
       deleted the last file in the dir */
    if (tc.filesindir)
    {
        while (start + tc.dircursor >= tc.filesindir)
        {
            if (start)
                start--;
            else
                if (tc.dircursor)
                    tc.dircursor--;
        }
        tc.dirstart = start;
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


    for ( i=start; i < start+tree_max_on_screen && i < tc.filesindir; i++ ) {
        int line = i - start;
        char* name;
        int attr = 0;

        if (id3db) {
            name = ((char**)tc.dircache)[i * tc.dentry_size];
            icon = db_get_icon(&tc);
        }
        else { 
            struct entry* dc = tc.dircache;
            struct entry* e = &dc[i];
            name = e->name;
            attr = e->attr;
            icon = filetype_get_icon(dircache[i].attr);
        }
            

        if (icon && global_settings.show_icons) {
#ifdef HAVE_LCD_BITMAP
            int offset=0;
            if ( line_height > 8 )
                offset = (line_height - 8) / 2;
            lcd_bitmap(icon,
                       CURSOR_X * 6 + CURSOR_WIDTH,
                       MARGIN_Y+(i-start)*line_height + offset,
                       6, 8, true);
#else
            if (icon < 0 )
                icon = Unknown;
            lcd_putc(LINE_X-1, i-start, icon);
#endif
        }

        showfileline(line, name, attr, false); /* no scroll */
    }

#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && (tc.dirlength > tree_max_on_screen))
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  tree_max_on_screen * line_height, tc.dirlength,
                  start + tc.firstpos,
                  start + tc.firstpos + tree_max_on_screen, VERTICAL);

#if CONFIG_KEYPAD == RECORDER_PAD
    if (global_settings.buttonbar) {
        if (*tc.dirfilter < NUM_FILTER_MODES)
            buttonbar_set(str(LANG_DIRBROWSE_F1),
                          str(LANG_DIRBROWSE_F2),
                          str(LANG_DIRBROWSE_F3));
        else
            buttonbar_set("<<<", "", "");
        buttonbar_draw();
    }
#endif
#endif
    status_draw(true);

    return tc.filesindir;
}

static bool ask_resume(bool ask_once)
{
    int button;
    bool stop = false;
    static bool ignore_power = true;

#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif

#ifdef HAVE_ALARM_MOD
    if ( rtc_check_alarm_started(true) ) {
       rtc_enable_alarm(false);
       return true;
    }
#endif

    /* always resume? */
    if ( global_settings.resume == RESUME_ON)
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
        button = button_get(true);
        switch (button) {
#ifdef TREE_RUN_PRE
            case TREE_RUN_PRE:  /* catch the press, not the release */
#else
            case TREE_RUN:
#endif

#ifdef TREE_RC_RUN_PRE
            case TREE_RC_RUN_PRE:  /* catch the press, not the release */
#else
#ifdef TREE_RC_RUN
            case TREE_RC_RUN:
#endif
#endif
                ignore_power = false;
                /* Don't ignore the power button for subsequent calls */
                return true;

#ifdef TREE_POWER_BTN
                /* Initially ignore the button which powers on the box. It
                   might still be pressed since booting. */
            case TREE_POWER_BTN:
            case TREE_POWER_BTN | BUTTON_REPEAT:
                if(!ignore_power)
                    stop = true;
                break;

                /* No longer ignore the power button after it was released */
            case TREE_POWER_BTN | BUTTON_REL:
                ignore_power = false;
                break;
#endif
                /* Handle sys events, ignore button releases */
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED ||
                   (!IS_SYSEVENT(button) && !(button & BUTTON_REL)))
                    stop = true;
                break;
        }
    }

    if ( global_settings.resume == RESUME_ASK_ONCE && ask_once) {
        global_settings.resume_index = -1;
        settings_save();
    }

    ignore_power = false; 
    /* Don't ignore the power button for subsequent calls */
    return false;
}

/* load tracks from specified directory to resume play */
void resume_directory(const char *dir)
{
    if (ft_load(&tc, dir) < 0)
        return;
    lastdir[0] = 0;

    ft_build_playlist(&tc, 0);
}

/* Returns the current working directory and also writes cwd to buf if
   non-NULL.  In case of error, returns NULL. */
char *getcwd(char *buf, int size)
{
    if (!buf)
        return tc.currdir;
    else if (size > 0)
    {
        strncpy(buf, tc.currdir, size);
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

            start_wps = true;
        }
        else
            return;
    }
}

void set_current_file(char *path)
{
    char *name;
    unsigned int i;

    /* in ID3DB mode it is a bad idea to call this function */
    /* (only happens with `follow playlist') */
    if( *tc.dirfilter == SHOW_ID3DB )
    {
        return;
    }

    /* separate directory from filename */
    name = strrchr(path+1,'/');
    if (name)
    {
        *name = 0;
        strcpy(tc.currdir, path);
        *name = '/';
        name++;
    }
    else
    {
        strcpy(tc.currdir, "/");
        name = path+1;
    }

    strcpy(lastfile, name);

    tc.dircursor    =  0;
    tc.dirstart     = -1;

    if (strncmp(tc.currdir,lastdir,sizeof(lastdir)))
    {
        tc.dirlevel            =  0;
        tc.dirpos[tc.dirlevel]    = -1;
        tc.cursorpos[tc.dirlevel] =  0;

        /* use '/' to calculate dirlevel */
        for (i=1; i<strlen(path)+1; i++)
        {
            if (path[i] == '/')
            {
                tc.dirlevel++;
                tc.dirpos[tc.dirlevel]    = -1;
                tc.cursorpos[tc.dirlevel] =  0;
            }
        }
    }
}

static bool check_changed_id3mode(bool currmode)
{
    if (currmode != (global_settings.dirfilter == SHOW_ID3DB)) {
        currmode = global_settings.dirfilter == SHOW_ID3DB;
        if (currmode) {
            db_load(&tc);
        }
        else
            ft_load(&tc, NULL);
    }
    return currmode;
}

static void tree_prepare_usb(void *parameter)
{
    (void) parameter;
    tagdb_shutdown();
}

static bool dirbrowse(void)
{
    int numentries=0;
    char buf[MAX_PATH];
    int i;
    int lasti=-1;
    unsigned button;
    int tree_max_on_screen;
    bool reload_root = false;
    int lastfilter = *tc.dirfilter;
    bool lastsortcase = global_settings.sort_case;
    int lastdircursor=-1;
    bool need_update = true;
    bool exit_func = false;
    long thumbnail_time = -1; /* for delaying a thumbnail */
    bool update_all = false; /* set this to true when the whole file list
                                has been refreshed on screen */
    unsigned lastbutton = 0;
    char* currdir = tc.currdir; /* just a shortcut */
    bool id3db = *tc.dirfilter == SHOW_ID3DB;

#ifdef HAVE_LCD_BITMAP
    tree_max_on_screen = recalc_screen_height();
#else
    tree_max_on_screen = TREE_MAX_ON_SCREEN;
#endif

    tc.dircursor=0;
    tc.dirstart=0;
    tc.dirlevel=0;
    tc.firstpos=0;
    lasttable = -1;
    lastextra = -1;
    lastfirstpos = 0;

    if (*tc.dirfilter < NUM_FILTER_MODES) {
        start_resume(true);

#ifdef HAVE_RECORDING
#ifndef SIMULATOR
        if (global_settings.rec_startup && ! start_wps) {
            /* We fake being in the menu structure by calling the appropriate */
            /* parent when we drop out of each screen */
            recording_screen();
            rec_menu();
            main_menu();
        }
#endif
#endif
    }
    
    if (!start_wps) {
        numentries = showdir();
        if (numentries == -1)
            return false;  /* currdir is not a directory */
    
        if (*tc.dirfilter > NUM_FILTER_MODES && numentries==0)
        {
            splash(HZ*2, true, str(LANG_NO_FILES));
            return false;  /* No files found for rockbox_browser() */
        }
        update_all = true;

        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);
    }

    while(1) {
        struct entry *dircache = tc.dircache;

        bool restore = false;

        button = button_get_w_tmo(HZ/5);

#ifdef BOOTFILE
        if (boot_changed) {
            bool stop = false;
            unsigned int button;

            lcd_clear_display();
            lcd_puts(0,0,str(LANG_BOOT_CHANGED));
            lcd_puts(0,1,str(LANG_REBOOT_NOW));
#ifdef HAVE_LCD_BITMAP
            lcd_puts(0,3,str(LANG_CONFIRM_WITH_PLAY_RECORDER));
            lcd_puts(0,4,str(LANG_CANCEL_WITH_ANY_RECORDER));
            lcd_update();
#endif
            while (!stop) {
                button = button_get(true);
                switch (button) {
                    case TREE_RUN:
#ifdef TREE_RC_RUN
                    case TREE_RC_RUN:
#endif
                        rolo_load("/" BOOTFILE);
                        stop = true;
                        break;

                    default:
                        if(default_event_handler(button) ||
                           (button & BUTTON_REL))
                            stop = true;
                        break;
                }
            }

            restore = true;
            boot_changed = false;
        }
#endif

        switch ( button ) {
#ifdef TREE_ENTER
            case TREE_ENTER:
            case TREE_ENTER | BUTTON_REPEAT:
#endif
#ifdef TREE_RC_RUN
            case TREE_RC_RUN:
#endif
            case TREE_RUN:
#ifdef TREE_RUN_PRE
                if (((button == TREE_RUN)
#ifdef TREE_RC_RUN_PRE
                    || (button == TREE_RC_RUN))
                        && ((lastbutton != TREE_RC_RUN_PRE)
#endif
                    && (lastbutton != TREE_RUN_PRE)))
                    break;
#endif
                if ( !numentries )
                    break;

                if (id3db)
                    i = db_enter(&tc);
                else
                    i = ft_enter(&tc);
                    
                switch (i)
                {
                    case 1: reload_dir = true; break;
                    case 2: start_wps = true; break;
                    case 3: exit_func = true; break;
                    default: break;
                }

#ifdef HAVE_LCD_BITMAP
                /* maybe we have a new font */
                tree_max_on_screen = recalc_screen_height();
#endif
                /* make sure cursor is on screen */
                while ( tc.dircursor > tree_max_on_screen )
                {
                    tc.dircursor--;
                    tc.dirstart++;
                }

                restore = true;
                break;

            case TREE_EXIT:
            case TREE_EXIT | BUTTON_REPEAT:
#ifdef TREE_RC_EXIT
            case TREE_RC_EXIT:
#endif
                if (*tc.dirfilter > NUM_FILTER_MODES && tc.dirlevel < 1) {
                    exit_func = true;
                    break;
                }

                if (!tc.dirlevel)
                    break;

                if (id3db)
                    db_exit(&tc);
                else
                    if (ft_exit(&tc) == 3)
                        exit_func = true;

                restore = true;
                break;

#ifdef TREE_OFF
#if (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
            case TREE_OFF:
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    /* Stop the music if it is playing, else show the shutdown
                       screen */
                    if(audio_status())
                        audio_stop();
                    else {
                        if (!charger_inserted()) {
                            shutdown_screen();
                        } else {
                            charging_splash();
                        }
                        restore = true;
                    }
                }
                break;
#endif
#if defined(HAVE_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)
            case TREE_OFF | BUTTON_REPEAT:
                if (charger_inserted()) {
                    charging_splash();
                    restore = true;
                }
                break;
#endif
#endif

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
#ifdef TREE_RC_PREV
            case TREE_RC_PREV:
            case TREE_RC_PREV | BUTTON_REPEAT:
#endif
                if (!tc.filesindir)
                    break;

                /* start scrolling when at 1/3 of the screen */
                if (tc.dircursor >=
                        tree_max_on_screen - (2 * tree_max_on_screen) / 3
                        || (tc.dirstart == 0 && tc.dircursor > 0)) {
                    put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, false);
                    tc.dircursor--;
                    put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);
                }
                else {
                    if (tc.dirstart || tc.firstpos) {
                        if (tc.dirstart)
                            tc.dirstart--;
                        else {
                            if (tc.firstpos > max_files/2) {
                                tc.firstpos -= max_files/2;
                                tc.dirstart += max_files/2;
                                tc.dirstart--;
                            }
                            else {
                                tc.dirstart = tc.firstpos - 1;
                                tc.firstpos = 0;
                            }
                        }
                        restore = true;
                    }
                    else {
                        if (numentries < tree_max_on_screen) {
                            put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor,
                                         false);
                            tc.dircursor = numentries - 1;
                            put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor,
                                         true);
                        }
                        else if (id3db && tc.dirfull) {
                            /* load last dir segment */
                            /* use max_files/2 in case names are longer than
                                AVERAGE_FILE_LENGTH */
                            tc.firstpos = tc.dirlength - max_files/2;
                            tc.dirstart = tc.firstpos;
                            tc.dircursor = tree_max_on_screen - 1;
                            numentries = showdir();
                            update_all = true;
                            put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor,
                                         true);
                        }
                        else {
                            tc.dirstart = numentries - tree_max_on_screen;
                            tc.dircursor = tree_max_on_screen - 1;
                            restore = true;
                        }
                    }
                }
                need_update = true;
                break;

            case TREE_NEXT:
            case TREE_NEXT | BUTTON_REPEAT:
#ifdef TREE_RC_NEXT
            case TREE_RC_NEXT:
            case TREE_RC_NEXT | BUTTON_REPEAT:
#endif
                if (!tc.filesindir)
                    break;

                if (tc.dircursor + tc.dirstart + 1 < numentries ) {
                    /* start scrolling when at 2/3 of the screen */
                    if(tc.dircursor < (2 * tree_max_on_screen) / 3 ||
                            numentries - tc.dirstart <= tree_max_on_screen) {
                        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, false);
                        tc.dircursor++;
                        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);
                    }
                    else {
                        tc.dirstart++;
                        restore = true;
                    }
                }
                else if (id3db && (tc.firstpos || tc.dirfull)) {
                    if (tc.dircursor + tc.dirstart + tc.firstpos + 1 >= tc.dirlength) {
                        /* wrap and load first dir segment */
                        tc.firstpos = tc.dirstart = tc.dircursor = 0;
                    }
                    else {
                        /* load next dir segment */
                        tc.firstpos += tc.dirstart;
                        tc.dirstart = 0;
                    }
                    restore = true;
                }
                else {
                    if(numentries < tree_max_on_screen) {
                        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, false);
                        tc.dirstart = tc.dircursor = 0;
                        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);
                    }
                    else {
                        tc.dirstart = tc.dircursor = 0;
                        numentries = showdir();
                        update_all=true;
                        put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);
                    }
                }
                need_update = true;
                break;

#ifdef TREE_PGUP
            case TREE_PGUP:
            case TREE_PGUP | BUTTON_REPEAT:
                if (tc.dirstart) {
                    tc.dirstart -= tree_max_on_screen;
                    if ( tc.dirstart < 0 )
                        tc.dirstart = 0;
                }
                else if (tc.firstpos) {
                    if (tc.firstpos > max_files/2) {
                        tc.firstpos -= max_files/2;
                        tc.dirstart += max_files/2;
                        tc.dirstart -= tree_max_on_screen;
                    }
                    else {
                        tc.dirstart = tc.firstpos - tree_max_on_screen;
                        tc.firstpos = 0;
                    }
                }
                else
                    tc.dircursor = 0;
                restore = true;
                break;

            case TREE_PGDN:
            case TREE_PGDN | BUTTON_REPEAT:
                if ( tc.dirstart < numentries - tree_max_on_screen ) {
                    tc.dirstart += tree_max_on_screen;
                    if ( tc.dirstart > numentries - tree_max_on_screen )
                        tc.dirstart = numentries - tree_max_on_screen;
                }
                else if (id3db && tc.dirfull) {
                    /* load next dir segment */
                    tc.firstpos += tc.dirstart;
                    tc.dirstart = 0;
                }
                else
                    tc.dircursor = numentries - tc.dirstart - 1;
                restore = true;
                break;
#endif

            case TREE_MENU:
#ifdef TREE_RC_MENU
            case TREE_RC_MENU:
#endif
#ifdef TREE_MENU_PRE
                if (lastbutton != TREE_MENU_PRE)
                    break;
#endif
                /* don't enter menu from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    lcd_stop_scroll();
                    if (main_menu())
                        reload_dir = true;
                    restore = true;

                    id3db = check_changed_id3mode(id3db);
                }
                else /* use it as a quick exit instead */
                    exit_func = true;
                break;

            case TREE_WPS:
#ifdef TREE_RC_WPS
            case TREE_RC_WPS:
#endif
#ifdef TREE_WPS_PRE
                if ((lastbutton != TREE_WPS_PRE)
#ifdef TREE_RC_WPS
                    && (lastbutton != TREE_RC_WPS_PRE)
#endif
                    )
                    break;
#endif
                /* don't enter wps from plugin browser etc */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    if (audio_status() & AUDIO_STATUS_PLAY)
                    {
                        start_wps=true;
                    }
                    else
                    {
                        start_resume(false);
                        restore = true;
                    }
                }
                break;

#ifdef BUTTON_F2
            case BUTTON_F2:
                /* don't enter f2 from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen(CONTEXT_TREE, BUTTON_F2))
                        reload_dir = true;
                    restore = true;

                    id3db = check_changed_id3mode(id3db);
                    break;
                }

            case BUTTON_F3:
                /* don't enter f3 from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen(CONTEXT_TREE, BUTTON_F3))
                        reload_dir = true;
                    tree_max_on_screen = recalc_screen_height();
                    restore = true;
                }
                break;
#endif

            case TREE_CONTEXT:
#ifdef TREE_RC_CONTEXT
            case TREE_RC_CONTEXT:
#endif
#ifdef TREE_CONTEXT2
            case TREE_CONTEXT2:
#endif
            {
                int onplay_result;
                int attr = 0;

                if(!numentries)
                    onplay_result = onplay(NULL, 0);
                else {
                    if (currdir[1])
                        snprintf(buf, sizeof buf, "%s/%s",
                                 currdir, dircache[tc.dircursor+tc.dirstart].name);
                    else
                        snprintf(buf, sizeof buf, "/%s",
                                 dircache[tc.dircursor+tc.dirstart].name);
                    if (!id3db)
                        attr = dircache[tc.dircursor+tc.dirstart].attr;
                    onplay_result = onplay(buf, attr);
                }
                
                switch (onplay_result)
                {
                    case ONPLAY_OK:
                        restore = true;
                        break;

                    case ONPLAY_RELOAD_DIR:
                        reload_dir = true;
                        break;

                    case ONPLAY_START_PLAY:
                        start_wps = true;
                        break;
                }
                break;
            }

            case BUTTON_NONE:
                if (thumbnail_time != -1 &&
                    TIME_AFTER(current_tick, thumbnail_time))
                {   /* a delayed hovering thumbnail is due now */
                    int res;
                    if (dircache[lasti].attr & ATTR_DIRECTORY)
                    {
                        DEBUGF("Playing directory thumbnail: %s", currdir);
                        res = ft_play_dirname(lasti);
                        if (res < 0) /* failed, not existing */
                        {   /* say the number instead, as a fallback */
                            talk_id(VOICE_DIR, false);
                            talk_number(lasti+1, true);
                        }
                    }
                    else
                    { 
                        DEBUGF("Playing file thumbnail: %s/%s%s\n", 
                               currdir, dircache[lasti].name, file_thumbnail_ext);
                        /* no fallback necessary, we knew in advance 
                           that the file exists */
                        ft_play_filename(currdir, dircache[lasti].name);
                    }
                    thumbnail_time = -1; /* job done */
                }
                status_draw(false);
                break;

#ifdef HAVE_HOTSWAP
            case SYS_FS_CHANGED:
                if (!id3db)
                    reload_dir = true; 
                /* The 'dir no longer valid' situation will be caught later
                 * by checking the showdir() result. */
                break;
#endif

            default:
                if (default_event_handler_ex(button, tree_prepare_usb, NULL)
                    == SYS_USB_CONNECTED)
                {
                    tagdb_init(); /* re-init database */
                    if(*tc.dirfilter > NUM_FILTER_MODES)
                        /* leave sub-browsers after usb, doing otherwise
                           might be confusing to the user */
                        exit_func = true;
                    else
                        reload_dir = true;
                }
                break;
        }

        if ( button )
        {
            ata_spin();
            lastbutton = button;
        }

        if (start_wps)
        {
            lcd_stop_scroll();
            if (wps_show() == SYS_USB_CONNECTED)
                reload_dir = true;
#ifdef HAVE_HOTSWAP
            else 
                if (!id3db) /* Try reload to catch 'no longer valid' case. */
                    reload_dir = true;
#endif
#ifdef HAVE_LCD_BITMAP
            tree_max_on_screen = recalc_screen_height();
#endif
            id3db = check_changed_id3mode(id3db);
            restore = true;
            start_wps=false;
        }

    check_rescan:
        /* do we need to rescan dir? */
        if (reload_dir || reload_root ||
            lastfilter != *tc.dirfilter ||
            lastsortcase != global_settings.sort_case)
        {
            if ( reload_root ) {
                strcpy(currdir, "/");
                tc.dirlevel = 0;
                tc.currtable = 0;
                tc.currextra = 0;
                lasttable = -1;
                lastextra = -1;
                reload_root = false;
            }
            if (! reload_dir )
            {
                tc.dircursor = 0;
                tc.dirstart = 0;
                lastdir[0] = 0;
            }

            lastfilter = *tc.dirfilter;
            lastsortcase = global_settings.sort_case;
            restore = true;
            button_clear_queue(); /* clear button queue */
        }

        if (exit_func)
            break;

        if (restore || reload_dir) {
            /* restore display */

#ifdef HAVE_LCD_BITMAP
            tree_max_on_screen = recalc_screen_height();
#endif

            /* We need to adjust if the number of lines on screen have
               changed because of a status bar change */
            if(CURSOR_Y+LINE_Y+tc.dircursor>tree_max_on_screen) {
                tc.dirstart++;
                tc.dircursor--;
            }
#ifdef HAVE_LCD_BITMAP
            /* the sub-screen might've ruined the margins */
            lcd_setmargins(MARGIN_X,MARGIN_Y); /* leave room for cursor and
                                                  icon */
            lcd_setfont(FONT_UI);
#endif
            numentries = showdir();
            if (currdir[1] && (numentries < 0))
            {   /* not in root and reload failed */
                reload_root = true; /* try root */
                reload_dir = false;
                goto check_rescan;
            }
            update_all = true;
            put_cursorxy(CURSOR_X, CURSOR_Y + tc.dircursor, true);

            need_update = true;
            reload_dir = false;
        }

        if ( (numentries > 0) && need_update) {
            i = tc.dirstart+tc.dircursor;

            /* if MP3 filter is on, cut off the extension */
            if(lasti!=i || restore) {
                char* name;
                int attr = 0;

                if (id3db)
                    name = ((char**)tc.dircache)[lasti * tc.dentry_size];
                else {
                    struct entry* dc = tc.dircache;
                    struct entry* e = &dc[lasti];
                    name = e->name;
                    attr = e->attr;
                }

                lcd_stop_scroll();

                /* So if lastdircursor and dircursor differ, and then full
                   screen was not refreshed, restore the previous line */
                if ((lastdircursor != tc.dircursor) && !update_all ) {
                    showfileline(lastdircursor, name, attr, false); /* no scroll */
                }
                lasti=i;
                lastdircursor=tc.dircursor;
                thumbnail_time = -1; /* cancel whatever we were about to say */

                if (id3db)
                    name = ((char**)tc.dircache)[lasti * tc.dentry_size];
                else {
                    struct entry* dc = tc.dircache;
                    struct entry* e = &dc[lasti];
                    name = e->name;
                    attr = e->attr;
                }
                showfileline(tc.dircursor, name, attr, true); /* scroll please */
                need_update = true;

                if (dircache[i].attr & ATTR_DIRECTORY) /* directory? */
                {
                    /* play directory thumbnail */
                    switch (global_settings.talk_dir) {
                        case 1: /* dirs as numbers */
                            talk_id(VOICE_DIR, false);
                            talk_number(i+1, true);
                            break;

                        case 2: /* dirs spelled */
                            talk_spell(dircache[i].name, false);
                            break;

                        case 3: /* thumbnail clip */
                            /* "schedule" a thumbnail, to have a little dalay */
                            thumbnail_time = current_tick + HOVER_DELAY;
                            break;

                        default:
                            break;
                    }
                }
                else /* file */
                {
                    switch (global_settings.talk_file) {
                        case 1: /* files as numbers */
                            ft_play_filenumber(i-tc.dirsindir+1, 
                                               dircache[i].attr & TREE_ATTR_MASK);
                            break;

                        case 2: /* files spelled */
                            talk_spell(dircache[i].name, false);
                            break;

                        case 3: /* thumbnail clip */
                            /* "schedule" a thumbnail, to have a little delay */
                            if (dircache[i].attr & TREE_ATTR_THUMBNAIL)
                                thumbnail_time = current_tick + HOVER_DELAY;
                            else
                                /* spell the number as fallback */
                                talk_spell(dircache[i].name, false);
                            break;

                        default:
                            break;
                    }
                }
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
static bool add_dir(char* dirname, int len, int fd)
{
    bool abort = false;
    DIR* dir;

    /* check for user abort */
#ifdef BUTTON_STOP
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
            int dirlen = strlen(dirname);
            bool result;

            if (!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, ".."))
                continue;

            if (dirname[1])
                snprintf(dirname+dirlen, len-dirlen, "/%s", entry->d_name);
            else
                snprintf(dirname, len, "/%s", entry->d_name);

            result = add_dir(dirname, len, fd);
            dirname[dirlen] = '\0';
            if (result) {
                abort = true;
                break;
            }
        }
        else {
            int x = strlen(entry->d_name);
            int xl;
            unsigned int i;

            /* add all supported audio files to playlists */
            for (i=0; i < sizeof(filetypes); i++) {
                if (filetypes[i].tree_attr == TREE_ATTR_MPA) {
                    xl=strlen(filetypes[i].extension);
                    if (!strcasecmp(&entry->d_name[x-xl],
                                    filetypes[i].extension))
                    {
                        char buf[8];
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
             tc.currdir[1] ? tc.currdir : "/root");

    lcd_clear_display();
    lcd_puts(0,0,str(LANG_CREATING));
    lcd_puts_scroll(0,1,filename);
    lcd_update();

    fd = creat(filename,0);
    if (fd < 0)
        return false;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(true);
#endif
    
    snprintf(filename, sizeof(filename), "%s",
             tc.currdir[1] ? tc.currdir : "/");
    plsize = 0;
    add_dir(filename, sizeof(filename), fd);
    close(fd);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    cpu_boost(false);
#endif
    
    sleep(HZ);

    return true;
}

bool rockbox_browse(const char *root, int dirfilter)
{
    static struct tree_context backup;

    backup = tc;
    reload_dir = true;
    memcpy(tc.currdir, root, sizeof(tc.currdir));
    start_wps = false;
    tc.dirfilter = &dirfilter;
    
    dirbrowse();

    tc = backup;
    reload_dir = true;

    return false;
}

void tree_init(void)
{
    /* We copy the settings value in case it is changed by the user. We can't
       use it until the next reboot. */
    max_files = global_settings.max_files_in_dir;
    
    /* initialize tree context struct */
    memset(&tc, 0, sizeof(tc));
    tc.dirfilter = &global_settings.dirfilter;

    tagdb_init();
    
    tc.name_buffer_size = AVERAGE_FILENAME_LENGTH * max_files;
    tc.name_buffer = buffer_alloc(tc.name_buffer_size);

    tc.dircache_size = max_files * sizeof(struct entry);
    tc.dircache = buffer_alloc(tc.dircache_size);
}

void bookmark_play(char *resume_file, int index, int offset, int seed,
                   char *filename)
{
    int i;
    int len=strlen(resume_file);

    if (!strcasecmp(&resume_file[len-4], ".m3u"))
    {
        /* Playlist playback */
        char* slash;
        // check that the file exists
        int fd = open(resume_file, O_RDONLY);
        if(fd<0)
            return;
        close(fd);

        slash = strrchr(resume_file,'/');
        if (slash)
        {
            char* cp;
            *slash=0;

            cp=resume_file;
            if (!cp[0])
                cp="/";

            if (playlist_create(cp, slash+1) != -1)
            {
                if (global_settings.playlist_shuffle)
                    playlist_shuffle(seed, -1);
                playlist_start(index,offset);
            }
            *slash='/';
        }
    }
    else
    {
        /* Directory playback */
        lastdir[0]='\0';
        if (playlist_create(resume_file, NULL) != -1)
        {
            resume_directory(resume_file);
            if (global_settings.playlist_shuffle)
                playlist_shuffle(seed, -1);

            /* Check if the file is at the same spot in the directory,
               else search for it */
            if ((strcmp(strrchr(playlist_peek(index) + 1,'/') + 1,
                        filename)))
            {
                for ( i=0; i < playlist_amount(); i++ ) 
                {
                    if ((strcmp(strrchr(playlist_peek(i) + 1,'/') + 1,
                                filename)) == 0)
                        break;
                }
                if (i < playlist_amount())
                    index = i;
                else
                    return;
            }
            playlist_start(index,offset);
        }
    }

    start_wps=true;
}

int ft_play_filenumber(int pos, int attr)
{
    /* try to find a voice ID for the extension, if known */
    unsigned int j;
    int ext_id = -1; /* default to none */
    for (j=0; j<sizeof(filetypes)/sizeof(*filetypes); j++)
    {
        if (attr == filetypes[j].tree_attr)
        {
            ext_id = filetypes[j].voiceclip;
            break;
        }
    }

    talk_id(VOICE_FILE, false);
    talk_number(pos, true);
    talk_id(ext_id, true);
    return 1;
}

int ft_play_dirname(int start_index)
{
    int fd;
    char dirname_mp3_filename[MAX_PATH+1];
    struct entry *dircache = tc.dircache;

    if (audio_status() & AUDIO_STATUS_PLAY)
        return 0;

    snprintf(dirname_mp3_filename, sizeof(dirname_mp3_filename), "%s/%s/%s",
             tc.currdir[1] ? tc.currdir : "" , dircache[start_index].name,
             dir_thumbnail_name);

    DEBUGF("Checking for %s\n", dirname_mp3_filename);

    fd = open(dirname_mp3_filename, O_RDONLY);
    if (fd < 0)
    {
        DEBUGF("Failed to find: %s\n", dirname_mp3_filename);
        return -1;
    }

    close(fd);
  
    DEBUGF("Found: %s\n", dirname_mp3_filename);

    talk_file(dirname_mp3_filename, false);
    return 1;
}

void ft_play_filename(char *dir, char *file)
{
    char name_mp3_filename[MAX_PATH+1];

    if (audio_status() & AUDIO_STATUS_PLAY)
        return;

    if (strcasecmp(&file[strlen(file) - strlen(file_thumbnail_ext)],
                    file_thumbnail_ext))
    {   /* file has no .talk extension */
        snprintf(name_mp3_filename, sizeof(name_mp3_filename),
                 "%s/%s%s", dir, file, file_thumbnail_ext);

        talk_file(name_mp3_filename, false);
    }
    else
    {   /* it already is a .talk file, play this directly */
        snprintf(name_mp3_filename, sizeof(name_mp3_filename),
            "%s/%s", dir, file);
        talk_id(LANG_VOICE_DIR_HOVER, false); /* prefix it */
        talk_file(name_mp3_filename, true);
    }
}

