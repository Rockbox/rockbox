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
#include "bookmark.h"
#include "onplay.h"
#include "buffer.h"
#include "plugin.h"
#include "power.h"
#include "action.h"
#include "talk.h"
#include "filetypes.h"
#include "misc.h"

#ifdef HAVE_LCD_BITMAP
#include "widgets.h"
#define BOOTFILE "ajbrec.ajz"
#else
#define BOOTFILE "archos.mod"
#endif

/* This flag is set by dirbrowse() if a new language is loaded.
   It is defined in settings_menu.c */
extern bool language_changed;

/* a table for the know file types */
const struct filetype filetypes[] = {
    { ".mp3", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".mp2", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".mpa", TREE_ATTR_MPA, File, VOICE_EXT_MPA },
    { ".m3u", TREE_ATTR_M3U, Playlist, LANG_PLAYINDICES_PLAYLIST },
    { ".cfg", TREE_ATTR_CFG, Config, VOICE_EXT_CFG },
    { ".wps", TREE_ATTR_WPS, Wps, VOICE_EXT_WPS },
    { ".lng", TREE_ATTR_LNG, Language, LANG_LANGUAGE },
    { ".rock",TREE_ATTR_ROCK,Plugin, VOICE_EXT_ROCK },
#ifdef HAVE_LCD_BITMAP
    { ".fnt", TREE_ATTR_FONT,Font, VOICE_EXT_FONT },
#endif
    { ".bmark",TREE_ATTR_BMARK, Bookmark, VOICE_EXT_BMARK },
#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP
    { ".ajz", TREE_ATTR_MOD, Mod_Ajz, VOICE_EXT_AJZ },
#else
    { ".mod", TREE_ATTR_MOD, Mod_Ajz, VOICE_EXT_AJZ },
#endif
#endif /* #ifndef SIMULATOR */
};

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
static int dirsindir; /* how many of the dircache entries are directories */
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

static bool start_wps = false;
static bool dirbrowse(const char *root, const int *dirfilter);

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

#ifndef SIMULATOR
    dirbrowse("/", &global_settings.dirfilter);

#else
    if (!dirbrowse("/", &global_settings.dirfilter)) {
        DEBUGF("No filesystem found. Have you forgotten to create it?\n");
    }
#endif
}

void tree_get_filetypes(const struct filetype** types, int* count)
{
    *types = filetypes;
    *count = sizeof(filetypes) / sizeof(*filetypes);
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

static int build_playlist(int start_index)
{
    int i;
    int start=start_index;

    for(i = 0;i < filesindir;i++)
    {
        if((dircache[i].attr & TREE_ATTR_MASK) == TREE_ATTR_MPA)
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

static int play_filenumber(int pos, int attr)
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

static int play_dirname(int start_index)
{
    int fd;
    char dirname_mp3_filename[MAX_PATH+1];

    if (mpeg_status() & MPEG_STATUS_PLAY)
        return 0;

    snprintf(dirname_mp3_filename, sizeof(dirname_mp3_filename), "%s/%s/%s",
         currdir, dircache[start_index].name, dir_thumbnail_name);

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

static void play_filename(char *dir, char *file)
{
    char name_mp3_filename[MAX_PATH+1];

    if (mpeg_status() & MPEG_STATUS_PLAY)
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

static int compare(const void* p1, const void* p2)
{
    struct entry* e1 = (struct entry*)p1;
    struct entry* e2 = (struct entry*)p2;
    int criteria;

    if (e1->attr & ATTR_DIRECTORY && e2->attr & ATTR_DIRECTORY)
    {   /* two directories */
        criteria = global_settings.sort_dir;
    }
    else if (!(e1->attr & ATTR_DIRECTORY) && !(e2->attr & ATTR_DIRECTORY))
    {   /* two files */
        criteria = global_settings.sort_file;
    }
    else /* dir and file, dir goes first */
        return ( e2->attr & ATTR_DIRECTORY ) - ( e1->attr & ATTR_DIRECTORY );

    switch(criteria)
    {
    case 3: /* sort type */
        {
            int t1 = e1->attr & TREE_ATTR_MASK;
            int t2 = e2->attr & TREE_ATTR_MASK;

            if (!t1) /* unknown type */
                t1 = 0x7FFFFFFF; /* gets a high number, to sort after known */
            if (!t2) /* unknown type */
                t2 = 0x7FFFFFFF; /* gets a high number, to sort after known */

            if (t1 - t2) /* if different */
                return t1 - t2;
            /* else fall through to alphabetical sorting */
        }
    case 0: /* sort alphabetically */
        if (global_settings.sort_case)
            return strncmp(e1->name, e2->name, MAX_PATH);
        else
            return strncasecmp(e1->name, e2->name, MAX_PATH);
    case 1: /* sort date */
        return e1->time_write - e2->time_write;
    case 2: /* sort date, newest first */
        return e2->time_write - e1->time_write;
    }
    return 0; /* never reached */
}

static void showfileline(int line, int direntry, bool scroll, const int *dirfilter)
{
    char* name = dircache[direntry].name;
    int xpos = LINE_X;
    char* dotpos = NULL;

#ifdef HAVE_LCD_CHARCELLS
    if (!global_settings.show_icons)
        xpos--;
#endif

    /* if any file filter is on, strip the extension */
    if (*dirfilter != SHOW_ALL &&
        !(dircache[direntry].attr & ATTR_DIRECTORY))
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

/* walk a directory and check all dircache entries if a .talk file exists */
void check_file_thumbnails(const char *dirname, int num_files)
{
    int i;
    struct dirent *entry;
    DIR *dir;
    
    dir = opendir(dirname);
    if(!dir)
        return;

    for (i=0; i<num_files; i++) /* mark all files as non talking, except the .talk ones */
    {
        if (dircache[i].attr & ATTR_DIRECTORY)
            continue; /* we're not touching directories */

        if (strcasecmp(file_thumbnail_ext,
            &dircache[i].name[strlen(dircache[i].name)
                              - strlen(file_thumbnail_ext)]))
        {   /* no .talk file */
            dircache[i].attr &= ~TREE_ATTR_THUMBNAIL; /* clear */
        }
        else
        {   /* .talk file, we later let them speak themselves */
            dircache[i].attr |= TREE_ATTR_THUMBNAIL; /* set */
        }
    }
    
    while((entry = readdir(dir)) != 0) /* walk directory */
    {
        int ext_pos;

        ext_pos = strlen(entry->d_name) - strlen(file_thumbnail_ext);
        if (ext_pos <= 0 /* too short to carry ".talk" */
            || (entry->attribute & ATTR_DIRECTORY) /* no file */
            || strcasecmp(&entry->d_name[ext_pos], file_thumbnail_ext))
        {   /* or doesn't end with ".talk", no candidate */
            continue; 
        }
        
        /* terminate the (disposable) name in dir buffer,
           this truncates off the ".talk" without needing an extra buffer */
        entry->d_name[ext_pos] = '\0';
        
        /* search corresponding file in dir cache */
        for (i=0; i<num_files; i++)
        {
            if (!strcasecmp(dircache[i].name, entry->d_name))
            {   /* match */
                dircache[i].attr |= TREE_ATTR_THUMBNAIL; /* set the flag */
                break; /* exit search loop, because we found it */
            }
        }
    }
    closedir(dir);
}

/* check all dircache directories if they contain a "_dirname.talk" file */
#if 0 /* not practical, this is too slow */
void check_dir_thumbnails(const char *dirname, int num_files)
{
    int i;
    int fd;
    char clipfile[MAX_PATH];

    for (i=0; i<num_files; i++)
    {
        if (!(dircache[i].attr & ATTR_DIRECTORY))  
            continue; /* only directories are interesting */
        
        /* compose pathname of directory name clip file */
        snprintf(clipfile, sizeof(clipfile), "%s%s/%s",
            dirname, dircache[i].name, dir_thumbnail_name);

        fd = open(clipfile, O_RDONLY); /* check if exists */
        if (fd >= 0)
        {   /* there is one */
            dircache[i].attr |= TREE_ATTR_THUMBNAIL; /* set the flag */
            close(fd);
        }
        else
        {   /* none found, clear the flag */
            dircache[i].attr &= ~TREE_ATTR_THUMBNAIL;
        }
    }
}
#endif /* #if 0 */

/* load sorted directory into dircache.  returns NULL on failure. */
struct entry* load_and_sort_directory(const char *dirname, const int *dirfilter,
                                      int *num_files, bool *buffer_full)
{
    int i;

    DIR *dir = opendir(dirname);
    if(!dir)
        return NULL; /* not a directory */

    name_buffer_length = 0;
    dirsindir = 0;
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

        /* check for known file types */
        if ( !(dptr->attr & ATTR_DIRECTORY) && (len > 4) )
           dptr->attr |= filetype_get_attr(entry->d_name);

        /* memorize/compare details about the boot file */
        if ((currdir[1] == 0) && !strcasecmp(entry->d_name, BOOTFILE)) {
            if (boot_size) {
                if ((entry->size != boot_size) ||
                    (entry->startcluster != boot_cluster))
                    boot_changed = true;
            }
            boot_size = entry->size;
            boot_cluster = entry->startcluster;
        }

        /* filter out non-visible files */
        if (!(dptr->attr & ATTR_DIRECTORY) && (
            (*dirfilter == SHOW_PLAYLIST &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_M3U) ||
            ((*dirfilter == SHOW_MUSIC &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_MPA) &&
             (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_M3U) ||
            (*dirfilter == SHOW_SUPPORTED && !filetype_supported(dptr->attr)) ||
            (*dirfilter == SHOW_WPS && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_WPS) ||
            (*dirfilter == SHOW_CFG && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_CFG) ||
            (*dirfilter == SHOW_LNG && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_LNG) ||
            (*dirfilter == SHOW_MOD && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_MOD) ||
            (*dirfilter == SHOW_FONT && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_FONT) ||
            (*dirfilter == SHOW_PLUGINS && (dptr->attr & TREE_ATTR_MASK) != TREE_ATTR_ROCK)))
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
        dptr->time_write = entry->wrtdate<<16 | entry->wrttime; /* in one # */
        strcpy(dptr->name,entry->d_name);
        name_buffer_length += len + 1;

        if (dptr->attr & ATTR_DIRECTORY) /* count the remaining dirs */
            dirsindir++; 
    }
    *num_files = i;
    closedir(dir);
    strncpy(lastdir,dirname,sizeof(lastdir));
    lastdir[sizeof(lastdir)-1] = 0;
    qsort(dircache,i,sizeof(struct entry),compare);

    /* If thumbnail talking is enabled, make an extra run to mark files with 
       associated thumbnails, so we don't do unsuccessful spinups later. */
    if (global_settings.talk_file == 3)
        check_file_thumbnails(dirname, i); /* map .talk to ours */
#if 0 /* not practical, this is too slow */
    if (global_settings.talk_dir == 3)
        check_dir_thumbnails(dirname, i); /* try in the directories */
#endif /* #if 0 */
    return dircache;
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
    
    if(global_settings.buttonbar)
        height -= BUTTONBAR_HEIGHT;

    return height / fh;
}
#endif

static int showdir(const char *path, int start, const int *dirfilter)
{
    int i;
    int tree_max_on_screen;
    bool dir_buffer_full;

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
        if ( i >= filesindir )
            break;

        icon = filetype_get_icon(dircache[i].attr);

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

        showfileline(i-start, i, false, dirfilter); /* no scroll */
    }

#ifdef HAVE_LCD_BITMAP
    if (global_settings.scrollbar && (filesindir > tree_max_on_screen))
        scrollbar(SCROLLBAR_X, SCROLLBAR_Y, SCROLLBAR_WIDTH - 1,
                  tree_max_on_screen * line_height, filesindir, start,
                  start + tree_max_on_screen, VERTICAL);

#if CONFIG_KEYPAD == RECORDER_PAD
    if(global_settings.buttonbar) {
        buttonbar_set(*dirfilter < NUM_FILTER_MODES ?
                      str(LANG_DIRBROWSE_F1) : (unsigned char *) "",
                      str(LANG_DIRBROWSE_F2),
                      str(LANG_DIRBROWSE_F3));
        buttonbar_draw();
    }
#endif
#endif
    status_draw(true);

    return filesindir;
}

static bool ask_resume(bool ask_once)
{
    int button;
    bool stop = false;
    static bool ignore_power = true;

#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
#endif

    if (usb_detect()) {
        default_event_handler(SYS_USB_CONNECTED);
        return false;
    }
    
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
        button = button_get(true);
        switch (button) {
#ifdef TREE_RUN_PRE
            case TREE_RUN_PRE:  /* catch the press, not the release */
#else
            case TREE_RUN:
#endif
#ifdef TREE_RC_RUN
            case TREE_RC_RUN:
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
                if(default_event_handler(button) || !(button & BUTTON_REL))
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

static bool dirbrowse(const char *root, const int *dirfilter)
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
    long thumbnail_time = -1; /* for delaying a thumbnail */
    bool update_all = false; /* set this to true when the whole file list
                                has been refreshed on screen */
    int lastbutton = 0;

#ifdef HAVE_LCD_BITMAP
    tree_max_on_screen = recalc_screen_height();
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

    if (*dirfilter > NUM_FILTER_MODES && numentries==0)
    {
        splash(HZ*2, true, str(LANG_NO_FILES));
        return false;  /* No files found for rockbox_browser() */
    }

    update_all = true;

    put_cursorxy(CURSOR_X, CURSOR_Y + dircursor, true);

    while(1) {
        struct entry* file = &dircache[dircursor+dirstart];

        bool restore = false;

        button = button_get_w_tmo(HZ/5);

#ifndef SIMULATOR
        if (boot_changed) {
            bool stop = false;
            int button;

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
            case TREE_EXIT:
            case TREE_EXIT | BUTTON_REPEAT:
#ifdef TREE_RC_EXIT
            case TREE_RC_EXIT:
#endif
                i=strlen(currdir);
                if (i>1) {
                    while (currdir[i-1]!='/')
                        i--;
                    strcpy(buf,&currdir[i]);
                    if (i==1)
                        currdir[i]=0;
                    else
                        currdir[i-1]=0;

                    if (*dirfilter > NUM_FILTER_MODES && dirlevel < 1)
                        exit_func = true;

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
                else
                {
                    if (*dirfilter > NUM_FILTER_MODES && dirlevel < 1)
                        exit_func = true;
                }
                break;

#ifdef TREE_OFF
#ifndef HAVE_SW_POWEROFF
            case TREE_OFF:
                if (*dirfilter < NUM_FILTER_MODES)
                {
                    /* Stop the music if it is playing, else show the shutdown
                       screen */
                    if(mpeg_status())
                        mpeg_stop();
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
            case TREE_OFF | BUTTON_REPEAT:
                if (charger_inserted()) {
                    charging_splash();
                    restore = true;
                }
                break;
#endif

#ifdef TREE_ENTER
            case TREE_ENTER:
            case TREE_ENTER | BUTTON_REPEAT:
#endif
#ifdef TREE_RC_ENTER
            case TREE_RC_ENTER:
#endif
            case TREE_RUN:
#ifdef TREE_RUN_PRE
                if ((button == TREE_RUN) &&
                    (lastbutton != TREE_RUN_PRE))
                    break;
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
                            if (bookmark_autoload(buf))
                            {
                                restore = true;
                                break;
                            }

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
                            if (bookmark_autoload(currdir))
                            {
                                restore = true;
                                break;
                            }

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
                            tree_max_on_screen = recalc_screen_height();
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

                        case TREE_ATTR_BMARK:
                            bookmark_load(buf, false);
                            restore = true;
                            reload_dir = true;
                            break;

                        case TREE_ATTR_LNG:
                            if(!lang_load(buf)) {
                                set_file(buf, global_settings.lang_file,
                                         MAX_FILENAME);
                                talk_init(); /* use voice of same language */
                                splash(HZ, true, str(LANG_LANGUAGE_LOADED));
                                restore = true;

                                language_changed = true;
                            }
                            break;

#ifdef HAVE_LCD_BITMAP
                        case TREE_ATTR_FONT:
                            font_load(buf);
                            set_file(buf, global_settings.font_file,
                                     MAX_FILENAME);

                            tree_max_on_screen = recalc_screen_height();
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
#endif

                            /* plugin file */
                        case TREE_ATTR_ROCK:
                            if (plugin_load(buf,NULL) == PLUGIN_USB_CONNECTED)
                                reload_root = true;
                            else
                                restore = true;
                            break;

                        default:
                        {
                            char* plugin = filetype_get_plugin(file);
                            if (plugin)
                            {
                                if (plugin_load(plugin,buf) == PLUGIN_USB_CONNECTED)
                                    reload_root = true;
                                else
                                    restore = true;
                            }
                            break;
                        }
                    }

                    if ( play ) {
                        if ( global_settings.resume ) {
                            /* the resume_index must always be the index in the
                               shuffled list in case shuffle is enabled */
                            global_settings.resume_index = start_index;
                            global_settings.resume_offset = 0;
                            settings_save();
                        }

                        start_wps = true;
                    }
                    else if (*dirfilter > NUM_FILTER_MODES &&
                             *dirfilter != SHOW_FONT &&
                             *dirfilter != SHOW_PLUGINS)
                        exit_func = true;
                }
                restore = true;
                break;

            case TREE_PREV:
            case TREE_PREV | BUTTON_REPEAT:
#ifdef TREE_RC_PREV
            case TREE_RC_PREV:
            case TREE_RC_PREV | BUTTON_REPEAT:
#endif
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
#ifdef TREE_RC_NEXT
            case TREE_RC_NEXT:
            case TREE_RC_NEXT | BUTTON_REPEAT:
#endif
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

#ifdef TREE_PGUP
            case TREE_PGUP:
            case TREE_PGUP | BUTTON_REPEAT:
                if ( dirstart ) {
                    dirstart -= tree_max_on_screen;
                    if ( dirstart < 0 )
                        dirstart = 0;
                }
                else
                    dircursor = 0;
                restore = true;
                break;

            case TREE_PGDN:
            case TREE_PGDN | BUTTON_REPEAT:
                if ( dirstart < numentries - tree_max_on_screen ) {
                    dirstart += tree_max_on_screen;
                    if ( dirstart >
                         numentries - tree_max_on_screen )
                        dirstart = numentries - tree_max_on_screen;
                }
                else
                    dircursor = numentries - dirstart - 1;
                restore = true;
                break;
#endif

            case TREE_MENU:
                if (*dirfilter < NUM_FILTER_MODES)
                {
                    lcd_stop_scroll();
                    if (main_menu())
                        reload_root = true;
                    restore = true;
                }
                break;

            case TREE_WPS:
#ifdef TREE_WPS_PRE
                if (lastbutton != TREE_WPS_PRE)
                    break;
#endif
                /* don't enter wps from plugin browser etc */
                if (*dirfilter < NUM_FILTER_MODES)
                {
                    if (mpeg_status() & MPEG_STATUS_PLAY)
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
                if (*dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen(CONTEXT_TREE, BUTTON_F2))
                        reload_root = true;
                    restore = true;
                    break;
                }

            case BUTTON_F3:
                if (*dirfilter < NUM_FILTER_MODES)
                {
                    if (quick_screen(CONTEXT_TREE, BUTTON_F3))
                        reload_root = true;
                    tree_max_on_screen = recalc_screen_height();
                    restore = true;
                }
                break;
#endif

            case TREE_CONTEXT:
#ifdef TREE_CONTEXT2
            case TREE_CONTEXT2:
#endif
            {
                int onplay_result;

                if(!numentries)
                    onplay_result = onplay(NULL, 0);
                else {
                    if (currdir[1])
                        snprintf(buf, sizeof buf, "%s/%s",
                                 currdir, dircache[dircursor+dirstart].name);
                    else
                        snprintf(buf, sizeof buf, "/%s",
                                 dircache[dircursor+dirstart].name);
                    onplay_result = onplay(buf,
                                           dircache[dircursor+dirstart].attr);
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
                        res = play_dirname(lasti);
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
                        play_filename(currdir, dircache[lasti].name);
                    }
                    thumbnail_time = -1; /* job done */
                }
                status_draw(false);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    reload_root = true;
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
                reload_root = true;
#ifdef HAVE_LCD_BITMAP
            tree_max_on_screen = recalc_screen_height();
#endif
            restore = true;
            start_wps=false;
        }

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
            while (button_get(false)); /* clear button queue */
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
                thumbnail_time = -1; /* cancel whatever we were about to say */

                showfileline(dircursor, i, true, dirfilter); /* scroll please */
                need_update = true;

                if (dircache[i].attr & ATTR_DIRECTORY) /* directory? */
                {
                    /* play directory thumbnail */
                    if (global_settings.talk_dir == 3) /* hover */
                    {   /* "schedule" a thumbnail, to have a little dalay */
                        thumbnail_time = current_tick + HOVER_DELAY;
                    }
                    else if (global_settings.talk_dir == 1) /* dirs as numbers */
                    {
                        talk_id(VOICE_DIR, false);
                        talk_number(i+1, true);
                    }
                    else if (global_settings.talk_dir == 2) /* dirs spelled */
                    {
                        talk_spell(dircache[i].name, false);
                    }
                }
                else /* file */
                {
                    int voicemethod = global_settings.talk_file;
                    if (voicemethod == 3) /* thumbnail clip */
                    {   /* "schedule" a thumbnail, to have a little delay */
                        if (dircache[i].attr & TREE_ATTR_THUMBNAIL)
                        {
                            thumbnail_time = current_tick + HOVER_DELAY;
                        }
                        else
                        {   /* say the number as fallback */
                            voicemethod = 1;
                        }
                    }
                    if (voicemethod == 1) /* files as numbers */
                    {
                        play_filenumber(i-dirsindir+1, 
                                        dircache[i].attr & TREE_ATTR_MASK);
                    }
                    else if (voicemethod == 2) /* files spelled */
                    {
                        talk_spell(dircache[i].name, false);
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
            if ((!strcasecmp(&entry->d_name[x-4], ".mp3")) ||
                (!strcasecmp(&entry->d_name[x-4], ".mp2")) ||
                (!strcasecmp(&entry->d_name[x-4], ".mpa")))
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

    snprintf(filename, sizeof(filename), "%s", currdir[1] ? currdir : "/");
    plsize = 0;
    add_dir(filename, sizeof(filename), fd);
    close(fd);
    sleep(HZ);

    return true;
}

bool rockbox_browse(const char *root, int dirfilter)
{
    bool rc;
    int dircursor_save = dircursor;
    int dirstart_save = dirstart;
    int dirlevel_save = dirlevel;
    int dirpos_save = dirpos[0];
    int cursorpos_save = cursorpos[0];

    memcpy(currdir_save, currdir, sizeof(currdir));
    reload_dir = true;
    start_wps = false;
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

void bookmark_play(char *resume_file, int index, int offset, int seed)
{
    int len=strlen(resume_file);

    if (!strcasecmp(&resume_file[len-4], ".m3u"))
    {
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
        lastdir[0]='\0';
        if (playlist_create(resume_file, NULL) != -1)
        {
            resume_directory(resume_file);
            if (global_settings.playlist_shuffle)
                playlist_shuffle(seed, -1);
            playlist_start(index,offset);
        }
    }

    start_wps=true;
}
