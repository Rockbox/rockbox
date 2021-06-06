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

#include "system.h"
#include "core_alloc.h"
#include "kernel/kernel-internal.h"
#include "i2c.h"
#include "power.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "storage.h"
#include "file_internal.h"
#include "disk.h"
#include "usb.h"
#include "rb-loader.h"
#include "loader_strerror.h"
#include "version.h"
#include "spl-x1000.h"
#include "x1000/cpm.h"

void main(void)
{
    /* This hack is needed because when USB booting, we cannot initialize
     * clocks in the SPL -- it may break the mask ROM's USB code. So if the
     * SPL has not already initialized the clocks, we need to do that now.
     *
     * Also use this as a sign that we should enter the recovery menu since
     * this is probably the expected result if the user is USB booting...
     */
    if(jz_readf(CPM_MPCR, ENABLE)) {
        spl_handle_pre_boot(0);
    }

    system_init();
    core_allocator_init();
    kernel_init();
    i2c_init();
    power_init();
    button_init();
    enable_irq();

    if(storage_init() < 0) {
        power_off();
    }

    filesystem_init();

    while(1);
}
