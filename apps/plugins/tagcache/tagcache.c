/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2023 by William Wilgus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*Plugin Includes*/

#include "plugin.h"
#include "errno.h"

/* Redefinitons of ANSI C functions. */
#include "lib/wrappers.h"
#include "lib/helper.h"

static void thread_create(void);
static void thread(void); /* the thread running commit*/
static void allocate_tempbuf(void);
static void free_tempbuf(void);
static bool do_timed_yield(void);
static void _log(const char *fmt, ...);
static bool logdump(bool append);
/*Aliases*/
#if 0
#ifdef ROCKBOX_HAS_LOGF
    #define logf         rb->logf
#else
    #define logf(...)   {}
#endif
#endif

#define logf _log
#define sleep rb->sleep
#define qsort rb->qsort

#define write(x,y,z)   rb->write(x,y,z)
#define ftruncate rb->ftruncate
#define remove rb->remove
#define rename rb->rename

#define vsnprintf    rb->vsnprintf
#define mkdir        rb->mkdir
#define filesize     rb->filesize

#define strtok_r     rb->strtok_r
#define strncasecmp  rb->strncasecmp
#define strcasecmp   rb->strcasecmp

#define current_tick (*rb->current_tick)
#define crc_32(x,y,z)   rb->crc_32(x,y,z)
#define plugin_get_buffer rb->plugin_get_buffer

#define MAX_LOG_SIZE 16384

#define EV_EXIT        MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0xFF)
#define EV_STARTUP     MAKE_SYS_EVENT(SYS_EVENT_CLS_PRIVATE, 0x01)

#define BACKUP_DIRECTORY PLUGIN_APPS_DATA_DIR "/db_commit"

#define RC_SUCCESS 0
#define RC_USER_CANCEL 2

enum fcpy_op_flags
{
    FCPY_MOVE       = 0x00, /* Is a move operation (default) */
    FCPY_COPY      = 0x01, /* Is a copy operation */
    FCPY_OVERWRITE = 0x02, /* Overwrite destination */
    FCPY_EXDEV     = 0x04, /* Actually copy/move across volumes */
};

/* communication to the worker thread */
static struct
{
    int user_index;
    bool exiting; /* signal to the thread that we want to exit */
    bool resume;
    unsigned int id; /* worker thread id */
    struct  event_queue queue; /* thread event queue */
    long    last_useraction_tick;
} gThread;

/* status of db and commit */
static struct
{
    long last_check;
    bool do_commit;
    bool auto_commit;
    bool commit_ready;
    bool db_exists;
    bool have_backup;
    bool bu_exists;
} gStatus;

static unsigned char logbuffer[MAX_LOG_SIZE + 1];
static int log_font_h = -1;
static int logindex;
static bool logwrap;
static bool logenabled = true;

/*Support Fns*/
/* open but with a builtin printf for assembling the path */
int open_pathfmt(char *buf, size_t size, int oflag, const char *pathfmt, ...)
{
    va_list ap;
    va_start(ap, pathfmt);
    vsnprintf(buf, size, pathfmt, ap);
    va_end(ap);
    if ((oflag & O_PATH) == O_PATH)
        return -1;
    int handle = open(buf, oflag, 0666);
    //logf("Open: %s %d flag: %x", buf, handle, oflag);
    return handle;
}

static void sleep_yield(void)
{
    sleep(1);
    #undef yield
    rb->yield();
    #define yield sleep_yield
}

/* make sure tag can be displayed by font pf*/
static bool text_is_displayable(struct font *pf, unsigned char *src)
{
    unsigned short code;
    const unsigned char *ptr = src;
    while(*ptr)
    {
        ptr = rb->utf8decode(ptr, &code);

        if(!rb->font_get_bits(pf, code))
        {
            return false;
        }
    }
    return true;
}

/* callback for each tag if false returned tag will not be added */
bool user_check_tag(int index_type, char* build_idx_buf)
{
    static struct font *pf = NULL;
    if (!pf)
        pf = rb->font_get(rb->screens[SCREEN_MAIN]->getuifont());

    if (index_type == tag_artist || index_type == tag_album ||
        index_type == tag_genre || index_type == tag_title ||
        index_type == tag_composer || index_type == tag_comment ||
        index_type == tag_albumartist || index_type == tag_virt_canonicalartist)
    {
        /* this could be expanded with more rules / transformations */
        if (rb->utf8length(build_idx_buf) != strlen(build_idx_buf))
        {
            if (!text_is_displayable(pf, build_idx_buf))
            {
                logf("Can't display (%d) %s", index_type, build_idx_buf);
            }
        }
    }
    return true;
}

/* undef features we don't need */
#undef HAVE_DIRCACHE
#undef HAVE_TC_RAMCACHE
#undef HAVE_EEPROM_SETTINGS
/* paste the whole tagcache.c file */
#include "../tagcache.c"

static void check_logindex(void)
{
    if(logindex >= MAX_LOG_SIZE)
    {
        logdump(true);
        //logwrap = true;
        logindex = 0;
    }
}

static int log_push(void *userp, int c)
{
    (void)userp;

    logbuffer[logindex++] = c;
    check_logindex();

    return 1;
}

/* our logf function */
static void _log(const char *fmt, ...)
{
    if (!logenabled)
    {
        rb->splash(10, "log not enabled");
        return;
    }

    #ifdef USB_ENABLE_SERIAL
    int old_logindex = logindex;
    #endif
    va_list ap;

    va_start(ap, fmt);

#if (CONFIG_PLATFORM & PLATFORM_HOSTED)
    char buf[1024];
    vsnprintf(buf, sizeof buf, fmt, ap);
    DEBUGF("%s\n", buf);
    /* restart va_list otherwise the result if undefined when vuprintf is called */
    va_end(ap);
    va_start(ap, fmt);
#endif

    rb->vuprintf(log_push, NULL, fmt, ap);
    va_end(ap);

    /* add trailing zero */
    log_push(NULL, '\0');
}


int compute_nb_lines(int w, struct font* font)
{
    int i, nb_lines;
    int cur_x, delta_x;

    if(logindex>= MAX_LOG_SIZE || (logindex == 0 && !logwrap))
        return 0;

    if(logwrap)
        i = logindex;
    else
        i = 0;

    cur_x = 0;
    nb_lines = 0;

    do {
        if(logbuffer[i] == '\0')
        {
            cur_x = 0;
            nb_lines++;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = rb->font_get_width(font, logbuffer[i]);

            if(cur_x + delta_x > w)
            {
                cur_x = 0;
                nb_lines++;
            }

            /* update pointer */
            cur_x += delta_x;
        }

        i++;
        if(i >= MAX_LOG_SIZE)
            i = 0;
    } while(i != logindex);

    return nb_lines;
}

static bool logdisplay(void)
{

    int w, h, i, index;
    int fontnr;
    int cur_x, cur_y, delta_x;
    struct font* font;

    char buf[2];

    fontnr = FONT_FIRSTUSERFONT;
    font = rb->font_get(fontnr);
    buf[1] = '\0';
    w = LCD_WIDTH;
    h = LCD_HEIGHT;

    if (log_font_h < 0) /* init, get the horizontal size of each line */
    {
        rb->font_getstringsize("A", NULL, &log_font_h, fontnr);
        /* start at the end of the log */
        gThread.user_index = compute_nb_lines(w, font) - h/log_font_h -1;
        /* user_index will be number of the first line to display (warning: line!=log entry) */
        /* if negative, will be set 0 to zero later */
    }

    rb->lcd_clear_display();

    if(gThread.user_index < 0 || gThread.user_index >= MAX_LOG_SIZE)
        gThread.user_index = 0;

    if(logwrap)
        i = logindex;
    else
        i = 0;

    index = 0;
    cur_x = 0;
    cur_y = 0;

    /* nothing to print ? */
    if(logindex == 0 && !logwrap)
        goto end_print;

    do {
        if(logbuffer[i] == '\0')
        {
            /* should be display a newline ? */
            if(index >= gThread.user_index)
                cur_y += log_font_h;
            cur_x = 0;
            index++;
        }
        else
        {
            /* does character fit on this line ? */
            delta_x = rb->font_get_width(font, logbuffer[i]);

            if(cur_x + delta_x > w)
            {
                /* should be display a newline ? */
                if(index >= gThread.user_index)
                    cur_y += log_font_h;
                cur_x = 0;
                index++;
            }

            /* should we print character ? */
            if(index >= gThread.user_index)
            {
                buf[0] = logbuffer[i];
                rb->lcd_putsxy(cur_x, cur_y, buf);
            }

            /* update pointer */
            cur_x += delta_x;
        }
        i++;
        /* did we fill the screen ? */
        if(cur_y > h - log_font_h)
        {
            if (TIME_AFTER(current_tick, gThread.last_useraction_tick + HZ))
                gThread.user_index++;
            break;
        }

        if(i >= MAX_LOG_SIZE)
            i = 0;
    } while(i != logindex);

    end_print:
    rb->lcd_update();

    return false;
}

static bool logdump(bool append)
{
    int fd;
    int flags = O_CREAT|O_WRONLY|O_TRUNC;
    /* nothing to print ? */
    if(!logenabled || (logindex == 0 && !logwrap))
    {
        /* nothing is logged just yet */
        return false;
    }
    if (append)
    {
        flags = O_CREAT|O_WRONLY|O_APPEND;
    }

    fd = open(ROCKBOX_DIR "/db_commit_log.txt", flags, 0666);
    logenabled = false;
    if(-1 != fd) {
        int i;

        if(logwrap)
            i = logindex;
        else
            i = 0;

        do {
            if(logbuffer[i]=='\0')
                rb->fdprintf(fd, "\n");
            else
                rb->fdprintf(fd, "%c", logbuffer[i]);

            i++;
            if(i >= MAX_LOG_SIZE)
                i = 0;
        } while(i != logindex);

        close(fd);
    }

    logenabled = true;

    return false;
}

static void allocate_tempbuf(void)
{
    tempbuf_size = 0;
    tempbuf = rb->plugin_get_audio_buffer(&tempbuf_size);
    tempbuf_size &= ~0x03;

}

static void free_tempbuf(void)
{
    if (tempbuf_size == 0)
        return ;

    rb->plugin_release_audio_buffer();
    tempbuf = NULL;
    tempbuf_size = 0;
}

static bool do_timed_yield(void)
{
    /* Sorting can lock up for quite a while, so yield occasionally */
    static long wakeup_tick = 0;
    if (TIME_AFTER(current_tick, wakeup_tick))
    {
        yield();

        wakeup_tick = current_tick + (HZ/5);
        return true;
    }
    return false;
}

/* copy/move a file */
static int fcpy(const char *src, const char *target,
                unsigned int flags, bool (*poll_cancel)(const char *))
{
    int rc = -1;

    while (!(flags & (FCPY_COPY | FCPY_EXDEV))) {
        if ((flags & FCPY_OVERWRITE) || !file_exists(target)) {
            /* Rename and possibly overwrite the file */
            if (poll_cancel && poll_cancel(src)) {
                rc = RC_USER_CANCEL;
            } else {
                rc = rename(src, target);
            }

        #ifdef HAVE_MULTIVOLUME
            if (rc < 0 && errno == EXDEV) {
                /* Failed because cross volume rename doesn't work; force
                   a move instead */
                flags |= FCPY_EXDEV;
                break;
            }
        #endif /* HAVE_MULTIVOLUME */
        }

        return rc;
    }

    /* See if we can get the plugin buffer for the file copy buffer */
    size_t buffersize;
    char *buffer = (char *) plugin_get_buffer(&buffersize);
    if (buffer == NULL || buffersize < 512) {
        /* Not large enough, try for a disk sector worth of stack
           instead */
        buffersize = 512;
        buffer = (char *)alloca(buffersize);
    }

    if (buffer == NULL) {
        return -1;
    }

    buffersize &= ~0x1ff;  /* Round buffer size to multiple of sector
                              size */

    int src_fd = open(src, O_RDONLY);
    if (src_fd >= 0) {
        int oflag = O_WRONLY|O_CREAT;

        if (!(flags & FCPY_OVERWRITE)) {
            oflag |= O_EXCL;
        }

        int target_fd = open(target, oflag, 0666);
        if (target_fd >= 0) {
            off_t total_size = 0;
            off_t next_cancel_test = 0; /* No excessive button polling */

            rc = RC_SUCCESS;

            while (rc == RC_SUCCESS) {
                if (total_size >= next_cancel_test) {
                    next_cancel_test = total_size + 0x10000;
                    if (poll_cancel && poll_cancel(src)) {
                       rc = RC_USER_CANCEL;
                       break;
                    }
                }

                ssize_t bytesread = read(src_fd, buffer, buffersize);
                if (bytesread <= 0) {
                    if (bytesread < 0) {
                        rc = -1;
                    }
                    /* else eof on buffer boundary; nothing to write */
                    break;
                }

                ssize_t byteswritten = write(target_fd, buffer, bytesread);
                if (byteswritten < bytesread) {
                    /* Some I/O error */
                    rc = -1;
                    break;
                }

                total_size += byteswritten;

                if (bytesread < (ssize_t)buffersize) {
                    /* EOF with trailing bytes */
                    break;
                }
            }

            if (rc == RC_SUCCESS) {
                /* If overwriting, set the correct length if original was
                   longer */
                rc = ftruncate(target_fd, total_size);
            }

            close(target_fd);

            if (rc != RC_SUCCESS) {
                /* Copy failed. Cleanup. */
                remove(target);
            }
        }

        close(src_fd);
    }

    if (rc == RC_SUCCESS && !(flags & FCPY_COPY)) {
        /* Remove the source file */
        rc = remove(src);
    }

    return rc;
}

static bool backup_restore_tagcache(bool backup)
{
    struct master_header   tcmh;
    char path[MAX_PATH];
    char bu_path[MAX_PATH];
    int fd;
    int rc;

    if (backup)
    {
        if (!rb->dir_exists(BACKUP_DIRECTORY))
            mkdir(BACKUP_DIRECTORY);
        snprintf(path, sizeof(path), "%s/"TAGCACHE_FILE_MASTER, tc_stat.db_path);
        snprintf(bu_path, sizeof(bu_path), "%s/"TAGCACHE_FILE_MASTER, BACKUP_DIRECTORY);
    }
    else
    {
        if (!rb->dir_exists(BACKUP_DIRECTORY))
            return false;
        snprintf(path, sizeof(path), "%s/"TAGCACHE_FILE_MASTER, BACKUP_DIRECTORY);
        snprintf(bu_path, sizeof(bu_path), "%s/"TAGCACHE_FILE_MASTER, tc_stat.db_path);
    }

    fd = open(path, O_RDONLY, 0666);

    if (fd >= 0)
    {
        rc = read(fd, &tcmh, sizeof(struct master_header));
        close(fd);
        if (rc != sizeof(struct master_header))
        {
            logf("master file read failed");
            return false;
        }
        int entries = tcmh.tch.entry_count;

        logf("master file %d entries", entries);
        if (backup)
            logf("backup db to %s", BACKUP_DIRECTORY);
        else
            logf("restore db to %s", tc_stat.db_path);

        if (entries > 0)
        {
            logf("%s", path);
            fcpy(path, bu_path, FCPY_COPY|FCPY_OVERWRITE, NULL);

            for (int i = 0; i < TAG_COUNT; i++)
            {
                if (TAGCACHE_IS_NUMERIC(i))
                    continue;
                snprintf(path, sizeof(path),
                         "%s/"TAGCACHE_FILE_INDEX, tc_stat.db_path, i);

                snprintf(bu_path, sizeof(bu_path),
                         "%s/"TAGCACHE_FILE_INDEX, BACKUP_DIRECTORY, i);
                /* Note: above we swapped paths in the snprintf call here we swap variables */
                if (backup)
                {
                    logf("%s", path);
                    if (fcpy(path, bu_path, FCPY_COPY|FCPY_OVERWRITE, NULL) < 0)
                        goto failed;
                    gStatus.have_backup = true;
                }
                else
                {
                    logf("%s", bu_path);
                    if (fcpy(bu_path, path, FCPY_COPY|FCPY_OVERWRITE, NULL) < 0)
                        goto failed;
                }
            }
        }
        return true;
    }
failed:
    if (backup)
    {
        logf("failed backup");
    }

    return false;
}

/* asks the user if they wish to quit */
static bool confirm_quit(void)
{
    const struct text_message prompt =
       { (const char*[]) {"Are you sure?", "This will abort commit."}, 2};
    enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
    return (response == YESNO_YES);
}

/* asks the user if they wish to backup/restore */
static bool prompt_backup_restore(bool backup)
{
    const struct text_message bu_prompt = { (const char*[]) {"Backup database?"}, 1};
    const struct text_message re_prompt =
        { (const char*[]) {"Error committing,", "Restore database?"}, 2};
    enum yesno_res response =
              rb->gui_syncyesno_run(backup ? &bu_prompt:&re_prompt, NULL, NULL);
    if(response == YESNO_YES)
        return backup_restore_tagcache(backup);
    return true;
}

static const char* list_get_name_cb(int selected_item, void* data,
                                    char* buf, size_t buf_len)
{
    (void) data;
    (void) buf;
    (void) buf_len;

    /* buf supplied isn't used so lets use it for a filename buffer */
    if (TIME_AFTER(current_tick, gStatus.last_check))
    {
        snprintf(buf, buf_len, "%s/"TAGCACHE_FILE_NOCOMMIT, tc_stat.db_path);
        gStatus.auto_commit = !file_exists(buf);
        snprintf(buf, buf_len, "%s/"TAGCACHE_FILE_TEMP, tc_stat.db_path);
        gStatus.commit_ready = file_exists(buf);
        snprintf(buf, buf_len, "%s/"TAGCACHE_FILE_MASTER, tc_stat.db_path);
        gStatus.db_exists = file_exists(buf);
        snprintf(buf, buf_len, "%s/"TAGCACHE_FILE_MASTER, BACKUP_DIRECTORY);
        gStatus.bu_exists = file_exists(buf);
        gStatus.last_check = current_tick + HZ;
        buf[0] = '\0';
    }

    switch(selected_item)
    {

        case 0: /* exit */
            return ID2P(LANG_MENU_QUIT);
        case 1: /*sep*/
            return ID2P(VOICE_BLANK);
        case 2: /*backup*/
            if (!gStatus.db_exists)
                return ID2P(VOICE_BLANK);
            return "Backup";
        case 3: /*restore*/
            if (!gStatus.bu_exists)
                return ID2P(VOICE_BLANK);
            return "Restore";
        case 4: /*sep*/
            return ID2P(VOICE_BLANK);
        case 5: /*auto commit*/
            if (gStatus.auto_commit)
                return "Disable auto commit";
            else
                return "Enable auto commit";
        case 6: /*destroy*/
            if (gStatus.db_exists)
                return "Delete database";
            /*fall through*/
        case 7: /*sep*/
            return ID2P(VOICE_BLANK);
        case 8: /*commit*/
            if (gStatus.commit_ready)
                return "Commit";
            else
                return "Nothing to commit";
        default:
            return "?";
    }
}

static int list_voice_cb(int list_index, void* data)
{
    #define MAX_MENU_NAME 32
    if (!rb->global_settings->talk_menu)
        return -1;
    else
    {
        char buf[MAX_MENU_NAME];
        const char* name = list_get_name_cb(list_index, data, buf, sizeof(buf));
        long id = P2ID((const unsigned char *)name);
        if(id>=0)
            rb->talk_id(id, true);
        else
            rb->talk_spell(name, true);
    }
    return 0;
}

static int commit_menu(void)
{
    struct gui_synclist lists;
    bool exit = false;
    int button,i;
    int selection, ret = 0;

    rb->gui_synclist_init(&lists,list_get_name_cb,0, false, 1, NULL);
    rb->gui_synclist_set_nb_items(&lists, 9);
    rb->gui_synclist_select_item(&lists, 0);

    while (!exit)
    {
        rb->gui_synclist_draw(&lists);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&lists, &button))
            continue;
        selection = rb->gui_synclist_get_sel_pos(&lists);

        if (button ==  ACTION_STD_CANCEL)
                return 0;
        else if (button ==  ACTION_STD_OK)
        {
            switch(selection)
            {
                case 0: /* exit */
                    exit = true;
                    break;
                case 1: /*sep*/
                    continue;
                case 2: /*backup*/
                    if (!gStatus.db_exists)
                        break;
                    if (!backup_restore_tagcache(true))
                        rb->splash(HZ, "Backup failed!");
                    else
                    {
                        rb->splash(HZ, "Backup success!");
                        gStatus.bu_exists = true;
                    }
                    break;
                case 3: /*restore*/
                    if (!gStatus.bu_exists)
                        break;
                    if (!backup_restore_tagcache(false))
                        rb->splash(HZ, "Restore failed!");
                    else
                        rb->splash(HZ, "Restore success!");
                    break;
                case 4: /*sep*/
                    continue;
                case 5: /*auto commit*/
                {
                    /* build_idx_buf supplied by tagcache.c isn't being used
                     * so lets use it for a filename buffer */
                    snprintf(build_idx_buf, build_idx_bufsz,
                            "%s/" TAGCACHE_FILE_NOCOMMIT, tc_stat.db_path);
                    if(gStatus.auto_commit)
                         close(open(build_idx_buf, O_WRONLY | O_CREAT | O_TRUNC, 0666));
                    else
                        remove(build_idx_buf);
                    gStatus.auto_commit = !file_exists(build_idx_buf);
                    break;
                }
                case 6: /*destroy*/
                {
                    if (!gStatus.db_exists)
                        break;
                    const struct text_message prompt =
                     { (const char*[]) {"Are you sure?", "This will destroy database."}, 2};
                    if (rb->gui_syncyesno_run(&prompt, NULL, NULL) == YESNO_YES)
                        remove_files();
                    break;
                }
                case 7: /*sep*/
                    continue;
                case 8: /*commit*/
                    if (gStatus.commit_ready)
                    {
                        gStatus.do_commit = true;
                        exit = true;
                    }
                    break;

                case MENU_ATTACHED_USB:
                    return PLUGIN_USB_CONNECTED;
                default:
                    return 0;
            }
        }
    } /*while*/
    return ret;
}

/*-----------------------------------------------------------------------------*/
/******* plugin_start *******                                                  */
/*-----------------------------------------------------------------------------*/

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;

    /* Turn off backlight timeout */
    backlight_ignore_timeout();

    memset(&gThread, 0, sizeof(gThread));
    memset(&gStatus, 0, sizeof(gStatus));
    memset(&tc_stat, 0, sizeof(struct tagcache_stat));
    memset(&current_tcmh, 0, sizeof(struct master_header));
    filenametag_fd = -1;

    strlcpy(tc_stat.db_path, rb->global_settings->tagcache_db_path, sizeof(tc_stat.db_path));
    if (!rb->dir_exists(tc_stat.db_path)) /* on fail use default DB path */
        strlcpy(tc_stat.db_path, ROCKBOX_DIR, sizeof(tc_stat.db_path));
    tc_stat.initialized = true;
    tc_stat.commit_step = -1;

    logf("started");

    int result = commit_menu();

    if (!gStatus.do_commit)
    {
        /* Turn on backlight timeout (revert to settings) */
        backlight_use_settings();
        return PLUGIN_OK;
    }

    logdump(false);
    allocate_tempbuf();
    if (gStatus.db_exists && !gStatus.have_backup && !prompt_backup_restore(true))
        rb->splash(HZ, "Backup failed!");
    thread_create();
    gThread.user_index = 0;
    logdisplay(); /* get something on the screen while user waits */

    while (!gThread.exiting)
    {
        logdisplay();

        int action = rb->get_action(CONTEXT_STD, HZ/20);

        switch( action )
        {
            case ACTION_NONE:
                break;
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index++;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index--;
                break;
            case ACTION_STD_OK:
                gThread.last_useraction_tick = current_tick;
                gThread.user_index = 0;
                break;
            case SYS_POWEROFF:
            case ACTION_STD_CANCEL:
                if (tc_stat.commit_step >= 0 && !tc_stat.ready)
                {
                    if (!confirm_quit())
                        break;
                    tc_stat.commit_delayed = true; /* Cancel the commit */
                }
                rb->queue_remove_from_head(&gThread.queue, EV_EXIT);
                rb->queue_post(&gThread.queue, EV_EXIT, 0);
                break;
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCHSCREEN:
            {
                gThread.last_useraction_tick = current_tick;
                short x, y;
                static int prev_y;

                action = rb->action_get_touchscreen_press(&x, &y);

                if(action & BUTTON_REL)
                    prev_y = 0;
                else
                {
                    if(prev_y != 0)
                        gThread.user_index += (prev_y - y) / log_font_h;

                    prev_y = y;
                }
            }
#endif
            default:
                break;
        }
        yield();
    }

    rb->thread_wait(gThread.id);
    rb->queue_delete(&gThread.queue);
    free_tempbuf();

    if (tc_stat.commit_delayed || !tc_stat.ready)
    {
        remove_files();
        if (gStatus.bu_exists && !prompt_backup_restore(false))
            rb->splash(HZ, "Restore failed!");
    }

    if (tc_stat.ready)
        rb->tagcache_commit_finalize();

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
    return PLUGIN_OK;
}

/****************** main thread + helper ******************/
static void thread(void)
{
    struct queue_event ev;
    while (!gThread.exiting)
    {
        rb->queue_wait_w_tmo(&gThread.queue, &ev, 1);
        switch (ev.id)
        {
            case SYS_USB_CONNECTED:
                rb->usb_acknowledge(SYS_USB_CONNECTED_ACK);
                logenabled = false;
                break;
            case SYS_USB_DISCONNECTED:
                logenabled = true;
                /*fall through*/
            case EV_STARTUP:
                logf("Thread Started");
                cpu_boost(true);
                if (commit())
                    tc_stat.ready = true;
                cpu_boost(false);
                logdump(true);
                gThread.user_index++;
                logdisplay();
                break;
            case EV_EXIT:
                gThread.exiting = true;
                return;
            default:
                break;
        }
        yield();
    }
}

static void thread_create(void)
{
    /* put the thread's queue in the bcast list */
    rb->queue_init(&gThread.queue, true);
    /*Note: tagcache_stack is defined in apps/tagcache.c */
    gThread.last_useraction_tick = current_tick;
    gThread.id = rb->create_thread(thread, tagcache_stack, sizeof(tagcache_stack),
                                      0, "db_commit"
                                      IF_PRIO(, PRIORITY_USER_INTERFACE)
                                      IF_COP(, CPU));
    rb->queue_post(&gThread.queue, EV_STARTUP, 0);
    yield();
}
