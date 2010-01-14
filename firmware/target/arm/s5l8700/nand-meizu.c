/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Bertrik Sikken
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
 
 #include "config.h"
 #include "nand-target.h"

const struct nand_device_info_type* nand_get_device_type(uint32_t bank);


uint32_t nand_read_page(uint32_t bank, uint32_t page, void* databuffer,
                        void* sparebuffer, uint32_t doecc,
                        uint32_t checkempty)
{
    /* TODO implement */
    return 0;
}

uint32_t nand_write_page(uint32_t bank, uint32_t page, void* databuffer,
                         void* sparebuffer, uint32_t doecc)
{
    /* TODO implement */
    return 0;
}

uint32_t nand_block_erase(uint32_t bank, uint32_t page)
{
    /* TODO implement */
    return 0;
}

uint32_t nand_reset(uint32_t bank)
{
    /* TODO implement */
    return 0;
}

uint32_t nand_device_init(void)
{
    /* TODO implement */
    return 0;
}

void nand_power_up(void)
{
    /* TODO implement */
}

void nand_power_down(void)
{
    /* TODO implement */
}

void nand_set_active(void)
{
    /* TODO implement */
}

long nand_last_activity(void)
{
    /* TODO implement */
    return 0;
}

