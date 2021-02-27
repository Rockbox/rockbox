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

#include "nand-x1000.h"
#include "nand-target.h"
#include "sfc-x1000.h"

/* Unbelievably FiiO has completely disabled the use of ECC for this chip
 * in their Linux kernel, even though it has perfectly good spare areas.
 * There's no internal ECC either.
 *
 * Using nanddump to read the spare areas reveals they're filled with 0xff,
 * and the publicly released Linux source has the ecc_strength set to 0.
 */
static const nand_chip_data ato25d1ga = {
    .name = "ATO25D1GA",
    .mf_id = 0x9b,
    .dev_id = 0x12,
    .dev_conf = NAND_INIT_SFC_DEV_CONF,
    /* XXX: datasheet says 104 MHz but FiiO seems to run this at 150 MHz.
     * Didn't find any issues doing this so might as well keep the behavior.
     */
    .clock_freq = NAND_INIT_CLOCK_SPEED,
    .block_size = 64,
    .page_size = 2048,
    .spare_size = 64,
    .rowaddr_width = 3,
    .coladdr_width = 2,
    .flags = NANDCHIP_FLAG_QUAD,
};

const nand_chip_desc target_nand_chip_descs[] = {
    {&ato25d1ga, &nand_chip_ops_std},
    {NULL, NULL},
};
