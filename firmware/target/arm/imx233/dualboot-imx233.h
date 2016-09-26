/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2011 by Amaury Pouly
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
#ifndef DUALBOOT_IMX233_H
#define DUALBOOT_IMX233_H

#include "config.h"

/* IMPORTANT NOTE This file is used by both Rockbox (firmware and bootloader)
 * and the dualboot stub. The stub #include this file directly with
 * COMPILE_DUALBOOT_STUB defined, so make sure this file is independent and does
 * not requires anything from the firmware */
#ifndef COMPILE_DUALBOOT_STUB
#include "stdint.h"
#endif

#ifdef HAVE_DUALBOOT_STUB
/* See dualboot-imx233.c for documentation */

enum imx233_dualboot_field_t
{
    DUALBOOT_CAP_BOOT, /* boot capability: 1 => BOOT field supports OF and UPDATER */
    DUALBOOT_BOOT, /* boot mode: IMX23_BOOT_x */
};

#define IMX233_BOOT_NORMAL      0 /* boot Rockbox (or OF if magic button) */
#define IMX233_BOOT_OF          1 /* boot OF */
#define IMX233_BOOT_UPDATER     2 /* boot updater */

/* get field value (or 0 if not present) */
unsigned imx233_dualboot_get_field(enum imx233_dualboot_field_t field);
/* write field value */
void imx233_dualboot_set_field(enum imx233_dualboot_field_t field, unsigned val);
#endif /* HAVE_DUALBOOT_STUB */ 

#endif /* DUALBOOT_IMX233_H */
