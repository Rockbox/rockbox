/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (c) 2016 Amaury Pouly
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

#include "logf.h"
#include "system.h"
#include "kernel.h"
#include "string.h"
#include "stdio.h"
#include "audio.h"
#include "sound.h"
#include "audiohw.h"
#include "cscodec.h"
#include "nwzlinux_codec.h"
#include "stdlib.h"
#include "panic.h"
#include <alsa/asoundlib.h>
#include <alloca.h>
#include <sys/ioctl.h>
#include "nwz_audio.h"

/* This driver handle the Sony linux audio drivers: despite using many differents
 * codecs, it appears that they all share a common interface and common controls. */

/* This is the alsa mixer interface exposed by Sony:
numid=3,iface=MIXER,name='Capture Src Switch'
  ; type=ENUMERATED,access=rw------,values=1,items=4
  ; Item #0 'None'
  ; Item #1 'Line'
  ; Item #2 'Fm'
  ; Item #3 'Mic'
  : values=0
numid=2,iface=MIXER,name='Playback Src Switch'
  ; type=ENUMERATED,access=rw------,values=1,items=7
  ; Item #0 'None'
  ; Item #1 'Music'
  ; Item #2 'Video'
  ; Item #3 'Tv'
  ; Item #4 'Fm'
  ; Item #5 'Line'
  ; Item #6 'Mic'
  : values=1
numid=1,iface=MIXER,name='Playback Volume'
  ; type=INTEGER,access=rw------,values=2,min=0,max=100,step=1
  : values=5,5
numid=7,iface=MIXER,name='CODEC Acoustic Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=on
numid=8,iface=MIXER,name='CODEC Cue/Rev Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off
numid=9,iface=MIXER,name='CODEC Fade In Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off
numid=6,iface=MIXER,name='CODEC Mute Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off
numid=5,iface=MIXER,name='CODEC Power Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=on
numid=10,iface=MIXER,name='CODEC Stanby Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=off
numid=4,iface=MIXER,name='Output Switch'
  ; type=ENUMERATED,access=rw------,values=1,items=4
  ; Item #0 'Headphone'
  ; Item #1 'LineVariable'
  ; Item #2 'LineFixed'
  ; Item #3 'Speaker'
  : values=0
*/

/* alsa control handle, we keep it open at all times */
static snd_ctl_t *alsa_ctl;
/* list of all controls, we allocate and fill it once, so we can easily lookup */
static snd_ctl_elem_list_t *alsa_ctl_list;
/* file descriptor of the icx_noican device */
static int fd_noican;
/* file descriptor of the hardware sound device */
static int fd_hw;

/* open alsa control interface and list all controls, keep control open */
static void alsa_init_controls(void)
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

static void alsa_close_controls(void)
{
    snd_ctl_close(alsa_ctl);
}

/* find a control element ID by name, return false of not found, the id needs
 * to be allocated */
static bool alsa_find_control(snd_ctl_elem_id_t *id, const char *name)
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

/* find a control element enum index by name, return -1 if not found */
static int alsa_find_control_enum(const char *name, const char *enum_name)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    /* allocate things on stack to speedup */
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    /* find control */
    if(!alsa_find_control(id, name))
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

/* set a control, potentially supports several values */
static void alsa_set_control(const char *name, snd_ctl_elem_type_t type,
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
    if(!alsa_find_control(id, name))
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
    /* set value */
    snd_ctl_elem_value_set_id(value, id);
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

/* get control information */
static void alsa_get_control_info(const char *name, int *out_count)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    /* allocate things on stack to speedup */
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    /* find control */
    if(!alsa_find_control(id, name))
        panicf("Cannot find control '%s'", name);
    /* get info */
    snd_ctl_elem_info_set_id(info, id);
    if(snd_ctl_elem_info(alsa_ctl, info) < 0)
        panicf("Cannot get control '%s' info", name);
    *out_count = snd_ctl_elem_info_get_count(info);
}

/* helper function: set a control with a single boolean value */
static void alsa_set_control_bool(const char *name, bool val)
{
    long lval = val;
    alsa_set_control(name, SND_CTL_ELEM_TYPE_BOOLEAN, 1, &lval);
}

/* helper function: set a control with a single enum value */
static void alsa_set_control_enum(const char *name, const char *enum_name)
{
    long idx = alsa_find_control_enum(name, enum_name);
    if(idx < 0)
        panicf("Cannot find enum '%s' for control '%s'", enum_name, name);
    alsa_set_control(name, SND_CTL_ELEM_TYPE_ENUMERATED, 1, &idx);
}

/* helper function: set a control with one or more integers */
static void alsa_set_control_ints(const char *name, int count, long *val)
{
    return alsa_set_control(name, SND_CTL_ELEM_TYPE_INTEGER, count, val);
}

static void noican_init(void)
{
    fd_noican = open(NWZ_NC_DEV, O_RDWR);
    if(fd_noican < 0)
        panicf("Cannot open '%s'", NWZ_NC_DEV);
}

static void noican_close(void)
{
    close(fd_noican);
}

/* Set NC switch */
static void noican_set_switch(int sw)
{
    if(ioctl(fd_noican, NWZ_NC_SET_SWITCH, &sw) < 0)
        panicf("ioctl(NWZ_NC_SET_SWITCH) failed");
}

/* Get NC switch */
static int noican_get_switch(void)
{
    int val;
    if(ioctl(fd_noican, NWZ_NC_GET_SWITCH, &val) < 0)
        panicf("ioctl(NWZ_NC_GET_SWITCH) failed");
    return val;
}

/* Get HP status */
static int noican_get_hp_status(void)
{
    int val;
    if(ioctl(fd_noican, NWZ_NC_GET_HP_STATUS, &val) < 0)
        panicf("ioctl(NWZ_NC_GET_HP_STATUS) failed");
    return val;
}

/* Set HP type */
static void noican_set_hp_type(int type)
{
    if(ioctl(fd_noican, NWZ_NC_SET_HP_TYPE, &type) < 0)
        panicf("ioctl(NWZ_NC_SET_HP_TYPE) failed");
}

/* Get HP type */
static int noican_get_hp_type(void)
{
    int val;
    if(ioctl(fd_noican, NWZ_NC_GET_HP_TYPE, &val) < 0)
        panicf("ioctl(NWZ_NC_GET_HP_TYPE) failed");
    return val;
}


/* Set gain */
static void noican_set_gain(int gain)
{
    if(ioctl(fd_noican, NWZ_NC_SET_GAIN, &gain) < 0)
        panicf("ioctl(NWZ_NC_SET_GAIN) failed");
}

/* Get gain */
static int noican_get_gain(void)
{
    int val;
    if(ioctl(fd_noican, NWZ_NC_GET_GAIN, &val) < 0)
        panicf("ioctl(NWZ_NC_GET_GAIN) failed");
    return val;
}

/* Set filter */
static void noican_set_filter(int filter)
{
    if(ioctl(fd_noican, NWZ_NC_SET_FILTER, &filter) < 0)
        panicf("ioctl(NWZ_NC_SET_FILTER) failed");
}

/* Get filter */
static int noican_get_filter(void)
{
    int val;
    if(ioctl(fd_noican, NWZ_NC_GET_FILTER, &val) < 0)
        panicf("ioctl(NWZ_NC_GET_FILTER) failed");
    return val;
}

static void hw_open(void)
{
    fd_hw = open("/dev/snd/hwC0D0", O_RDWR);
    if(fd_hw < 0)
        panicf("Cannot open '/dev/snd/hwC0D0'");
}

static void hw_close(void)
{
    close(fd_hw);
}

static void hw_get_clear_stereo(struct nwz_cst_param_t *cst)
{
    if(ioctl(fd_hw, NWZ_AUDIO_GET_CLRSTRO_VAL, cst) < 0)
        panicf("ioctl(NWZ_AUDIO_GET_CLRSTRO_VAL) failed\n");
}

void audiohw_preinit(void)
{
    alsa_init_controls();
    /* turn on codec */
    alsa_set_control_bool("CODEC Power Switch", true);
    /* mute */
    alsa_set_control_bool("CODEC Mute Switch", true);
    /* Acoustic and Cue/Rev control how the volume curve, but it is not clear
     * what the intention of these modes are and the OF does not seem to use
     * them by default */
    alsa_set_control_bool("CODEC Acoustic Switch", false);
    alsa_set_control_bool("CODEC Cue/Rev Switch", false);
    /* not sure exactly what it means */
    alsa_set_control_enum("Playback Src Switch", "Music");
    /* use headphone output */
    alsa_set_control_enum("Output Switch", "Headphone");
    /* unmute */
    alsa_set_control_bool("CODEC Mute Switch", false);

    /* init noican */
    noican_init();
    /* dump configuration, for debug purposes */
    printf("nc hp status: %d\n", noican_get_hp_status());
    printf("nc type: %d\n", noican_get_hp_type());
    printf("nc switch: %d\n", noican_get_switch());
    printf("nc gain: %d\n", noican_get_gain());
    printf("nc filter: %d\n", noican_get_filter());
    /* make sure we start in a clean state */
    noican_set_switch(NWZ_NC_SWITCH_OFF);
    noican_set_hp_type(NC_HP_TYPE_DEFAULT);
    noican_set_filter(NWZ_NC_FILTER_INDEX_0);
    noican_set_gain(NWZ_NC_GAIN_CENTER);

    /* init hw */
    hw_open();
    struct nwz_cst_param_t cst;
    hw_get_clear_stereo(&cst);
    printf("cst: %d, %d\n", cst.no_ext_cbl, cst.with_ext_cbl);
}

void audiohw_set_acoustic(bool val)
{
    alsa_set_control_bool("CODEC Acoustic Switch", val);
}

void audiohw_set_cuerev(bool val)
{
    alsa_set_control_bool("CODEC Cue/Rev Switch", val);
}

void audiohw_postinit(void)
{
}

void audiohw_set_volume(int vol_l, int vol_r)
{
    long vol[2];
    /* the driver expects percent, convert from centibel in range 0...x */
    vol[0] = vol_l / 10;
    vol[1] = vol_r / 10;
    /* on some recent players like A10, Sony decided to merge left/right volume
     * into one, thus we need to make sure we write the correct number of values */
    int vol_cnt;
    alsa_get_control_info("Playback Volume", &vol_cnt);
    alsa_set_control_ints("Playback Volume", vol_cnt, vol);
}

void audiohw_close(void)
{
    hw_close();
    alsa_close_controls();
    noican_close();
}

void audiohw_set_frequency(int fsel)
{
    (void) fsel;
}
