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
#ifndef __dri_imx233__
#define __dri_imx233__

#include "config.h"
#include "cpu.h"
#include "system.h"

struct imx233_dri_info_t
{
    bool running;
    bool inputs_enabled;
    bool attention;
    bool pilot_sync_loss;
    bool overflow;
    int pilot_phase;
};

void imx233_dri_init(void);
/* enable/disable DRI block */
void imx233_dri_enable(bool en);
/* start/stop reception of data */
void imx233_dri_run(bool start);
struct imx233_dri_info_t imx233_dri_get_info(void);

#endif /* __dri_imx233__ */
