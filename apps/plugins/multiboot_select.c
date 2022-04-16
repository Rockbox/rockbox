/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2022 Aidan MacDonald
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

#include "plugin.h"

/* should be more than enough */
#define MAX_ROOTS 128

static enum plugin_status plugin_status = PLUGIN_OK;
static char tmpbuf[MAX_PATH+1];
static char tmpbuf2[MAX_PATH+1];
static char cur_root[MAX_PATH];
static char roots[MAX_ROOTS][MAX_PATH];
static int nroots;

/* Read a redirect file and return the path */
static char* read_redirect_file(const char* filename)
{
    int fd = rb->open(filename, O_RDONLY);
    if(fd < 0)
        return NULL;

    ssize_t ret = rb->read(fd, tmpbuf, sizeof(tmpbuf));
    rb->close(fd);
    if(ret < 0 || (size_t)ret >= sizeof(tmpbuf))
        return NULL;

    /* relative paths are ignored */
    if(tmpbuf[0] != '/')
        ret = 0;

    /* remove trailing control chars (newlines etc.) */
    for(ssize_t i = ret - 1; i >= 0 && tmpbuf[i] < 0x20; --i)
        tmpbuf[i] = '\0';

    return tmpbuf;
}

/* Search for a redirect file, like get_redirect_dir() */
static const char* read_redirect(void)
{
    for(int vol = NUM_VOLUMES-1; vol >= MULTIBOOT_MIN_VOLUME; --vol) {
        rb->snprintf(tmpbuf, sizeof(tmpbuf), "/<%d>/%s", vol, BOOT_REDIR);
        const char* path = read_redirect_file(tmpbuf);
        if(path && path[0] == '/') {
            /* prepend the volume because that's what we expect */
            rb->snprintf(tmpbuf2, sizeof(tmpbuf2), "/<%d>%s", vol, path);
            return tmpbuf2;
        }
    }

    tmpbuf[0] = '\0';
    return tmpbuf;
}

/* Remove all redirect files except for the one on volume 'exclude_vol' */
static int clear_redirect(int exclude_vol)
{
    int ret = 0;
    for(int vol = MULTIBOOT_MIN_VOLUME; vol < NUM_VOLUMES; ++vol) {
        if(vol == exclude_vol)
            continue;

        rb->snprintf(tmpbuf, sizeof(tmpbuf), "/<%d>/%s", vol, BOOT_REDIR);
        if(rb->file_exists(tmpbuf) && rb->remove(tmpbuf) < 0)
            ret = -1;
    }

    return ret;
}

/* Save a path to the redirect file */
static int write_redirect(const char* abspath)
{
    /* get the volume (required) */
    const char* path = abspath;
    int vol = rb->path_strip_volume(abspath, &path, false);
    if(path == abspath)
        return -1;

    /* remove all other redirect files */
    if(clear_redirect(vol))
        return -1;

    /* open the redirect file on the same volume */
    rb->snprintf(tmpbuf, sizeof(tmpbuf), "/<%d>/%s", vol, BOOT_REDIR);
    int fd = rb->open(tmpbuf, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(fd < 0)
        return fd;

    size_t pathlen = rb->strlen(path);
    ssize_t ret = rb->write(fd, path, pathlen);
    if(ret < 0 || (size_t)ret != pathlen) {
        rb->close(fd);
        return -1;
    }

    ret = rb->write(fd, "\n", 1);
    rb->close(fd);
    if(ret != 1)
        return -1;

    return 0;
}

/* Check if the firmware file is valid
 * TODO: this should at least check model number or something */
static bool check_firmware(const char* path)
{
    return rb->file_exists(path);
}

static int root_compare(const void* a, const void* b)
{
    const char* as = a;
    const char* bs = b;
    return rb->strcmp(as, bs);
}

/* Scan the filesystem for possible redirect targets. To prevent this from
 * taking too long we only check the directories in the root of each volume
 * look check for a rockbox firmware binary underneath /dir/.rockbox. If it
 * exists then we report /<vol#>/dir as a root. */
static int find_roots(void)
{
    const char* bootdir = *BOOTDIR == '/' ? &BOOTDIR[1] : BOOTDIR;
    nroots = 0;

    for(int vol = MULTIBOOT_MIN_VOLUME; vol < NUM_VOLUMES; ++vol) {
        rb->snprintf(tmpbuf, sizeof(tmpbuf), "/<%d>", vol);

        /* try to open the volume root; ignore failures since they'll
         * occur if the volume is unmounted */
        DIR* dir = rb->opendir(tmpbuf);
        if(!dir)
            continue;

        struct dirent* ent;
        while((ent = rb->readdir(dir))) {
            int r = rb->snprintf(tmpbuf, sizeof(tmpbuf), "/<%d>/%s/%s/%s",
                                 vol, ent->d_name, bootdir, BOOTFILE);
            if(r < 0 || (size_t)r >= sizeof(tmpbuf))
                continue;

            if(check_firmware(tmpbuf)) {
                rb->snprintf(roots[nroots], MAX_PATH, "/<%d>/%s",
                             vol, ent->d_name);
                nroots += 1;

                /* quit if we hit the maximum */
                if(nroots == MAX_ROOTS) {
                    vol = NUM_VOLUMES;
                    break;
                }
            }
        }

        rb->closedir(dir);
    }

    rb->qsort(roots, nroots, MAX_PATH, root_compare);
    return nroots;
}

static const char* picker_get_name_cb(int selected, void* data,
                                      char* buffer, size_t buffer_len)
{
    (void)data;
    (void)buffer;
    (void)buffer_len;
    return roots[selected];
}

static int picker_action_cb(int action, struct gui_synclist* lists)
{
    (void)lists;
    if(action == ACTION_STD_OK)
        action = ACTION_STD_CANCEL;
    return action;
}

static bool show_picker_menu(int* selection)
{
    struct simplelist_info info;
    rb->simplelist_info_init(&info, "Select new root", nroots, NULL);
    info.selection = *selection;
    info.get_name = picker_get_name_cb;
    info.action_callback = picker_action_cb;
    if(rb->simplelist_show_list(&info))
        return true;

    if(0 <= info.selection && info.selection < nroots)
        *selection = info.selection;

    return false;
}

enum {
    MB_SELECT_ROOT,
    MB_CLEAR_REDIRECT,
    MB_CURRENT_ROOT,
    MB_SAVE_AND_EXIT,
    MB_SAVE_AND_REBOOT,
    MB_EXIT,
    MB_NUM_ITEMS,
};

static const char* menu_get_name_cb(int selected, void* data,
                                    char* buffer, size_t buffer_len)
{
    (void)data;

    switch(selected) {
    case MB_SELECT_ROOT:
        return "Select root";

    case MB_CLEAR_REDIRECT:
        return "Clear redirect";

    case MB_CURRENT_ROOT:
        if(cur_root[0]) {
            rb->snprintf(buffer, buffer_len, "Using root: %s", cur_root);
            return buffer;
        }

        return "Using default root";

    case MB_SAVE_AND_EXIT:
        return "Save and Exit";

    case MB_SAVE_AND_REBOOT:
        return "Save and Reboot";

    case MB_EXIT:
    default:
        return "Exit";
    }
}

static int menu_action_cb(int action, struct gui_synclist* lists)
{
    int selected = rb->gui_synclist_get_sel_pos(lists);

    if(action != ACTION_STD_OK)
        return action;

    switch(selected) {
    case MB_SELECT_ROOT: {
        if(find_roots() <= 0) {
            rb->splashf(3*HZ, "No roots found");
            break;
        }

        int root_sel = nroots-1;
        for(; root_sel > 0; --root_sel)
            if(!rb->strcmp(roots[root_sel], cur_root))
                break;

        if(show_picker_menu(&root_sel))
            return SYS_USB_CONNECTED;

        rb->strcpy(cur_root, roots[root_sel]);
        action = ACTION_REDRAW;
    } break;

    case MB_CLEAR_REDIRECT:
        if(cur_root[0]) {
            memset(cur_root, 0, sizeof(cur_root));
            rb->splashf(HZ, "Cleared redirect");
        }

        action = ACTION_REDRAW;
        break;

    case MB_SAVE_AND_REBOOT:
    case MB_SAVE_AND_EXIT: {
        int ret;
        if(cur_root[0])
            ret = write_redirect(cur_root);
        else
            ret = clear_redirect(-1);

        if(ret < 0)
            rb->splashf(3*HZ, "Couldn't save settings");
        else
            rb->splashf(HZ, "Settings saved");

        action = ACTION_STD_CANCEL;
        plugin_status = (ret < 0) ? PLUGIN_ERROR : PLUGIN_OK;

        if(ret >= 0 && selected == MB_SAVE_AND_REBOOT)
            rb->sys_reboot();
    } break;

    case MB_EXIT:
        return ACTION_STD_CANCEL;

    default:
        action = ACTION_REDRAW;
        break;
    }

    return action;
}

static bool show_menu(void)
{
    struct simplelist_info info;
    rb->simplelist_info_init(&info, "Multiboot Settings", MB_NUM_ITEMS, NULL);
    info.get_name = menu_get_name_cb;
    info.action_callback = menu_action_cb;
    return rb->simplelist_show_list(&info);
}

enum plugin_status plugin_start(const void* param)
{
    (void)param;

    /* load the current root */
    const char* myroot = read_redirect();
    rb->strcpy(cur_root, myroot);

    /* display the menu */
    if(show_menu())
        return PLUGIN_USB_CONNECTED;
    else
        return plugin_status;
}
