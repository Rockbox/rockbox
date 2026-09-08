/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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

#include "spl-ingenic.h"
#include "nand-ingenic.h"
#include "ingenic-soc.h"

static struct nand_drv* ndrv = NULL;

int spl_storage_open(void)
{
    /* We need to assign the GPIOs manually */
    sfc_configure_pins();

    /* Allocate NAND driver manually in DRAM */
    ndrv = spl_alloc(sizeof(struct nand_drv));
    ndrv->page_buf = spl_alloc(NAND_DRV_MAXPAGESIZE);
    ndrv->scratch_buf = spl_alloc(NAND_DRV_SCRATCHSIZE);
    ndrv->refcount = 0;

    return nand_open(ndrv);
}

void spl_storage_close(void)
{
    nand_close(ndrv);
}

int spl_storage_read(uint32_t addr, uint32_t length, void* buffer)
{
    return nand_read_bytes(ndrv, addr, length, buffer);
}
