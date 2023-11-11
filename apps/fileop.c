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
 * Copyright (C) 2024 William Wilgus
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
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string-extra.h"
#include "debug.h"
#include "powermgmt.h"

#include "misc.h"
#include "plugin.h"
#include "dir.h"
#include "tree.h"
#include "fileop.h"
#include "pathfuncs.h"

#include "settings.h"
#include "lang.h"
#include "yesno.h"
#include "splash.h"
#include "keyboard.h"

/* Used for directory move, copy and delete */
struct file_op_params
{
    char   path[MAX_PATH]; /* Buffer for full path */
    const char*  toplevel_name;
    bool   is_dir;
    int    objects;        /* how many files and subdirectories*/
    int    processed;
    unsigned long long total_size;
    unsigned long long processed_size;
    size_t append;         /* Append position in 'path' for stack push */
    size_t extra_len;      /* Length added by dst path compared to src */
};

static int prompt_name(char* buf, size_t bufsz)
{
    if (kbd_input(buf, bufsz, NULL) < 0)
        return FORC_CANCELLED;
    /* at least prevent escapes out of the base directory from keyboard-
       entered filenames; the file code should reject other invalidities */
    if (*buf != '\0' && !strchr(buf, PATH_SEPCH) && !is_dotdir_name(buf))
        return FORC_SUCCESS;
    return FORC_UNKNOWN_FAILURE;
}

static bool poll_cancel_action(int operation, struct file_op_params *param)
{
    static unsigned long last_tick;

    if (operation == FOC_COUNT)
    {
        if (param->objects <= 1)
            last_tick = current_tick;
        else if (TIME_AFTER(current_tick, last_tick + HZ/2))
        {
            clear_screen_buffer(false);
            splashf(0, "%s (%d)", str(LANG_SCANNING_DISK), param->objects);
            last_tick = current_tick;
        }
    }
    else
    {
        const char *op_str = (operation == FOC_DELETE) ? str(LANG_DELETING) :
                             (operation == FOC_COPY) ? str(LANG_COPYING) :
                             str(LANG_MOVING);

        if ((operation == FOC_DELETE || !param->total_size) &&
            param->objects > 0)
        {
            splash_progress(param->processed, param->objects,
                            "%s %s", op_str, param->toplevel_name);
        }
        else if (param->total_size >= 10 * 1024 * 1024)
        {
            int total_shft = (int) (param->total_size >> 15);
            int current_shft = (int) (param->processed_size >> 15);
            splash_progress(current_shft, total_shft,
                            "%s %s (%d MiB)\n%d MiB",
                            op_str, param->toplevel_name,
                            total_shft >> 5, current_shft >> 5);
        }
        else if (param->total_size >= 1024)
        {
            int total_kib = (int) (param->total_size >> 10);
            int current_kib = (int) (param->processed_size >> 10);
            splash_progress(current_kib, total_kib,
                            "%s %s (%d KiB)\n%d KiB",
                            op_str, param->toplevel_name,
                            total_kib, current_kib);
        }
    }
    return ACTION_STD_CANCEL == get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
}

static void init_file_op(struct file_op_params *param,
                                    const char *basename,
                                    const char *selected_file)
{
    /* Assumes: basename will never be NULL */
    if (selected_file == NULL)
    {
        param->append = strlcpy(param->path, basename, sizeof (param->path));
        path_basename(basename, &basename);
        param->toplevel_name = basename;
    }
    else
    {
        param->append = path_append(param->path, basename,
                                    selected_file, sizeof (param->path));
        param->toplevel_name = selected_file;
    }
    param->is_dir = dir_exists(param->path);
    param->extra_len = 0;
    param->objects = 0; /* how many files and subdirectories*/
    param->processed = 0;
    param->total_size = 0;
    param->processed_size = 0;
}

/* counts file objects, deletes file objects */
static int directory_fileop(struct file_op_params *param, enum file_op_current fileop)
{
    errno = 0;
    DIR *dir = opendir(param->path);
    if (!dir) {
            if (errno == EMFILE) {
                return FORC_TOO_MANY_SUBDIRS;
            }
        return FORC_PATH_NOT_EXIST; /* open error */
    }
    int rc = FORC_SUCCESS;
    size_t append = param->append;

    /* walk through the directory content */
    while (rc == FORC_SUCCESS) {
        errno = 0; /* distinguish failure from eod */
        struct dirent *entry = readdir(dir);
        if (!entry) {
            if (errno) {
                rc = FORC_PATH_NOT_EXIST;
            }
            break;
        }

        struct dirinfo info = dir_get_info(dir, entry);
        if ((info.attribute & ATTR_DIRECTORY) &&
            is_dotdir_name(entry->d_name)) {
            continue; /* skip these */
        }

        /* append name to current directory */
        param->append = append + path_append(&param->path[append],
                                            PA_SEP_HARD, entry->d_name,
                                            sizeof (param->path) - append);

        if (fileop == FOC_DELETE)
        {
            param->processed++;
            /* at this point we've already counted and never had a path too long
               in the other branch so we 'should not' encounter one here either */

            if (param->processed > param->objects)
            {
                rc = FORC_UNKNOWN_FAILURE;
                break;
            }

            if (info.attribute & ATTR_DIRECTORY) {
                /* remove a subdirectory */
                rc = directory_fileop(param, fileop); /* recursion */
            } else {
                /* remove a file */
                if (poll_cancel_action(FOC_DELETE, param))
                {
                    rc = FORC_CANCELLED;
                    break;
                }
                rc = remove(param->path);
            }
        }
        else /* count objects */
        {
            param->objects++;
            param->total_size += info.size;

            if (poll_cancel_action(FOC_COUNT, param))
            {
                rc = FORC_CANCELLED;
                break;
            }

            if (param->append + param->extra_len >= sizeof (param->path)) {
                rc = FORC_PATH_TOO_LONG;
                break; /* no space left in buffer */
            }

            if (info.attribute & ATTR_DIRECTORY) {
                /* enter subdirectory */
                rc = directory_fileop(param, FOC_COUNT); /* recursion */
            }
        }
        param->append = append; /* other functions may use param, reset append */
        /* Remove basename we added above */
        param->path[append] = '\0';
    }

    closedir(dir);

    if (fileop == FOC_DELETE && rc == FORC_SUCCESS) {
        /* remove the now empty directory */
        if (poll_cancel_action(FOC_DELETE, param))
        {
            rc = FORC_CANCELLED;
        } else {
            rc = rmdir(param->path);
        }
    }

    return rc;
}

/* Walk a directory tree and count the number of objects (dirs & files)
 * also check that enough resources exist to do an operation */
static int check_count_fileobjects(struct file_op_params *param)
{
    cpu_boost(true);
    int rc = directory_fileop(param, FOC_COUNT);
    cpu_boost(false);
    DEBUGF("%s res:(%d) objects %d \n", __func__, rc, param->objects);
    return rc;
}

/* Attempt to just rename a file or directory */
static int move_by_rename(struct file_op_params *src,
                          const char *dst_path,
                          unsigned int *pflags)
{
    unsigned int flags = *pflags;
    int rc = FORC_UNKNOWN_FAILURE;
    reset_poweroff_timer();
    if (!(flags & (PASTE_COPY | PASTE_EXDEV))) {
        if ((flags & PASTE_OVERWRITE) || !file_exists(dst_path)) {
            /* Just try to move the directory / file */
            rc = rename(src->path, dst_path);
#ifdef HAVE_MULTIVOLUME
            if (rc < FORC_SUCCESS && errno == EXDEV) {
                /* Failed because cross volume rename doesn't work */
                *pflags |= PASTE_EXDEV; /* force a move instead */
            }
#endif /* HAVE_MULTIVOLUME */
             /* if (errno == ENOTEMPTY && (flags & PASTE_OVERWRITE)) {
              * Directory is not empty thus rename() will not do a quick overwrite */
        }

    }
    return rc;
}

/* Paste a file */
static int copy_move_file(struct file_op_params *src, const char *dst_path,
                          unsigned int flags)
{
    /* Try renaming first */
    int rc = move_by_rename(src, dst_path, &flags);
    if (rc == FORC_SUCCESS)
    {
        src->total_size = 0; /* switch from counting size to number of items */
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
        return FORC_NO_BUFFER_AVAIL;
    }

    buffersize &= ~0x1ff;  /* Round buffer size to multiple of sector size */

    int src_fd = open(src->path, O_RDONLY);
    if (src_fd >= 0) {
        off_t src_sz = lseek(src_fd, 0, SEEK_END);
        if (!src->total_size && !src->processed) /* single file copy */
            src->total_size = src_sz;
        lseek(src_fd, 0, SEEK_SET);

        int oflag = O_WRONLY|O_CREAT;

        if (!(flags & PASTE_OVERWRITE)) {
            oflag |= O_EXCL;
        }

        int dst_fd = open(dst_path, oflag, 0666);
        if (dst_fd >= 0) {
            off_t total_size = 0;
            off_t next_cancel_test = 0; /* No excessive button polling */

            rc = FORC_SUCCESS;

            while (rc == FORC_SUCCESS) {
                if (total_size >= next_cancel_test) {
                    next_cancel_test = total_size + 0x10000;
                    if (poll_cancel_action(!(flags & PASTE_COPY) ?
                                           FOC_MOVE : FOC_COPY, src))
                    {
                       rc = FORC_CANCELLED;
                       break;
                    }
                }

                ssize_t bytesread = read(src_fd, buffer, buffersize);
                if (bytesread <= 0) {
                    if (bytesread < 0) {
                        rc = FORC_READ_FAILURE;
                    }
                    /* else eof on buffer boundary; nothing to write */
                    break;
                }

                ssize_t byteswritten = write(dst_fd, buffer, bytesread);
                if (byteswritten < bytesread) {
                    /* Some I/O error */
                    rc = FORC_WRITE_FAILURE;
                    break;
                }

                total_size += byteswritten;
                src->processed_size += byteswritten;

                if (bytesread < (ssize_t)buffersize) {
                    /* EOF with trailing bytes */
                    break;
                }
            }

            if (rc == FORC_SUCCESS) {
                if (total_size != src_sz)
                    rc = FORC_UNKNOWN_FAILURE;
                else {
                /* If overwriting, set the correct length if original was longer */
                    rc = ftruncate(dst_fd, total_size) * 10;
                }
            }

            close(dst_fd);

            if (rc != FORC_SUCCESS) {
                /* Copy failed. Cleanup. */
                remove(dst_path);
            }
        }

        close(src_fd);
    }

    if (rc == FORC_SUCCESS && !(flags & PASTE_COPY)) {
        /* Remove the source file */
        rc = remove(src->path) * 10;
    }

    return rc;
}

/* Paste a directory */
static int copy_move_directory(struct file_op_params *src,
                               struct file_op_params *dst,
                                        unsigned int flags)
{
    DIR *srcdir = opendir(src->path);

    if (!srcdir)
        return FORC_PATH_NOT_EXIST;

    /* Make a directory to copy things to */
    int rc = mkdir(dst->path) * 10;
    if (rc < 0 && errno == EEXIST && (flags & PASTE_OVERWRITE)) {
        /* Exists and overwrite was approved */
        rc = FORC_SUCCESS;
    }

    size_t srcap = src->append, dstap = dst->append;

    /* Walk through the directory content; this loop will exit as soon as
       there's a problem */
    while (rc == FORC_SUCCESS) {
        errno = 0; /* Distinguish failure from eod */
        struct dirent *entry = readdir(srcdir);
        if (!entry) {
            if (errno) {
                rc = FORC_PATH_NOT_EXIST;
            }
            break;
        }

        struct dirinfo info = dir_get_info(srcdir, entry);
        if ((info.attribute & ATTR_DIRECTORY) &&
            is_dotdir_name(entry->d_name)) {
            continue; /* Skip these */
        }

        /* Append names to current directories */
        src->append = srcap +
            path_append(&src->path[srcap], PA_SEP_HARD, entry->d_name,
                        sizeof (src->path) - srcap);

        dst->append = dstap +
            path_append(&dst->path[dstap], PA_SEP_HARD, entry->d_name,
                        sizeof (dst->path) - dstap);
        /* src length was already checked by check_count_fileobjects() */
        if (dst->append >= sizeof (dst->path)) {
            rc = FORC_PATH_TOO_LONG; /* No space left in buffer */
            break;
        }

        src->processed++;
        if (src->processed > src->objects)
        {
            rc = FORC_UNKNOWN_FAILURE;
            break;
        }

        if (poll_cancel_action(!(flags & PASTE_COPY) ?
                               FOC_MOVE : FOC_COPY, src))
        {
            rc = FORC_CANCELLED;
            break;
        }

        DEBUGF("Copy %s to %s\n", src->path, dst->path);

        if (info.attribute & ATTR_DIRECTORY) {
            src->processed_size += info.size;
            /* Copy/move a subdirectory */
            rc = copy_move_directory(src, dst, flags); /* recursion */;
        } else {
            /* Copy/move a file */
            rc = copy_move_file(src, dst->path, flags);
        }

        /* Remove basenames we added above */
        src->path[srcap] = '\0';
        dst->path[dstap] = '\0';
    }

    if (rc == FORC_SUCCESS && !(flags & PASTE_COPY)) {
        /* Remove the now empty directory */
        rc = rmdir(src->path) * 10;
    }

    closedir(srcdir);
    return rc;
}

/************************************************************************************/
/* PUBLIC FUNCTIONS                                                                 */
/************************************************************************************/

/* Copy or move a file or directory see: file_op_flags */
int copy_move_fileobject(const char *src_path, const char *dst_path, unsigned int flags)
{
    if (!src_path[0])
        return FORC_NOOP;

    struct file_op_params src, dst;

    /* Figure out the name of the selection */
    const char *nameptr;
    path_basename(src_path, &nameptr);

    /* Final target is current directory plus name of selection  */
    init_file_op(&dst, dst_path, nameptr);
    if (dst.append >= sizeof (dst.path))
        return FORC_PATH_TOO_LONG;

    int rel = relate(src_path, dst.path);
    if (rel == RELATE_SAME)
        return FORC_NOOP;

    if (rel == RELATE_DIFFERENT) {
        int rc;
        if (file_exists(dst.path)) {
            /* If user chooses not to overwrite, cancel */
            if (!yesno_pop(ID2P(LANG_REALLY_OVERWRITE)))
            {
                splash(HZ, ID2P(LANG_CANCEL));
                return FORC_NOOVERWRT;
            }

            flags |= PASTE_OVERWRITE;
        }

        init_file_op(&src, src_path, NULL);
        if (src.append >= sizeof (src.path))
            return FORC_PATH_TOO_LONG;
        /* Now figure out what we're doing */
        cpu_boost(true);
        if (src.is_dir) {
            /* Copy or move a subdirectory */
            /* Try renaming first */
            rc = move_by_rename(&src, dst.path, &flags);
            if (rc < FORC_SUCCESS) {
                int extra_len = dst.append - src.append;
                if (extra_len > 0)
                    src.extra_len = extra_len;

                rc = check_count_fileobjects(&src);
                if (rc == FORC_SUCCESS) {
                    rc = copy_move_directory(&src, &dst, flags);
                }
            }
        } else {
            /* Copy or move a file */
            rc = copy_move_file(&src, dst.path, flags);
        }

        cpu_boost(false);
        DEBUGF("%s res: %d, ct: %d/%d %s\n",
               __func__, rc, src.objects, src.processed, src.path);
        return rc;
    }

    /* Else Some other relation / failure */
    DEBUGF("%s res: %d, rel: %d\n", __func__, FORC_UNKNOWN_FAILURE, rel);
    return FORC_UNKNOWN_FAILURE;
}

int create_dir(void)
{
    int rc;
    char dirname[MAX_PATH];
    size_t pathlen = path_append(dirname, getcwd(NULL, 0), PA_SEP_HARD,
                                 sizeof (dirname));
    char *basename = dirname + pathlen;

    if (pathlen >= sizeof (dirname))
        return FORC_PATH_TOO_LONG;

    rc = prompt_name(basename, sizeof (dirname) - pathlen);
    if (rc == FORC_SUCCESS)
        rc = mkdir(dirname) * 10;
    return rc;
}

/* share code for file and directory deletion, saves space */
int delete_fileobject(const char *selected_file)
{
    int rc;
    struct file_op_params param;
    init_file_op(&param, selected_file, NULL);
    if (param.append >= sizeof (param.path))
        return FORC_PATH_TOO_LONG;

    /* Note: delete_fileobject() will happily delete whatever
     * path is passed (after confirmation) */
    if (confirm_delete_yesno(param.path) != YESNO_YES) {
        return FORC_CANCELLED;
    }

    if (param.is_dir) {
        int rc = check_count_fileobjects(&param);
        DEBUGF("%s res: %d, ct: %d, %s", __func__, rc, param.objects, param.path);
        if (rc != FORC_SUCCESS)
            return rc;
    }

    clear_screen_buffer(true);

    if (param.is_dir) { /* if directory */
        cpu_boost(true);
        rc = directory_fileop(&param, FOC_DELETE);
        cpu_boost(false);
    } else {
        param.objects = param.processed = 1;
        if (poll_cancel_action(FOC_DELETE, &param))
            return FORC_CANCELLED;
        rc = remove(param.path) * 10;
    }

    return rc;
}

int rename_file(const char *selected_file)
{
    int rc;
    char newname[MAX_PATH];
    char *newext = NULL;
    const char *oldbase, *selection = selected_file;

    path_basename(selection, &oldbase);
    size_t pathlen = oldbase - selection;
    char *newbase = newname + pathlen;

    if (strmemccpy(newname, selection, sizeof (newname)) == NULL)
        return FORC_PATH_TOO_LONG;

    if ((*tree_get_context()->dirfilter > NUM_FILTER_MODES) &&
        (newext = strrchr(newbase, '.')))
        /* hide extension when renaming in lists restricted to a
        single file format, such as in the Playlists menu */
        *newext = '\0';

    rc = prompt_name(newbase, sizeof (newname) - pathlen);

    if (rc != FORC_SUCCESS)
        return rc;

    if (newext) /* re-add original extension */
        strlcat(newbase, strrchr(selection, '.'), sizeof (newname) - pathlen);

    if (!strcmp(oldbase, newbase))
        return FORC_NOOP; /* No change at all */

    int rel = relate(selection, newname);
    if (rel == RELATE_DIFFERENT)
    {
        if (file_exists(newname)) { /* don't overwrite */
            return FORC_PATH_EXISTS;
        }
        return rename(selection, newname) * 10;
    }
    if (rel == RELATE_SAME)
        return rename(selection, newname) * 10;

    /* Else Some other relation / failure */
    return FORC_UNKNOWN_FAILURE;
}
