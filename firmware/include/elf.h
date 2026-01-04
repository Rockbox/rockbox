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
#ifndef __ELF_H__
#define __ELF_H__

#include <stdint.h>

/* Field offsets in the e_ident array */
#define EI_MAG0         0
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6
#define EI_OSABI        7
#define EI_ABIVERSION   8
#define EI_PAD          9
#define EI_NIDENT       16

/* Values for ei_magic */
#define ELFMAG0         0x7f
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

/* Values for ei_class */
#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

/* Values for ei_data */
#define ELFDATANONE     0
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

/* Values for ei_version and e_version */
#define EV_NONE         0
#define EV_CURRENT      1

/* Values for ei_osabi */
#define ELFOSABI_NONE   ELFOSABI_SYSV
#define ELFOSABI_SYSV   0

/* Values for e_type */
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4

/* Values for e_machine */
#define EM_NONE         0
#define EM_68K          4
#define EM_MIPS         8
#define EM_ARM          40

/*
 * Special value for e_phnum when the real number of
 * program headers is too large.
 */
#define PN_XNUM         0xFFFFu

/* ELF basic types */
typedef uint32_t elf_addr_t;
typedef uint32_t elf_off_t;

/* ELF file header */
struct elf_header
{
    union {
        uint8_t e_ident[EI_NIDENT];
        struct {
            uint8_t ei_magic[4];
            uint8_t ei_class;
            uint8_t ei_data;
            uint8_t ei_version;
            uint8_t ei_osabi;
            uint8_t ei_abiversion;
        };
    };

    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    elf_addr_t  e_entry;
    elf_off_t   e_phoff;
    elf_off_t   e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
};

/* Values for p_type */
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

/* Values for p_flags */
#define PF_X            (1 << 0)  /* Segment is executable */
#define PF_W            (1 << 1)  /* Segment is writable */
#define PF_R            (1 << 2)  /* Segment is readable */

/* ELF 32-bit program header */
struct elf32_phdr
{
    uint32_t    p_type;
    elf_off_t   p_offset;
    elf_addr_t  p_vaddr;
    elf_addr_t  p_paddr;
    uint32_t    p_filesz;
    uint32_t    p_memsz;
    uint32_t    p_flags;
    uint32_t    p_align;
};

#endif /* __ELF_H__ */
