/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#ifndef __NWZ_AUDIO_H__
#define __NWZ_AUDIO_H__

/* The Sony audio driver is a beast that involves several modules but eventually
 * all boils down to the codec driver that handles everything, including ALSA
 * controls. The volume, power up and other controls are doing through the ALSA
 * interface. The driver also accepts private IOCTL on the hardware device
 * (/dev/snd/hwC0D0). Finally some noice cancelling (NC) ioctl must be sent to
 * the noican driver (/dev/icx_noican).
 */

/**
 * Sound driver
 */

#define NWZ_AUDIO_TYPE  'a'

#define NWZ_AUDIO_GET_CLRSTRO_VAL   _IOR(NWZ_AUDIO_TYPE, 0x04, struct nwz_cst_param_t)
struct nwz_cst_param_t
{
    int no_ext_cbl;
    int with_ext_cbl;
};

#define NEW_AUDIO_ALC_DATA_NUM      32

enum nwz_audio_alc_types
{
    NWZ_ALC_TYPE_NONE = 0,  /* invalid        */
    NWZ_ALC_TYPE_HP_DSP,    /* HP   / DSP ALC */
    NWZ_ALC_TYPE_HP_ARM,    /* HP   / ARM ALC */
    NWZ_ALC_TYPE_LINE_DSP,  /* LINE / DSP ALC */
    NWZ_ALC_TYPE_LINE_ARM,  /* LINE / ARM ALC */
    NWZ_ALC_TYPE_BT_DSP,    /* BT   / DSP ALC */
    NWZ_ALC_TYPE_BT_ARM	    /* BT   / ARM ALC */
};

struct nwz_audio_alc_data_ex_t
{
    enum nwz_audio_alc_types type;
    unsigned short table[NEW_AUDIO_ALC_DATA_NUM];
};

#define NWZ_AUDIO_GET_ALC_DATA_EX   _IOWR(NWZ_AUDIO_TYPE, 0x0f, struct nwz_audio_alc_data_ex_t)

/**
 * Noise cancelling driver
 */

#define NWZ_NC_DEV      "/dev/icx_noican"

#define NWZ_NC_TYPE     'c'

/* Enable/Disable NC switch */
#define NWZ_NC_SET_SWITCH       _IOW(NWZ_NC_TYPE, 0x01, int)
#define NWZ_NC_GET_SWITCH       _IOR(NWZ_NC_TYPE, 0x02, int)
#define NWZ_NC_SWITCH_OFF       0
#define NWZ_NC_SWITCH_ON        1

/* Get NC HP status (whether it is NC capable or not) */
#define NWZ_NC_GET_HP_STATUS    _IOR(NWZ_NC_TYPE, 0x09, int)
#define NWZ_NC_HP_NMLHP        (0x1 << 0)
#define NWZ_NC_HP_NCHP         (0x1 << 1)

/* NC amp gain */
#define NWZ_NC_SET_GAIN         _IOW(NWZ_NC_TYPE, 0x03, int)
#define NWZ_NC_GET_GAIN         _IOR(NWZ_NC_TYPE, 0x04, int)
#define NWZ_NC_GAIN_MIN         0
#define NWZ_NC_GAIN_CENTER      15
#define NWZ_NC_GAIN_MAX         30

/* NC amp gain by value */
#define NWZ_NC_SET_GAIN_VALUE   _IOW(NWZ_NC_TYPE, 0x05, int)
#define NWZ_NC_GET_GAIN_VALUE   _IOR(NWZ_NC_TYPE, 0x06, int)
#define NWZ_NC_MAKE_GAIN(l, r)  (((r) << 8) | (l))
#define NWZ_NC_GET_L_GAIN(vol)  ((vol) & 0xff)
#define NWZ_NC_GET_R_GAIN(vol)  (((vol) >> 8) & 0xff)

/* Set/Get NC filter */
#define NWZ_NC_SET_FILTER       _IOWR(NWZ_NC_TYPE, 0x07, int)
#define NWZ_NC_GET_FILTER       _IOWR(NWZ_NC_TYPE, 0x08, int)
#define NWZ_NC_FILTER_INDEX_0   0
#define NWZ_NC_FILTER_INDEX_1   1
#define NWZ_NC_FILTER_INDEX_2   2

/* Get/Set HP type */
#define NWZ_NC_SET_HP_TYPE      _IOWR(NWZ_NC_TYPE, 0x09, int)
#define NWZ_NC_GET_HP_TYPE      _IOWR(NWZ_NC_TYPE, 0x0a, int)
#define	NC_HP_TYPE_DEFAULT      0
#define	NC_HP_TYPE_NWNC200      1

#endif /* __NWZ_AUDIO_H__ */
