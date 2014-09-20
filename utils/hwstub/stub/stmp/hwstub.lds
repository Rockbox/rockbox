/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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

ENTRY(start)
OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
STARTUP(stmp/crt0.o)

#define IRAM_END_ADDR   (IRAM_ORIG + IRAM_SIZE)

MEMORY
{
    OCRAM : ORIGIN = IRAM_ORIG, LENGTH = IRAM_SIZE
}

SECTIONS
{
    .octext :
    {
        _copystart = .;
        oc_codestart = .;
        *(.vectors);
        *(.text*)
        *(.icode*)
        *(.data*)
        *(.rodata*)
        _copyend = .;
    } > OCRAM

    .bss (NOLOAD) :
    {
        bss_start = .;
        *(.bss)
        bss_end = .;
    } > OCRAM

    .stack (NOLOAD) :
    {
        oc_codeend = .;
        oc_stackstart = .;
        . += STACK_SIZE;
        oc_stackend = .;
        oc_bufferstart = .;
    } > OCRAM

    .ocend IRAM_END_ADDR (NOLOAD) :
    {
        oc_bufferend = .;
    } > OCRAM

    /DISCARD/ :
    {
        *(.eh_frame)
    } 
}
