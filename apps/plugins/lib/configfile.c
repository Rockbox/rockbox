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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "configfile.h"

static struct plugin_api *cfg_rb;

void configfile_init(struct plugin_api* newrb)
{
    cfg_rb = newrb;
}

int configfile_save(const char *filename, struct configdata *cfg,
                    int num_items, int version)
{
    int fd;
    int i;
    char buf[MAX_PATH];

    cfg_rb->snprintf(buf, MAX_PATH, "/.rockbox/rocks/%s", filename);
    fd = cfg_rb->creat(buf, 0);
    if(fd < 0)
        return fd*10 - 1;

    cfg_rb->fdprintf(fd, "file version: %d\n", version);
    
    for(i = 0;i < num_items;i++) {
        switch(cfg[i].type) {
            case TYPE_INT:
                cfg_rb->fdprintf(fd, "%s: %d\n",
                                cfg[i].name,
                                *cfg[i].val);
                break;

            case TYPE_ENUM:
                cfg_rb->fdprintf(fd, "%s: %s\n",
                                cfg[i].name,
                                cfg[i].values[*cfg[i].val]);
                break;
                
            case TYPE_STRING:
                cfg_rb->fdprintf(fd, "%s: %s\n",
                                cfg[i].name,
                                cfg[i].string);
                break;

        }
    }

    cfg_rb->close(fd);
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

    cfg_rb->snprintf(buf, MAX_PATH, "/.rockbox/rocks/%s", filename);
    fd = cfg_rb->open(buf, O_RDONLY);
    if(fd < 0)
        return fd*10 - 1;

    while(cfg_rb->read_line(fd, buf, MAX_PATH) > 0) {
        cfg_rb->settings_parseline(buf, &name, &val);

        /* Bail out if the file version is too old */
        if(!cfg_rb->strcmp("file version", name)) {
            file_version = cfg_rb->atoi(val);
            if(file_version < min_version) {
                cfg_rb->close(fd);
                return -1;
            }
        }
        
        for(i = 0;i < num_items;i++) {
            if(!cfg_rb->strcmp(cfg[i].name, name)) {
                switch(cfg[i].type) {
                    case TYPE_INT:
                        tmp = cfg_rb->atoi(val);
                        /* Only set it if it's within range */
                        if(tmp >= cfg[i].min && tmp <= cfg[i].max)
                            *cfg[i].val = tmp;
                        break;
                        
                    case TYPE_ENUM:
                        for(j = 0;j < cfg[i].max;j++) {
                            if(!cfg_rb->strcmp(cfg[i].values[j], val)) {
                                *cfg[i].val = j;
                            }
                        }
                        break;
                        
                    case TYPE_STRING:
                        cfg_rb->strncpy(cfg[i].string, val, cfg[i].max);
                        break;
                }
            }
        }
    }
    
    cfg_rb->close(fd);
    return 0;
}
