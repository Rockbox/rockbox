/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Linus Nielsen Feltzing
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
#include "configfile.h"

static void get_cfg_filename(char* buf, int buf_len, const char* filename)
{
    char *s;
    rb->strcpy(buf, rb->plugin_get_current_filename());
    s = rb->strrchr(buf, '/');
    if (!s) /* should never happen */
    {
        rb->snprintf(buf, buf_len, PLUGIN_DIR "/%s", filename);
    }
    else
    {
        s++;
        *s = '\0';
        rb->strcat(s, filename);
    }
}

int configfile_save(const char *filename, struct configdata *cfg,
                    int num_items, int version)
{
    int fd;
    int i;
    char buf[MAX_PATH];

    get_cfg_filename(buf, MAX_PATH, filename);
    fd = rb->creat(buf);
    if(fd < 0)
        return fd*10 - 1;

    /* pre-allocate 10 bytes for INT */
    rb->fdprintf(fd, "file version: %10d\n", version);
    
    for(i = 0;i < num_items;i++) {
        switch(cfg[i].type) {
            case TYPE_INT:
                /* pre-allocate 10 bytes for INT */
                rb->fdprintf(fd, "%s: %10d\n",
                                cfg[i].name,
                                *cfg[i].int_p);
                break;

            case TYPE_BOOL:
                rb->fdprintf(fd, "%s: %10d\n",
                                cfg[i].name,
                                (int)*cfg[i].bool_p);
                break;

            case TYPE_ENUM:
                rb->fdprintf(fd, "%s: %s\n",
                                cfg[i].name,
                                cfg[i].values[*cfg[i].int_p]);
                break;

            case TYPE_STRING:
                rb->fdprintf(fd, "%s: %s\n",
                                cfg[i].name,
                                cfg[i].string);
                break;

        }
    }

    rb->close(fd);
    return 0;
}

int configfile_load(const char *filename, struct configdata *cfg,
                    int num_items, int min_version)
{
    int fd;
    int i, j;
    char *name;
    char *val;
    char buf[MAX_PATH];
    int file_version = -1;
    int tmp;

    get_cfg_filename(buf, MAX_PATH, filename);
    fd = rb->open(buf, O_RDONLY);
    if(fd < 0)
        return fd*10 - 1;

    while(rb->read_line(fd, buf, MAX_PATH) > 0) {
        rb->settings_parseline(buf, &name, &val);

        /* Bail out if the file version is too old */
        if(!rb->strcmp("file version", name)) {
            file_version = rb->atoi(val);
            if(file_version < min_version) {
                rb->close(fd);
                return -1;
            }
        }
        
        for(i = 0;i < num_items;i++) {
            if(!rb->strcmp(cfg[i].name, name)) {
                switch(cfg[i].type) {
                    case TYPE_INT:
                        tmp = rb->atoi(val);
                        /* Only set it if it's within range */
                        if(tmp >= cfg[i].min && tmp <= cfg[i].max)
                            *cfg[i].int_p = tmp;
                        break;

                    case TYPE_BOOL:
                        tmp = rb->atoi(val);
                        *cfg[i].bool_p = (bool)tmp;
                        break;

                    case TYPE_ENUM:
                        for(j = 0;j < cfg[i].max;j++) {
                            if(!rb->strcmp(cfg[i].values[j], val)) {
                                *cfg[i].int_p = j;
                            }
                        }
                        break;

                    case TYPE_STRING:
                        rb->strncpy(cfg[i].string, val, cfg[i].max);
                        break;
                }
            }
        }
    }
    
    rb->close(fd);
    return 0;
}

int configfile_get_value(const char* filename, const char* name)
{
    int fd;
    char *pname;
    char *pval;
    char buf[MAX_PATH];

    get_cfg_filename(buf, MAX_PATH, filename);
    fd = rb->open(buf, O_RDONLY);
    if(fd < 0)
        return -1;

    while(rb->read_line(fd, buf, MAX_PATH) > 0)
    {
        rb->settings_parseline(buf, &pname, &pval);
        if(!rb->strcmp(name, pname))
        {
          rb->close(fd);
          return rb->atoi(pval);
        }
    }

    rb->close(fd);
    return -1;
}

int configfile_update_entry(const char* filename, const char* name, int val)
{
    int fd;
    char *pname;
    char *pval;
    char path[MAX_PATH];
    char buf[256];
    int found = 0;
    int line_len = 0;
    int pos = 0;
    
    /* open the current config file */
    get_cfg_filename(path, MAX_PATH, filename);
    fd = rb->open(path, O_RDWR);
    if(fd < 0)
        return -1;
    
    /* read in the current stored settings */
    while((line_len = rb->read_line(fd, buf, 256)) > 0)
    {
        rb->settings_parseline(buf, &pname, &pval);
        if(!rb->strcmp(name, pname))
        {
            found = 1;
            rb->lseek(fd, pos, SEEK_SET);
            /* pre-allocate 10 bytes for INT */
            rb->fdprintf(fd, "%s: %10d\n", pname, val);
            break;
        }
        pos += line_len;
    }
    
    /* if (name/val) is a new entry just append to file */
    if (found == 0)
        /* pre-allocate 10 bytes for INT */
        rb->fdprintf(fd, "%s: %10d\n", name, val);
    
    rb->close(fd);
    
    return found;
}
