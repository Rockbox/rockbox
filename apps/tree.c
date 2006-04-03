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
#include "gwps.h"
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
#include "tagtree.h"
#include "recorder/recording.h"
#include "rtc.h"
#include "dircache.h"
#include "tagcache.h"
#include "yesno.h"

/* gui api */
#include "list.h"
#include "statusbar.h"
#include "splash.h"
#include "buttonbar.h"
#include "textarea.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#endif

/* a table for the know file types */
const struct filetype filetypes[] = {
    { "mp3", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "mp2", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "mpa", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
#if CONFIG_CODEC == SWCODEC
    /* Temporary hack to allow playlist creation */
    { "mp1", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "ogg", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "wma", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "wav", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "flac",TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "ac3", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "a52", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "mpc", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "wv",  TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "m4a", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "mp4", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "shn", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "aif", TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
    { "aiff",TREE_ATTR_MPA, Icon_Audio, VOICE_EXT_MPA },
#endif
    { "m3u", TREE_ATTR_M3U, Icon_Playlist, LANG_PLAYLIST },
    { "cfg", TREE_ATTR_CFG, Icon_Config, VOICE_EXT_CFG },
    { "wps", TREE_ATTR_WPS, Icon_Wps, VOICE_EXT_WPS },
#ifdef HAVE_REMOTE_LCD
    { "rwps", TREE_ATTR_RWPS, Icon_Wps, VOICE_EXT_RWPS },
#endif
#ifdef HAVE_LCD_COLOR
    { "bmp", TREE_ATTR_BMP, Icon_Wps, VOICE_EXT_WPS },
#endif
#ifdef CONFIG_TUNER
    { "fmr", TREE_ATTR_FMR, Icon_Preset, LANG_FMR },
#endif
    { "lng", TREE_ATTR_LNG, Icon_Language, LANG_LANGUAGE },
    { "rock",TREE_ATTR_ROCK,Icon_Plugin, VOICE_EXT_ROCK },
#ifdef HAVE_LCD_BITMAP
    { "fnt", TREE_ATTR_FONT,Icon_Font, VOICE_EXT_FONT },
    { "kbd", TREE_ATTR_KBD, Icon_Keyboard, VOICE_EXT_KBD },
#endif
    { "bmark",TREE_ATTR_BMARK, Icon_Bookmark, VOICE_EXT_BMARK },
#ifdef BOOTFILE_EXT
    { BOOTFILE_EXT, TREE_ATTR_MOD, Icon_Firmware, VOICE_EXT_AJZ },
#endif /* #ifndef SIMULATOR */
};

struct gui_synclist tree_lists;

/* I put it here because other files doesn't use it yet,
 * but should be elsewhere since it will be used mostly everywhere */
#ifdef HAS_BUTTONBAR
struct gui_buttonbar tree_buttonbar;
#endif
static struct tree_context tc;

bool boot_changed = false;

char lastfile[MAX_PATH];
static char lastdir[MAX_PATH];
static int lasttable, lastextra, lastfirstpos;
static int max_files = 0;

static bool reload_dir = false;

static bool start_wps = false;
static bool dirbrowse(void);
static int curr_context = false;/* id3db or tree*/

/*
 * removes the extension of filename (if it doesn't start with a .)
 * puts the result in buffer
 */
char * strip_extension(char * filename, char * buffer)
{
    int dotpos;
    char * dot=strrchr(filename, '.');
    if(dot!=0 && filename[0]!='.')
    {
        dotpos = dot-filename;
        strncpy(buffer, filename, dotpos);
        buffer[dotpos]='\0';
        return(buffer);
    }
    else
        return(filename);
}
char * tree_get_filename(int selected_item, void * data, char *buffer)
{
    struct tree_context * local_tc=(struct tree_context *)data;
    char *name;
    int attr=0;
    bool id3db = *(local_tc->dirfilter) == SHOW_ID3DB;

    if (id3db) {
        char **buf = local_tc->dircache;
        name = buf[selected_item * (local_tc->dentry_size/sizeof(int))];
    }
    else {
        struct entry* dc = local_tc->dircache;
        struct entry* e = &dc[selected_item];
        name = e->name;
        attr = e->attr;
    }
    /* if any file filter is on, and if it's not a directory,
     * strip the extension */

    if ( (*(local_tc->dirfilter) != SHOW_ID3DB) && !(attr & ATTR_DIRECTORY)
        && (*(local_tc->dirfilter) != SHOW_ALL) )
    {
        return(strip_extension(name, buffer));
    }
    return(name);
}


void tree_get_fileicon(int selected_item, void * data, ICON * icon)
{
    struct tree_context * local_tc=(struct tree_context *)data;
    bool id3db = *(local_tc->dirfilter) == SHOW_ID3DB;
    if (id3db) {
        *icon = (ICON)tagtree_get_icon(&tc);
    }
    else {
        struct entry* dc = local_tc->dircache;
        struct entry* e = &dc[selected_item];
        *icon = (ICON)filetype_get_icon(e->attr);
    }
}

bool check_rockboxdir(void)
{
    DIR *dir = opendir(ROCKBOX_DIR);
    if(!dir)
    {   /* No need to localise this message.
           If .rockbox is missing, it wouldn't work anyway */
        int i;
        FOR_NB_SCREENS(i)
            screens[i].clear_display();
        gui_syncsplash(HZ*2, true, "No .rockbox directory");
        FOR_NB_SCREENS(i)
            screens[i].clear_display();
        gui_syncsplash(HZ*2, true, "Installation incomplete");
        return false;
    }
    closedir(dir);
    return true;
}

void browse_root(void)
{
    gui_sync_wps_screen_init();

    filetype_init();
    check_rockboxdir();

    strcpy(tc.currdir, "/");

#ifdef HAVE_LCD_CHARCELLS
    int i;
    FOR_NB_SCREENS(i)
        screens[i].double_height(false);
#endif
#ifdef HAS_BUTTONBAR
    gui_buttonbar_init(&tree_buttonbar);
    /* since archos only have one screen, no need to create more than that */
    gui_buttonbar_set_display(&tree_buttonbar, &(screens[SCREEN_MAIN]) );
#endif
    gui_synclist_init(&tree_lists, &tree_get_filename, &tc);
    gui_synclist_set_icon_callback(&tree_lists,
                  global_settings.show_icons?&tree_get_fileicon:NULL);
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

/* talkbox hovering delay, to avoid immediate disk activity */
#define HOVER_DELAY (HZ/2)
/*
 * Returns the position of a given file in the current directory
 * returns -1 if not found
 */
int tree_get_file_position(char * filename)
{
    int i;

    /* use lastfile to determine the selected item (default=0) */
    for (i=0; i < tc.filesindir; i++)
    {
        struct entry* dc = tc.dircache;
        struct entry* e = &dc[i];
        if (!strcasecmp(e->name, filename))
            return(i);
    }
    return(-1);/* no file can match, returns undefined */
}

/*
 * Called when a new dir is loaded (for example when returning from other apps ...)
 * also completely redraws the tree
 */
static int update_dir(void)
{
    bool id3db = *tc.dirfilter == SHOW_ID3DB;
    bool changed = false;
    /* Checks for changes */
    if (id3db) {
        if (tc.currtable != lasttable ||
            tc.currextra != lastextra ||
            tc.firstpos  != lastfirstpos)
        {
            if (tagtree_load(&tc) < 0)
                return -1;

            lasttable = tc.currtable;
            lastextra = tc.currextra;
            lastfirstpos = tc.firstpos;
            changed = true;
        }
    }
    else {
        /* if the tc.currdir has been changed, reload it ...*/
        if (strncmp(tc.currdir, lastdir, sizeof(lastdir)) || reload_dir) {

            if (ft_load(&tc, NULL) < 0)
                return -1;
            strcpy(lastdir, tc.currdir);
            changed = true;
        }
    }
    /* if selected item is undefined */
    if (tc.selected_item == -1)
    {
        /* use lastfile to determine the selected item */
        tc.selected_item = tree_get_file_position(lastfile);

        /* If the file doesn't exists, select the first one (default) */
        if(tc.selected_item < 0)
            tc.selected_item = 0;
        changed = true;
    }
    if (changed)
    {
        if(!id3db && (tc.dirfull ||
                      tc.filesindir == global_settings.max_files_in_dir) )
        {
            gui_syncsplash(HZ, true, str(LANG_SHOWDIR_BUFFER_FULL));
        }
    }
    gui_synclist_set_nb_items(&tree_lists, tc.filesindir);
    gui_synclist_set_icon_callback(&tree_lists,
                  global_settings.show_icons?&tree_get_fileicon:NULL);
    if( tc.selected_item >= tc.filesindir)
        tc.selected_item=tc.filesindir-1;

    gui_synclist_select_item(&tree_lists, tc.selected_item);
    gui_synclist_draw(&tree_lists);
    gui_syncstatusbar_draw(&statusbars, true);
#ifdef HAS_BUTTONBAR
    if (global_settings.buttonbar) {
        if (*tc.dirfilter < NUM_FILTER_MODES)
            gui_buttonbar_set(&tree_buttonbar, str(LANG_DIRBROWSE_F1),
                          str(LANG_DIRBROWSE_F2),
                          str(LANG_DIRBROWSE_F3));
        else
            gui_buttonbar_set(&tree_buttonbar, "<<<", "", "");
        gui_buttonbar_draw(&tree_buttonbar);
    }
#endif
    return tc.filesindir;
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

static void start_resume(bool just_powered_on)
{
    bool do_resume = false;
    if ( global_settings.resume_index != -1 ) {
        DEBUGF("Resume index %X offset %X\n",
               global_settings.resume_index,
               global_settings.resume_offset);

#ifdef HAVE_ALARM_MOD
        if ( rtc_check_alarm_started(true) ) {
           rtc_enable_alarm(false);
           do_resume = true;
        }
#endif

        /* always resume? */
        if ( global_settings.resume || ! just_powered_on)
            do_resume = true;

        if (! do_resume) return;

        if (playlist_resume() != -1)
        {
            playlist_start(global_settings.resume_index,
                global_settings.resume_offset);

            start_wps = true;
        }
        else return;
    }
    else if (! just_powered_on) {
        gui_syncsplash(HZ*2, true, str(LANG_NOTHING_TO_RESUME));
    }
}

/* Selects a file and update tree context properly */
void set_current_file(char *path)
{
    char *name;
    int i;

    /* in ID3DB mode it is a bad idea to call this function */
    /* (only happens with `follow playlist') */
    if( *tc.dirfilter == SHOW_ID3DB )
        return;

    /* separate directory from filename */
    /* gets the directory's name and put it into tc.currdir */
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
    
    /* undefined item selected */
    tc.selected_item = -1;

    /* If we changed dir we must recalculate the dirlevel
       and adjust the selected history properly */
    if (strncmp(tc.currdir,lastdir,sizeof(lastdir)))
    {
        tc.dirlevel =  0;
        tc.selected_item_history[tc.dirlevel] = -1;

        /* use '/' to calculate dirlevel */
        /* FIXME : strlen(path) : crazy oO better to store it at
           the beginning */
        int path_len = strlen(path) + 1;
        for (i = 1; i < path_len; i++)
        {
            if (path[i] == '/')
            {
                tc.dirlevel++;
                tc.selected_item_history[tc.dirlevel] = -1;
            }
        }
    }
}

static bool check_changed_id3mode(bool currmode)
{
    if (currmode != (global_settings.dirfilter == SHOW_ID3DB)) {
        currmode = global_settings.dirfilter == SHOW_ID3DB;
        if (currmode) {
            curr_context=CONTEXT_ID3DB;
            tagtree_load(&tc);
        }
        else
        {
            curr_context=CONTEXT_TREE;
            ft_load(&tc, NULL);
        }
    }
    return currmode;
}
/* main loop, handles key events */
static bool dirbrowse(void)
{
    int numentries=0;
    char buf[MAX_PATH];
    int lasti = -1;
    unsigned button;
    bool reload_root = false;
    int lastfilter = *tc.dirfilter;
    bool lastsortcase = global_settings.sort_case;
    bool need_update = true;
    bool exit_func = false;
    long thumbnail_time = -1; /* for delaying a thumbnail */

    unsigned lastbutton = 0;
    char* currdir = tc.currdir; /* just a shortcut */
    bool id3db = *tc.dirfilter == SHOW_ID3DB;

    if (id3db)
        curr_context=CONTEXT_ID3DB;
    else
        curr_context=CONTEXT_TREE;
    tc.selected_item = 0;
    tc.dirlevel=0;
    tc.firstpos=0;
    lasttable = -1;
    lastextra = -1;
    lastfirstpos = 0;

    if (*tc.dirfilter < NUM_FILTER_MODES) {
#ifdef HAVE_RECORDING
#ifndef SIMULATOR
        if (global_settings.rec_startup) {
            /* We fake being in the menu structure by calling
               the appropriate parent when we drop out of each screen */
            recording_screen();
            rec_menu();
            main_menu();
        }
        else
#endif
#endif
        start_resume(true);

    }
    /* If we don't need to show the wps, draw the dir */
    if (!start_wps) {
        numentries = update_dir();
        if (numentries == -1)
            return false;  /* currdir is not a directory */

        if (*tc.dirfilter > NUM_FILTER_MODES && numentries==0)
        {
            gui_syncsplash(HZ*2, true, str(LANG_NO_FILES));
            return false;  /* No files found for rockbox_browser() */
        }
    }

    while(1) {
        struct entry *dircache = tc.dircache;
        bool restore = false;
#ifdef BOOTFILE
        if (boot_changed) {
            char *lines[]={str(LANG_BOOT_CHANGED), str(LANG_REBOOT_NOW)};
            struct text_message message={lines, 2};
            if(gui_syncyesno_run(&message, NULL, NULL)==YESNO_YES)
                rolo_load("/" BOOTFILE);
            restore = true;
            boot_changed = false;
        }
#endif
        button = button_get_w_tmo(HZ/5);
        need_update = gui_synclist_do_button(&tree_lists, button);

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
                /* nothing to do if no files to display */
                if ( numentries == 0 )
                    break;

                switch (id3db?tagtree_enter(&tc):ft_enter(&tc))
                {
                    case 1: reload_dir = true; break;
                    case 2: start_wps = true; break;
                    case 3: exit_func = true; break;
                    default: break;
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
                /* if we are in /, nothing to do */
                if (tc.dirlevel == 0)
                    break;

                if (id3db)
                    tagtree_exit(&tc);
                else
                    if (ft_exit(&tc) == 3)
                        exit_func = true;

                restore = true;
                break;

#ifdef TREE_OFF
            case TREE_OFF:
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    /* Stop the music if it is playing */
                    if(audio_status()) {
                        if (!global_settings.party_mode)
                            audio_stop();
                    }
#if defined(HAVE_CHARGING) && \
    (CONFIG_KEYPAD == RECORDER_PAD) && !defined(HAVE_SW_POWEROFF)
                    else { 
                        if (!charger_inserted()) {
                            if(shutdown_screen())
                                reload_dir = true;
                        } else {
                            charging_splash();
                        }
                        restore = true;
                    }
#endif
                }
                break;
#if defined(HAVE_CHARGING) && !defined(HAVE_POWEROFF_WHILE_CHARGING)
            case TREE_OFF | BUTTON_REPEAT:
                if (charger_inserted()) {
                    charging_splash();
                    restore = true;
                }
                break;
#endif
#endif /* TREE_OFF */
            case TREE_MENU:
#ifdef TREE_RC_MENU
            case TREE_RC_MENU:
#endif
#ifdef TREE_MENU_PRE
                if (lastbutton != TREE_MENU_PRE
#ifdef TREE_RC_MENU_PRE
                    && lastbutton != TREE_RC_MENU_PRE
#endif
                    )
                    break;
#endif
                /* don't enter menu from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    int i;
                    FOR_NB_SCREENS(i)
                        screens[i].stop_scroll();
                    if (main_menu())
                        reload_dir = true;
                    restore = true;

                    id3db = check_changed_id3mode(id3db);
                    if(id3db)
                        reload_dir = true;
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

#ifdef TREE_QUICK
            case TREE_QUICK:
#ifdef TREE_RC_QUICK
            case TREE_RC_QUICK:
#endif
                /* don't enter f2 from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen_quick(button))
                        reload_dir = true;
                    restore = true;

                    id3db = check_changed_id3mode(id3db);
                    reload_dir = true;
                }
                break;
#endif

#ifdef BUTTON_F3
            case BUTTON_F3:
                /* don't enter f3 from plugin browser */
                if (*tc.dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen_f3(button))
                        reload_dir = true;
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
                    onplay_result = onplay(NULL, 0, curr_context);
                else {
                    if (id3db)
                    {
                        switch (tc.currtable)
                        {
                            case allsongs:
                            case songs4album:
                            case songs4artist:
                            case searchsongs:
                                attr=TREE_ATTR_MPA;
                                tagtree_get_filename(&tc, buf, sizeof(buf));
                                break;
                        }
                    }
                    else
                    {
                        attr = dircache[tc.selected_item].attr;

                        if (currdir[1]) /* Not in / */
                            snprintf(buf, sizeof buf, "%s/%s",
                                     currdir,
                                     dircache[tc.selected_item].name);
                        else /* In / */
                            snprintf(buf, sizeof buf, "/%s",
                                     dircache[tc.selected_item].name);
                    }
                    onplay_result = onplay(buf, attr, curr_context);
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
                               currdir, dircache[lasti].name,
                               file_thumbnail_ext);
                        /* no fallback necessary, we knew in advance
                           that the file exists */
                        ft_play_filename(currdir, dircache[lasti].name);
                    }
                    thumbnail_time = -1; /* job done */
                }
                gui_syncstatusbar_draw(&statusbars, false);
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
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                {
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

        if (start_wps && audio_status() )
        {
            int i;
#ifdef HAVE_LCD_COLOR
            fb_data* old_backdrop;
#endif

            FOR_NB_SCREENS(i)
                screens[i].stop_scroll();
#ifdef HAVE_LCD_COLOR
            old_backdrop = lcd_get_backdrop();
#endif
            if (gui_wps_show() == SYS_USB_CONNECTED)
                reload_dir = true;
#ifdef HAVE_LCD_COLOR
            lcd_set_backdrop(old_backdrop);
#endif
#ifdef HAVE_HOTSWAP
            else
                if (!id3db) /* Try reload to catch 'no longer valid' case. */
                    reload_dir = true;
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
                gui_synclist_select_item(&tree_lists, 0);
                gui_synclist_draw(&tree_lists);
                tc.selected_item = 0;
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
            numentries = update_dir();
            if (currdir[1] && (numentries < 0))
            {   /* not in root and reload failed */
                reload_root = true; /* try root */
                reload_dir = false;
                goto check_rescan;
            }
            need_update = true;
            reload_dir = false;
        }

        if(need_update) {
            tc.selected_item = gui_synclist_get_sel_pos(&tree_lists);
            need_update=false;
            if ( numentries > 0 ) {
                /* Voice the file if changed */
                if(lasti != tc.selected_item || restore) {
                    lasti = tc.selected_item;
                    thumbnail_time = -1; /* Cancel whatever we were
                                            about to say */

                    /* Directory? */
                    if (dircache[tc.selected_item].attr & ATTR_DIRECTORY)
                    {
                        /* play directory thumbnail */
                        switch (global_settings.talk_dir) {
                            case 1: /* dirs as numbers */
                                talk_id(VOICE_DIR, false);
                                talk_number(tc.selected_item+1, true);
                                break;

                            case 2: /* dirs spelled */
                                talk_spell(dircache[tc.selected_item].name,
                                           false);
                                break;

                            case 3: /* thumbnail clip */
                                /* "schedule" a thumbnail, to have a little
                                   delay */
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
                                ft_play_filenumber(
                                    tc.selected_item-tc.dirsindir+1,
                                    dircache[tc.selected_item].attr &
                                    TREE_ATTR_MASK);
                                break;

                            case 2: /* files spelled */
                                talk_spell(dircache[tc.selected_item].name,
                                           false);
                                break;

                            case 3: /* thumbnail clip */
                                /* "schedule" a thumbnail, to have a little
                                   delay */
                                if (dircache[tc.selected_item].attr &
                                    TREE_ATTR_THUMBNAIL)
                                    thumbnail_time = current_tick + HOVER_DELAY;
                                else
                                    /* spell the number as fallback */
                                    talk_spell(dircache[tc.selected_item].name,
                                               false);
                                break;

                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    return true;
}

static int plsize = 0;
static long pltick;
static bool add_dir(char* dirname, int len, int fd)
{
    bool abort = false;
    DIRCACHED* dir;

    /* check for user abort */
    if (button_get(false) == TREE_ABORT)
        return true;

    dir = opendir_cached(dirname);
    if(!dir)
        return true;

    while (true) {
        struct dircache_entry *entry;

        entry = readdir_cached(dir);
        if (!entry)
            break;
        if (entry->attribute & ATTR_DIRECTORY) {
            int dirlen = strlen(dirname);
            bool result;

            if (!strcmp((char *)entry->d_name, ".") ||
                !strcmp((char *)entry->d_name, ".."))
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
            int x = strlen((char *)entry->d_name);
            unsigned int i;
            char *cp = strrchr((char *)entry->d_name,'.');

            if (cp) {
                cp++;

                /* add all supported audio files to playlists */
                for (i=0; i < sizeof(filetypes)/sizeof(struct filetype); i++) {
                    if (filetypes[i].tree_attr == TREE_ATTR_MPA) {
                        if (!strcasecmp(cp, filetypes[i].extension)) {
                            char buf[8];
                            int i;
                            write(fd, dirname, strlen(dirname));
                            write(fd, "/", 1);
                            write(fd, entry->d_name, x);
                            write(fd, "\n", 1);

                            plsize++;
                            if(TIME_AFTER(current_tick, pltick+HZ/4)) {
                                pltick = current_tick;
                                
                                snprintf(buf, sizeof buf, "%d", plsize);
#ifdef HAVE_LCD_BITMAP
                                FOR_NB_SCREENS(i)
                                {
                                    screens[i].puts(0, 4, (unsigned char *)buf);
                                    gui_textarea_update(&screens[i]);
                                }
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
                                FOR_NB_SCREENS(i) {
                                    screens[i].puts(x,0,buf);
                                }
#endif
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    closedir_cached(dir);

    return abort;
}

bool create_playlist(void)
{
    int fd;
    int i;
    char filename[MAX_PATH];

    pltick = current_tick;
    
    snprintf(filename, sizeof filename, "%s.m3u",
             tc.currdir[1] ? tc.currdir : "/root");
    FOR_NB_SCREENS(i)
    {
        gui_textarea_clear(&screens[i]);
        screens[i].puts(0, 0, str(LANG_CREATING));
        screens[i].puts_scroll(0, 1, (unsigned char *)filename);
#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR)
        gui_textarea_update(&screens[i]);
#endif
    }
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
        /* check that the file exists */
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

/* These two functions are called by the USB and shutdown handlers */
void tree_flush(void)
{
    tagcache_stop_scan();
    playlist_shutdown();

#ifdef HAVE_DIRCACHE
    if (global_settings.dircache)
    {
        if (dircache_is_enabled())
            global_settings.dircache_size = dircache_get_cache_size();
        dircache_disable();
    }
    else
    {
        global_settings.dircache_size = 0;
    }
    settings_save();
#endif
}

void tree_restore(void)
{
#ifdef HAVE_DIRCACHE
    if (global_settings.dircache)
    {
        /* Print "Scanning disk..." to the display. */
        int i;
        FOR_NB_SCREENS(i)
        {
            screens[i].putsxy((LCD_WIDTH/2) -
                              ((strlen(str(LANG_DIRCACHE_BUILDING)) *
                                screens[i].char_width)/2),
                              LCD_HEIGHT-screens[i].char_height*3,
                              str(LANG_DIRCACHE_BUILDING));
            gui_textarea_update(&screens[i]);
        }

        dircache_build(global_settings.dircache_size);

        /* Clean the text when we are done. */
        FOR_NB_SCREENS(i)
        {
            gui_textarea_clear(&screens[i]);
        }
    }
    tagcache_start_scan();
#endif
}
