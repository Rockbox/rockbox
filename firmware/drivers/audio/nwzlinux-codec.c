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
#include "pcm-alsa.h"

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

/* List of various codecs used by Sony */
enum nwz_codec_t
{
    NWZ_CS42L56,
    NWZ_R2A15602LG_D,
    NWZ_CS47L01_A,
    NWZ_CS47L01_D,
    NWZ_CXD3774GF_D,
    NWZ_UNK_CODEC,
};

#define NWZ_LEVEL_MUTE  -1000
/* Description of the volume curve implemented by the kernel driver */
struct nwz_vol_curve_t
{
    int count; /* number of levels */
    int level[]; /* levels in tenth-dB, level[0] is always mute */
};

/* alsa control handle, we keep it open at all times */
static snd_ctl_t *alsa_ctl;
/* list of all controls, we allocate and fill it once, so we can easily lookup */
static snd_ctl_elem_list_t *alsa_ctl_list;
/* file descriptor of the icx_noican device */
static int fd_noican;
/* file descriptor of the hardware sound device */
static int fd_hw;
/* Codec */
static enum nwz_codec_t nwz_codec;

static enum nwz_codec_t find_codec(void)
{
    if(nwz_is_kernel_module_loaded("cs42L56_d"))
        return NWZ_CS42L56;
    if(nwz_is_kernel_module_loaded("r2A15602LG_d"))
        return NWZ_R2A15602LG_D;
    if(nwz_is_kernel_module_loaded("cs47L01_d"))
        return NWZ_CS47L01_D;
    if(nwz_is_kernel_module_loaded("cs47L01_a"))
        return NWZ_CS47L01_A;
    if(nwz_is_kernel_module_loaded("cxd3774gf_d"))
        return NWZ_CXD3774GF_D;
    return NWZ_UNK_CODEC;
}

const char *nwz_get_codec_name(void)
{
    switch(nwz_codec)
    {
        case NWZ_CS42L56: return "cs42L56_d";
        case NWZ_R2A15602LG_D: return "r2A15602LG_d";
        case NWZ_CS47L01_D: return "cs47L01_d";
        case NWZ_CS47L01_A: return "cs47L01_a";
        case NWZ_CXD3774GF_D: return "cxd3774gf_d";
        default: return "Unknown";
    }
}

static struct nwz_vol_curve_t cxd3774gf_vol_curve =
{
    .count = 31,
    /* Most Sonys seem to follow the convention of 3dB/step then 2dB/step then 1dB/step */
    .level = {NWZ_LEVEL_MUTE,
        -550, -520, -490, -460, -430, -400, -370, -340, -310, -280, -250, /* 3dB/step */
        -230, -210, -190, -170, -150, -130, -110, -90, /* 2dB/step */
        -80, -70, -60, -50, -40, -30, -20, -10, 0, /* 1dB/step */
        15, 35, /* 1.5dB then 2dB */
    }
};

struct nwz_vol_curve_t *nwz_get_codec_vol_curve(void)
{
    switch(nwz_codec)
    {
        case NWZ_CS47L01_A:
        case NWZ_CS47L01_D:
            /* there are 32 levels but the last two are the same so in fact it
             * is the same curve as the cxd3774gf_d */
        case NWZ_CXD3774GF_D:
            return &cxd3774gf_vol_curve;
        default:
            /* return the safest curve (only 31 levels) */
            return &cxd3774gf_vol_curve;
    }
}

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
    /* some targets don't have noise cancelling so silently fail */
}

static void noican_close(void)
{
    if(fd_noican >= 0)
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
    if(fd_noican >= 0)
    {
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
    }

    /* init hw */
    hw_open();
    nwz_codec = find_codec();
    printf("Codec: %s\n", nwz_get_codec_name());
}

void audiohw_postinit(void)
{
}

/* volume must be driver unit */
static void nwz_set_driver_vol(int vol)
{
    long vols[2];
    /* the driver expects percent, convert from centibel in range 0...x */
    vols[0] = vols[1] = vol;
    /* on some recent players like A10, Sony decided to merge left/right volume
     * into one, thus we need to make sure we write the correct number of values */
    int vol_cnt;
    alsa_get_control_info("Playback Volume", &vol_cnt);
    alsa_set_control_ints("Playback Volume", vol_cnt, vols);
}

/* volume is in tenth-dB */
void audiohw_set_volume(int vol_l, int vol_r)
{
#if 1
    /* the Sony drivers expect vol_l = vol_r */
    int vol = (vol_l + vol_r) / 2;
    printf("request volume %d dB\n", vol / 10);
    struct nwz_vol_curve_t *curve = nwz_get_codec_vol_curve();
    /* min/max for pcm volume */
    int min_pcm = -430;
    int max_pcm = 0;
    /* On some codecs (like cs47L01), Sony clear overdrives the DAC which produces
     * massive clipping at any level (since they fix the DAC volume at around +6dB
     * and then adjust HP volume in negative at the top of range !!). The only
     * solution around this problem is to use the digital volume first so that
     * very quickly the digital volume compensate for the DAC overdrive and we
     * avoid clipping. */
    int sony_clip_level = -70; /* any volume above this will cause massive clipping the DAC */

    /* to avoid the clipping problem, virtually decrease requested volume by the
     * clipping threshold, so that we will compensate in digital later by
     * at least this amount if possibly */
    vol -= sony_clip_level;

    int drv_vol = curve->count - 1;
    /* pick driver level just above request volume */
    while(drv_vol > 0 && curve->level[drv_vol - 1] > vol)
        drv_vol--;
    /* now remove the artifical volume change */
    vol += sony_clip_level;
    /* now adjust digital volume */
    vol -= curve->level[drv_vol];
    if(vol < min_pcm)
    {
        vol = min_pcm; /* digital cannot do <43dB */
        drv_vol = 0; /* mute */
    }
    else if(vol > max_pcm)
        vol = max_pcm; /* digital cannot do >0dB */
    printf(" set driver volume %d (%d dB)\n", drv_vol, curve->level[drv_vol] / 10);
    nwz_set_driver_vol(drv_vol);
    printf(" set digital volume %d dB\n", vol / 10);
    pcm_alsa_set_digital_volume(vol / 10);
#else
    nwz_set_driver_vol(vol_l / 10 + 30);
#endif
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
