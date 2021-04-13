/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct jz_paramlist {
    int size;
    char** keys;
    char** values;
};

static int jz_paramlist_extend(jz_paramlist* pl, int count)
{
    int nsize = pl->size + count;

    /* Reallocate key list */
    char** nkeys = realloc(pl->keys, nsize * sizeof(char*));
    if(!nkeys)
        return JZ_ERR_OUT_OF_MEMORY;

    for(int i = pl->size; i < nsize; ++i)
        nkeys[i] = NULL;

    pl->keys = nkeys;

    /* Reallocate value list */
    char** nvalues = realloc(pl->values, nsize * sizeof(char*));
    if(!nvalues)
        return JZ_ERR_OUT_OF_MEMORY;

    for(int i = pl->size; i < nsize; ++i)
        nvalues[i] = NULL;

    pl->values = nvalues;

    pl->size = nsize;
    return JZ_SUCCESS;
}

jz_paramlist* jz_paramlist_new(void)
{
    jz_paramlist* pl = malloc(sizeof(struct jz_paramlist));
    if(!pl)
        return NULL;

    pl->size = 0;
    pl->keys = NULL;
    pl->values = NULL;
    return pl;
}

void jz_paramlist_free(jz_paramlist* pl)
{
    for(int i = 0; i < pl->size; ++i) {
        free(pl->keys[i]);
        free(pl->values[i]);
    }

    if(pl->size > 0) {
        free(pl->keys);
        free(pl->values);
    }

    free(pl);
}

int jz_paramlist_set(jz_paramlist* pl, const char* param, const char* value)
{
    int pos = -1;
    for(int i = 0; i < pl->size; ++i) {
        if(!pl->keys[i] || !strcmp(pl->keys[i], param)) {
            pos = i;
            break;
        }
    }

    if(pos == -1) {
        pos = pl->size;
        int rc = jz_paramlist_extend(pl, 1);
        if(rc < 0)
            return rc;
    }

    bool need_key = (pl->keys[pos] == NULL);
    if(need_key) {
        char* newparam = strdup(param);
        if(!newparam)
            return JZ_ERR_OUT_OF_MEMORY;

        pl->keys[pos] = newparam;
    }

    char* newvalue = strdup(value);
    if(!newvalue) {
        if(need_key) {
            free(pl->keys[pos]);
            pl->keys[pos] = NULL;
        }

        return JZ_ERR_OUT_OF_MEMORY;
    }

    pl->values[pos] = newvalue;
    return JZ_SUCCESS;
}

const char* jz_paramlist_get(jz_paramlist* pl, const char* param)
{
    for(int i = 0; i < pl->size; ++i)
        if(pl->keys[i] && !strcmp(pl->keys[i], param))
            return pl->values[i];

    return NULL;
}
