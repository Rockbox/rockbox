/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "nvp-nwz.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

static unsigned long find_model_id(void)
{
    /* try with the environment variable */
    const char *mid = getenv("ICX_MODEL_ID");
    if(mid == NULL)
        return 0;
    char *end;
    unsigned long v = strtoul(mid, &end, 0);
    if(*end)
        return 0;
    else
        return v;
}

unsigned long nwz_get_model_id(void)
{
    static unsigned long model_id = 0xffffffff;
    if(model_id == 0xffffffff)
        model_id = find_model_id();
    return model_id;
}

const char *nwz_get_model_name(void)
{
    for(int i = 0; i < NWZ_MODEL_COUNT; i++)
        if(nwz_model[i].mid == nwz_get_model_id())
            return nwz_model[i].name;
    return NULL;
}

static int find_series(void)
{
    for(int i = 0; i < NWZ_SERIES_COUNT; i++)
        for(int j = 0; j < nwz_series[i].mid_count; j++)
            if(nwz_series[i].mid[j] == nwz_get_model_id())
                return i;
    return -1;
}

int nwz_get_series(void)
{
    static int series = -2;
    if(series == -2)
        series = find_series();
    return series;
}

static nwz_nvp_index_t *get_nvp_index(void)
{
    static nwz_nvp_index_t *index = 0;
    if(index == 0)
    {
        int series = nwz_get_series();
        index = series < 0 ? 0 : nwz_series[series].nvp_index;
    }
    return index;
}

int nwz_nvp_read(enum nwz_nvp_node_t node, void *data)
{
    int size = nwz_nvp[node].size;
    if(data == 0)
        return size;
    nwz_nvp_index_t *index = get_nvp_index();
    if(index == 0 || (*index)[node] == NWZ_NVP_INVALID)
        return -1;
    char nvp_path[32];
    snprintf(nvp_path, sizeof(nvp_path), "/dev/icx_nvp/%03d", (*index)[node]);
    int fd = open(nvp_path, O_RDONLY);
    if(fd < 0)
        return -1;
    int cnt = read(fd, data, size);
    close(fd);
    return cnt == size ? size : -1;
}

int nwz_nvp_write(enum nwz_nvp_node_t node, void *data)
{
    int size = nwz_nvp[node].size;
    nwz_nvp_index_t *index = get_nvp_index();
    if(index == 0 || (*index)[node] == NWZ_NVP_INVALID)
        return -1;
    char nvp_path[32];
    snprintf(nvp_path, sizeof(nvp_path), "/dev/icx_nvp/%03d", (*index)[node]);
    int fd = open(nvp_path, O_WRONLY);
    if(fd < 0)
        return -1;
    int cnt = write(fd, data, size);
    close(fd);
    return cnt == size ? 0 : -1;
}
