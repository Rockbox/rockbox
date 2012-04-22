/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2012 by Jonathan Gordon
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
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "file.h"
#include "fat.h"
#include "dir.h"
#include "debug.h"
#include "dircache.h"
#include "filefuncs.h"
#include "system.h"
#include "filescanner.h"

static struct event_queue filescanner_queue SHAREDBSS_ATTR;
static long filescanner_stack[(DEFAULT_STACK_SIZE + 0x4000)/sizeof(long)];
static const char filescanner_thread_name[] = "filescanner";

static long kill_scan;
static bool in_scan;

enum {
    Q_STOP_SCAN = 0,
    Q_START_SCAN,
};

/*
 * User stacks.
 *
 * Some users of this scanner will require a stack to keep track of some
 * recursive state. (i.e whether the user cares about the current subtree.)
 * Instead of requireing each user to manage their own stack implementation,
 * users can call filescanner_request_stack_var(size) which will allocate
 * size bytes in each recursion level for that users use.
 */
#define MAX_STACK_VARS 8
#define MAX_STACK_VARS_SIZE 64
struct user_stack_var {
    short offset;
    short size;
};
static struct user_stack_var user_stack_vars[MAX_STACK_VARS];
static int user_stack_var_count;
static int user_stack_var_size;
static void *current_user_stack, *previous_user_stack;

/*
 * returns -1 on error, or a handle to pass to filescanner_get_stack_var()
 * to get the pointer to the data.
 */
int filescanner_request_stack_var(size_t size)
{
    size_t new_size = ALIGN_UP(size, sizeof(int));
    if (user_stack_var_count >= MAX_STACK_VARS)
        return -1;
    else if (in_scan)
        return -2;
    user_stack_vars[user_stack_var_count].offset = user_stack_var_size;
    user_stack_vars[user_stack_var_count].size = size;
    user_stack_var_size += new_size;

    user_stack_var_count++;
    return user_stack_var_count - 1;
}

void *filescanner_get_stack_var(int handle, size_t *size)
{
    if (!in_scan || handle < 0 || handle >= user_stack_var_count)
        return NULL;

    if (size)
        *size = user_stack_vars[handle].size;

    return current_user_stack + user_stack_vars[handle].offset;
}

static void filescanner_scan_dir(char *path)
{
    DIR *dir;
    int len = strlen(path);
    struct file_scan_dir_data dir_data;
    char user_stack[user_stack_var_size];
    void *local_previous = previous_user_stack;

    dir = opendir(path);
    if (!dir)
    {
        DEBUGF("Couldn't open Directory %s\n", path);
        return;
    }
    dir_data.dir = dir;
    dir_data.path = path;
    previous_user_stack = current_user_stack;
    current_user_stack = &user_stack[0];
    if (previous_user_stack)
        memcpy(current_user_stack, previous_user_stack, user_stack_var_size);
    else
        memset(current_user_stack, 0, user_stack_var_size);
    send_event(FILE_SCANNER_ENTER_DIRECTORY, &dir_data);

    while (!kill_scan)
    {
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
            break;

        if (!strcmp((char *)entry->d_name, ".") ||
            !strcmp((char *)entry->d_name, ".."))
            continue;

        struct dirinfo info = dir_get_info(dir, entry);

        /* Not sure if yield() is necessary as the ata thread should
         * cause this thread to block/yield anyway. Defintly need to
         * yield on sim though.
         */
        yield();
        /* don't add an extra / for curpath == / */
        if (len <= 1) len = 0;
        snprintf(&path[len], MAX_PATH - len, "/%s", entry->d_name);
        
        if (info.attribute & ATTR_DIRECTORY)
            filescanner_scan_dir(path);
        else
        {
            struct file_scan_file_data data;
            data.filename = path;
            data.info = &info;
            data.dir = dir;
            send_event(FILE_SCANNER_FILE, &data);
        }
    }

    closedir(dir);
    path[len] = '\0';
    send_event(FILE_SCANNER_EXIT_DIRECTORY, path);
    current_user_stack = previous_user_stack;
    previous_user_stack = local_previous;
}

static void filescanner_thread(void)
{
    struct queue_event ev;
    char buffer[MAX_PATH];
    char user_stack_var_storage[MAX_STACK_VARS_SIZE];

    while (1)
    {
        queue_wait_w_tmo(&filescanner_queue, &ev, HZ);
        
        switch (ev.id)
        {
            case Q_START_SCAN:
                in_scan = true;
                previous_user_stack = NULL;
                kill_scan = 0;
                cpu_boost(true);
                memset(user_stack_var_storage, 0, user_stack_var_size);
                current_user_stack = user_stack_var_storage;
                previous_user_stack = user_stack_var_storage;
                send_event(FILE_SCANNER_START, (void*)ev.data);

                strcpy(buffer, "/");
                filescanner_scan_dir(buffer);

                current_user_stack = user_stack_var_storage;
                send_event(FILE_SCANNER_FINISH, &kill_scan);
                cpu_boost(false);
                in_scan = false;
                break;

            case Q_STOP_SCAN:
                kill_scan = 1;
                break;
        }
    }
}


void filescanner_init(void)
{
    user_stack_var_count = 0;
    in_scan = false;
    create_thread(filescanner_thread, filescanner_stack,
                  sizeof(filescanner_stack), 0, filescanner_thread_name 
                  IF_PRIO(, PRIORITY_BACKGROUND)
                  IF_COP(, CPU));

}

void filescanner_scan(long reinit)
{
    queue_post(&filescanner_queue, Q_START_SCAN, reinit);
}

void filescanner_abort(void)
{
    queue_post(&filescanner_queue, Q_STOP_SCAN, 0);
}
