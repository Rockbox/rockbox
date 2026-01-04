/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#ifndef __ELF_TARGET_H__
#define __ELF_TARGET_H__

#include "elf.h"
#include "config.h"

#define TARGET_ELF_CLASS        ELFCLASS32
#define TARGET_ELF_OSABI        ELFOSABI_SYSV
#define TARGET_ELF_ABIVERSION   0
#define elf_phdr                elf32_phdr

#if defined(CPU_ARM)
# define TARGET_ELF_MACHINE     EM_ARM
#elif defined(CPU_MIPS)
# define TARGET_ELF_MACHINE     EM_MIPS
#elif defined(CPU_COLDFIRE)
# define TARGET_ELF_MACHINE     EM_68K
#else
# error "Unknown CPU architecture!"
#endif

#if defined(ROCKBOX_LITTLE_ENDIAN)
# define TARGET_ELF_DATA        ELFDATA2LSB
#elif defined(ROCKBOX_BIG_ENDIAN)
# define TARGET_ELF_DATA        ELFDATA2MSB
#else
# error "Unknown endianness!"
#endif

#endif /* __ELF_TARGET_H__ */
