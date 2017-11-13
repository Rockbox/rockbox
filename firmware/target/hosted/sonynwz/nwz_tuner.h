/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#ifndef __NWZ_TUNER_H__
#define __NWZ_TUNER_H__

#define NWZ_TUNER_DEV       "/dev/radio0"
/* we only describe the private ioctl, the rest is standard v4l2 */
#define NWZ_TUNER_TYPE      'v'

/* Of course Sony had to change something in the structure without changing the ioctl codes.
 * Fortunately they are close enough so that some clever naming and ifdef solves the problem. */

#if defined(SONY_NWZE350) || defined(SONY_NWZE450) || defined(SONY_NWZE460) || \
    defined(SONY_NWZA860) || defined(SONY_NWZS750)
#define NWZ_TUNER_V1
#endif

#ifdef NWZ_TUNER_V1

#define NWZ_TUNER_REG_COUNT 14

#else /* !NWZ_TUNER_V1 */

#define NWZ_TUNER_REG_COUNT 20

#endif /* NWZ_TUNER_V1 */

struct nwz_tuner_ioctl_t
{
    union
    {
        uint32_t regno; // for GET_REG
        uint32_t rssi; // for GET_RSSI
        uint32_t freq; // for SET_FREQ
        struct
        {
            uint8_t regno;
            uint8_t reserved;
            uint16_t val;
        }put; // for PUT_REG
    };
    uint16_t regs[NWZ_TUNER_REG_COUNT];
} __attribute__((packed));

/* these requests seem to be the only one supported by all generations */
#define NWZ_TUNER_INIT_REG      _IOWR(NWZ_TUNER_TYPE, 41, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_PUT_REG       _IOWR(NWZ_TUNER_TYPE, 42, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_GET_REG       _IOWR(NWZ_TUNER_TYPE, 43, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_GET_REG_ALL   _IOWR(NWZ_TUNER_TYPE, 44, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_SET_FREQ      _IOWR(NWZ_TUNER_TYPE, 45, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_RESET         _IOW(NWZ_TUNER_TYPE, 46, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_GPIO2_ON      _IOW(NWZ_TUNER_TYPE, 47, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_GPIO2_OFF     _IOW(NWZ_TUNER_TYPE, 48, struct nwz_tuner_ioctl_t)
#define NWZ_TUNER_GET_RSSI      _IOWR(NWZ_TUNER_TYPE, 49, struct nwz_tuner_ioctl_t)

#endif /* __NWZ_TUNER_H__ */
