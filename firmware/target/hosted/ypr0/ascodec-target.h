/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ascodec-target.h 26116 2010-05-17 20:53:25Z funman $
 *
 * Module wrapper for AS3543 audio codec, using /dev/afe (afe.ko) of Samsung YP-R0
 *
 * Copyright (c) 2011 Lorenzo Miori
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

#ifndef _ASCODEC_TARGET_H
#define _ASCODEC_TARGET_H

#include "as3514.h"
#include "kernel.h"
#include "adc.h"
#include "ascodec.h"

int ascodec_init(void);
void ascodec_close(void);
int ascodec_write(unsigned int reg, unsigned int value);
int ascodec_read(unsigned int reg);
void ascodec_write_pmu(unsigned int index, unsigned int subreg, unsigned int value);
int ascodec_read_pmu(unsigned int index, unsigned int subreg);
int ascodec_readbytes(unsigned int index, unsigned int len, unsigned char *data);
unsigned short adc_read(int channel);
void ascodec_lock(void);
void ascodec_unlock(void);

static inline bool ascodec_chg_status(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_STATUS;
}

static inline bool ascodec_endofch(void)
{
    return ascodec_read(AS3514_IRQ_ENRD0) & CHG_ENDOFCH;
}

static inline void ascodec_monitor_endofch(void)
{
    ascodec_write(AS3514_IRQ_ENRD0, IRQ_ENDOFCH);
}

static inline void ascodec_wait_adc_finished(void)
{
    /*
     * FIXME: not implemented
     *
     * If irqs are not available on the target platform,
     * this should be most likely implemented by polling
     * AS3514_IRQ_ENRD2 in the same way powermgmt-ascodec.c
     * is polling IRQ_ENDOFCH.
     */
}

static inline void ascodec_write_charger(int value)
{
    ascodec_write_pmu(AS3543_CHARGER, 1, value);
}

static inline int ascodec_read_charger(void)
{
    return ascodec_read_pmu(AS3543_CHARGER, 1);
}

#endif /* !_ASCODEC_TARGET_H */
