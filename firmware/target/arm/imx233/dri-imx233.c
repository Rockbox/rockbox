/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Amaury Pouly
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
#include "dri-imx233.h"
#include "clkctrl-imx233.h"
#include "system-target.h"
#include "string.h"

#include "regs/dri.h"

void imx233_dri_init(void)
{
    imx233_clkctrl_enable(CLK_DRI, true);
    imx233_reset_block(&HW_DRI_CTRL);
    /* power down to save power */
    imx233_dri_enable(false);
}

void imx233_dri_run(bool start)
{
    if(start)
    {
        /* enable inputs, reacquire correct phrase */
        BF_SET(DRI_CTRL, ENABLE_INPUTS, REACQUIRE_PHASE);
        /* start dri */
        BF_SET(DRI_CTRL, RUN);
    }
    else
    {
        /* stop dri */
        BF_CLR(DRI_CTRL, ENABLE_INPUTS, RUN);
    }
}

void imx233_dri_enable(bool en)
{
    if(en)
        imx233_reset_block(&HW_DRI_CTRL);
    else
        BF_SET(DRI_CTRL, CLKGATE, SFTRST);
}

struct imx233_dri_info_t imx233_dri_get_info(void)
{
    struct imx233_dri_info_t info;
    memset(&info, 0, sizeof(info));

    info.running = BF_RD(DRI_CTRL, RUN);
    info.inputs_enabled = BF_RD(DRI_CTRL, ENABLE_INPUTS);
    info.attention = BF_RD(DRI_CTRL, ATTENTION_IRQ);
    info.pilot_sync_loss = BF_RD(DRI_CTRL, PILOT_SYNC_LOSS_IRQ);
    info.overflow = BF_RD(DRI_CTRL, OVERFLOW_IRQ);
    info.pilot_phase = BF_RD(DRI_STAT, PILOT_PHASE);

    return info;
}
