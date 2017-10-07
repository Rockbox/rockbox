/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "alsa-controls.h"
#include "panic.h"
#include <stdlib.h>

/* alsa control handle, we keep it open at all times */
static snd_ctl_t *alsa_ctl;
/* list of all controls, we allocate and fill it once, so we can easily lookup */
static snd_ctl_elem_list_t *alsa_ctl_list;

void alsa_controls_init(void)
{
    snd_ctl_elem_info_t *info;
    /* allocate list on heap because it is used it is used in other functions */
    snd_ctl_elem_list_malloc(&alsa_ctl_list);
    /* allocate info on stack so there is no need to free them */
    snd_ctl_elem_info_alloca(&info);
    /* there is only one card and "default" always works */
    if(snd_ctl_open(&alsa_ctl, "default", 0) < 0)
        panicf("Cannot open ctl\n");
    /* ALSA is braindead: first "get" the list -> only retrieve count */
    if(snd_ctl_elem_list(alsa_ctl, alsa_ctl_list) < 0)
        panicf("Cannot get element list\n");
    /* now we can allocate the list since the know the size */
    int count = snd_ctl_elem_list_get_count(alsa_ctl_list);
    if(snd_ctl_elem_list_alloc_space(alsa_ctl_list, count) < 0)
        panicf("Cannot allocate space for element list\n");
    /* ... and get the list again! */
    if(snd_ctl_elem_list(alsa_ctl, alsa_ctl_list) < 0)
        panicf("Cannot get element list\n");
}

void alsa_controls_close(void)
{
    snd_ctl_close(alsa_ctl);
}

/* find a control element ID by name, return false of not found, the id needs
 * to be allocated */
bool alsa_controls_find(snd_ctl_elem_id_t *id, const char *name)
{
    /* ALSA identifies controls by "id"s, it almost makes sense, except ids
     * are a horrible opaque structure */
    int count = snd_ctl_elem_list_get_count(alsa_ctl_list);
    /* enumerate controls */
    for(int i = 0; i < count; i++)
    {
        snd_ctl_elem_list_get_id(alsa_ctl_list, i, id);

        if(strcmp(snd_ctl_elem_id_get_name(id), name) == 0)
            return true;
    }
    /* not found */
    return false;
}

bool alsa_has_control(const char *name)
{
    snd_ctl_elem_id_t *id;
    /* allocate things on stack */
    snd_ctl_elem_id_alloca(&id);
    /* find control */
    return alsa_controls_find(id, name);
}

/* find a control element enum index by name, return -1 if not found */
int alsa_controls_find_enum(const char *name, const char *enum_name)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    /* allocate things on stack to speedup */
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    /* find control */
    if(!alsa_controls_find(id, name))
        panicf("Cannot find control '%s'", name);
    snd_ctl_elem_info_set_id(info, id);
    if(snd_ctl_elem_info(alsa_ctl, info) < 0)
        panicf("Cannot get control '%s' info", name);
    /* list items */
    unsigned count = snd_ctl_elem_info_get_items(info);
    for(unsigned i = 0; i < count; i++)
    {
        snd_ctl_elem_info_set_item(info, i);
        if(snd_ctl_elem_info(alsa_ctl, info) < 0)
            panicf("Cannot get control '%s' info for item %u", name, i);
        if(strcmp(snd_ctl_elem_info_get_item_name(info), enum_name) == 0)
            return i;
    }
    return -1;
}

/* set/get a control, potentially supports several values */
void alsa_controls_get_set(bool get, const char *name, snd_ctl_elem_type_t type,
    unsigned nr_values, long *val)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *value;
    /* allocate things on stack to speedup */
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&value);
    /* find control */
    if(!alsa_controls_find(id, name))
        panicf("Cannot find control '%s'", name);
    /* check the type of the control */
    snd_ctl_elem_info_set_id(info, id);
    if(snd_ctl_elem_info(alsa_ctl, info) < 0)
        panicf("Cannot get control '%s' info", name);
    if(snd_ctl_elem_info_get_type(info) != type)
        panicf("Control '%s' has wrong type (got %d, expected %d", name,
            snd_ctl_elem_info_get_type(info), type);
    if(snd_ctl_elem_info_get_count(info) != nr_values)
        panicf("Control '%s' has wrong count (got %u, expected %u)",
            name, snd_ctl_elem_info_get_count(info), nr_values);
    snd_ctl_elem_value_set_id(value, id);
    /* set value */
    if(get)
    {
        /* read value */
        if(snd_ctl_elem_read(alsa_ctl, value) < 0)
            panicf("Cannot read control '%s'", name);
        for(unsigned i = 0; i < nr_values; i++)
        {
            /* ALSA is braindead: there are "typed" setters but they all take long anyway */
            if(type == SND_CTL_ELEM_TYPE_BOOLEAN)
                val[i] = snd_ctl_elem_value_get_boolean(value, i);
            else if(type == SND_CTL_ELEM_TYPE_INTEGER)
                val[i] = snd_ctl_elem_value_get_integer(value, i);
            else if(type == SND_CTL_ELEM_TYPE_ENUMERATED)
                val[i] = snd_ctl_elem_value_get_enumerated(value, i);
        }
    }
    else
    {
        for(unsigned i = 0; i < nr_values; i++)
        {
            /* ALSA is braindead: there are "typed" setters but they all take long anyway */
            if(type == SND_CTL_ELEM_TYPE_BOOLEAN)
                snd_ctl_elem_value_set_boolean(value, i, val[i]);
            else if(type == SND_CTL_ELEM_TYPE_INTEGER)
                snd_ctl_elem_value_set_integer(value, i, val[i]);
            else if(type == SND_CTL_ELEM_TYPE_ENUMERATED)
                snd_ctl_elem_value_set_enumerated(value, i, val[i]);
        }
        /* write value */
        if(snd_ctl_elem_write(alsa_ctl, value) < 0)
            panicf("Cannot write control '%s'", name);
    }
}

/* set a control, potentially supports several values */
void alsa_controls_set(const char *name, snd_ctl_elem_type_t type,
    unsigned nr_values, long *val)
{
    return alsa_controls_get_set(false, name, type, nr_values, val);
}

/* get a control, potentially supports several values */
void alsa_controls_get(const char *name, snd_ctl_elem_type_t type,
    unsigned nr_values, long *val)
{
    return alsa_controls_get_set(true, name, type, nr_values, val);
}

/* get control information */
void alsa_controls_get_info(const char *name, int *out_count)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    /* allocate things on stack to speedup */
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    /* find control */
    if(!alsa_controls_find(id, name))
        panicf("Cannot find control '%s'", name);
    /* get info */
    snd_ctl_elem_info_set_id(info, id);
    if(snd_ctl_elem_info(alsa_ctl, info) < 0)
        panicf("Cannot get control '%s' info", name);
    *out_count = snd_ctl_elem_info_get_count(info);
}

/* helper function: set a control with a single boolean value */
void alsa_controls_set_bool(const char *name, bool val)
{
    long lval = val;
    alsa_controls_set(name, SND_CTL_ELEM_TYPE_BOOLEAN, 1, &lval);
}

/* helper function: set a control with a single enum value */
void alsa_controls_set_enum(const char *name, const char *enum_name)
{
    long idx = alsa_controls_find_enum(name, enum_name);
    if(idx < 0)
        panicf("Cannot find enum '%s' for control '%s'", enum_name, name);
    alsa_controls_set(name, SND_CTL_ELEM_TYPE_ENUMERATED, 1, &idx);
}

/* helper function: set a control with one or more integers */
void alsa_controls_set_ints(const char *name, int count, long *val)
{
    return alsa_controls_set(name, SND_CTL_ELEM_TYPE_INTEGER, count, val);
}

/* helper function: get a control with a single boolean value */
bool alsa_controls_get_bool(const char *name)
{
    long lval = 0;
    alsa_controls_get(name, SND_CTL_ELEM_TYPE_BOOLEAN, 1, &lval);
    return lval != 0;
}
