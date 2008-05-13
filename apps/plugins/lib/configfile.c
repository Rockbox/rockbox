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

static const struct plugin_api *cfg_rb;

void configfile_init(const struct plugin_api* newrb)
{
    cfg_rb = newrb;
}

static void get_cfg_filename(char* buf, int buf_len, const char* filename)
{
    char *s;
    cfg_rb->strcpy(buf, cfg_rb->plugin_get_current_filename());
    s = cfg_rb->strrchr(buf, '/');
    if (!s) /* should never happen */
    {
        cfg_rb->snprintf(buf, buf_len, PLUGIN_DIR "/%s", filename);
    }
    else
    {
        s++;
        *s = '\0';
        cfg_rb->strcat(s, filename);
    }
}

int configfile_save(const char *filename, struct configdata *cfg,
                    int num_items, int version)
{
    int fd;
    int i;
    char buf[MAX_PATH];

    get_cfg_filename(buf, MAX_PATH, filename);
    fd = cfg_rb->creat(buf);
    if(fd < 0)
        return fd*10 - 1;

    /* pre-allocate 10 bytes for INT */
    cfg_rb->fdprintf(fd, "file version: %10d\n", version);
    
    for(i = 0;i < num_items;i++) {
        switch(cfg[i].type) {
            case TYPE_INT:
                /* pre-allocate 10 bytes for INT */
                cfg_rb->fdprintf(fd, "%s: %10d\n",
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

    get_cfg_filename(buf, MAX_PATH, filename);
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

int configfile_get_value(const char* filename, const char* name)
{
    int fd;
    char *pname;
    char *pval;
    char buf[MAX_PATH];

    get_cfg_filename(buf, MAX_PATH, filename);
    fd = cfg_rb->open(buf, O_RDONLY);
    if(fd < 0)
        return -1;

    while(cfg_rb->read_line(fd, buf, MAX_PATH) > 0)
    {
        cfg_rb->settings_parseline(buf, &pname, &pval);
        if(!cfg_rb->strcmp(name, pname))
        {
          cfg_rb->close(fd);
          return cfg_rb->atoi(pval);
        }
    }

    cfg_rb->close(fd);
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
    fd = cfg_rb->open(path, O_RDWR);
    if(fd < 0)
        return -1;
    
    /* read in the current stored settings */
    while((line_len = cfg_rb->read_line(fd, buf, 256)) > 0)
    {
        cfg_rb->settings_parseline(buf, &pname, &pval);
        if(!cfg_rb->strcmp(name, pname))
        {
            found = 1;
            cfg_rb->lseek(fd, pos, SEEK_SET);
            /* pre-allocate 10 bytes for INT */
            cfg_rb->fdprintf(fd, "%s: %10d\n", pname, val);
            break;
        }
        pos += line_len;
    }
    
    /* if (name/val) is a new entry just append to file */
    if (found == 0)
        /* pre-allocate 10 bytes for INT */
        cfg_rb->fdprintf(fd, "%s: %10d\n", name, val);
    
    cfg_rb->close(fd);
    
    return found;
}
