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

#ifndef __SPL_X1000_H__
#define __SPL_X1000_H__

/* TODO: this needs some refactoring... */

/* Called on a fatal error */
extern void spl_error(void) __attribute__((noreturn));

/* Called by SPL to handle a main boot */
extern void spl_target_boot(void);

/* Invoked by SPL main routine to determine the boot option */
extern int spl_get_boot_option(void);

/* Do any setup/initialization needed for the given boot option, this
 * will be called right before flushing caches + jumping to the image.
 * Typical use is to set up system clocks, etc.
 */
extern void spl_handle_pre_boot(int bootopt);

#endif /* __SPL_X1000_H__ */
