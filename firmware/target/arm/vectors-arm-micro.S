/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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

#if ARCH_VERSION < 7
# define memmanage_handler  UIE
# define busfault_handler   UIE
# define usagefault_handler UIE
#endif

#if ARCH_VERSION < 8
# define securefault_handler UIE
#endif

    .syntax unified
    .text

    /*
     * Architecturally defined exception vectors for ARMv6/7/8-M.
     *
     * The target linker script is expected to place these in the
     * correct location and then append the implementation-specific
     * vector table entries, which would normally be defined at the
     * manufacturer level of the target tree.
     */
    .section .vectors.arm,"ax",%progbits

    .global __vectors_arm
__vectors_arm:
    .word   irqstackend                 /* [ 0] Stack pointer */
    .word   reset_handler               /* [ 1] Reset */
    .word   nmi_handler                 /* [ 2] Non-maskable interrupt */
    .word   hardfault_handler           /* [ 3] */
    .word   memmanage_handler           /* [ 4] (ARMv7-M and later only) */
    .word   busfault_handler            /* [ 5] (ARMv7-M and later only) */
    .word   usagefault_handler          /* [ 6] (ARMv7-M and later only) */
    .word   securefault_handler         /* [ 7] (ARMv8-M only) */
    .word   UIE                         /* [ 8] Reserved */
    .word   UIE                         /* [ 9] Reserved */
    .word   UIE                         /* [10] Reserved */
    .word   svcall_handler              /* [11] */
    .word   debugmonitor_handler        /* [12] (ARMv7-M and later only) */
    .word   UIE                         /* [13] Reserved */
    .word   pendsv_handler              /* [14] */
    .word   systick_handler             /* [15] (Optional on ARMv6-M) */
