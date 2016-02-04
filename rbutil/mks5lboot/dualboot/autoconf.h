/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2012 by Andrew Ryabinin
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
#ifndef __BUILD_AUTOCONF_H
#define __BUILD_AUTOCONF_H

/* lower case names match the what's exported in the Makefile
 * upper case name looks nicer in the code */

#define arch_none 0
#define ARCH_NONE 0

#define arch_sh 1
#define ARCH_SH 1

#define arch_m68k 2
#define ARCH_M68K 2

#define arch_arm 3
#define ARCH_ARM 3

#define arch_mips 4
#define ARCH_MIPS 4

#define arch_x86 5
#define ARCH_X86 5

#define arch_amd64 6
#define ARCH_AMD64 6

/* Define target machine architecture */
#define ARCH arch_arm
/* Optionally define architecture version */
#define ARCH_VERSION 5

/* Define endianess for the target or simulator platform */
#define ROCKBOX_LITTLE_ENDIAN 1

/* Define this if you build rockbox to support the logf logging and display */
#undef ROCKBOX_HAS_LOGF

/* Define this if you want logf to output to the serial port */
#undef LOGF_SERIAL

/* Define this to record a chart with timings for the stages of boot */
#undef DO_BOOTCHART

/* the threading backend we use */
#define ASSEMBLER_THREADS

/* root of Rockbox */
#define ROCKBOX_DIR "/.rockbox"
#define ROCKBOX_SHARE_PATH ""
#define ROCKBOX_BINARY_PATH ""
#define ROCKBOX_LIBRARY_PATH ""

#endif /* __BUILD_AUTOCONF_H */
