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
#ifndef __ELF_LOADER_H__
#define __ELF_LOADER_H__

/*
 * Primitive ELF loader -- allows loading an ELF binary from
 * a file descriptor into RAM. Only static binaries linked at
 * a fixed address are supported.
 *
 * The caller supplies a memory map which limits where the
 * ELF file can be loaded; if a program header requests any
 * memory outside of this mapping, loading will fail. There
 * is no support for relocation, PIC code, or any form of
 * dynamic linking.
 *
 * The loader can return the entry point of the ELF binary.
 * It does not support arbitrary symbol lookups.
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

enum elf_error
{
    ELF_OK,
    ELF_ERR_FILE_NOT_FOUND,
    ELF_ERR_IO,
    ELF_ERR_EMPTY,
    ELF_ERR_BADMAGIC,
    ELF_ERR_UNSUPP_ARCH,
    ELF_ERR_UNSUPP_OS,
    ELF_ERR_UNSUPP_VERSION,
    ELF_ERR_UNSUPP_TYPE,
    ELF_ERR_UNSUPP_FORMAT,
    ELF_ERR_UNSUPP_PHSIZE,
    ELF_ERR_TOO_MANY_PHDRS,
    ELF_ERR_UNSUPP_PHDR,
    ELF_ERR_INVALID_PHDR,
    ELF_ERR_MEM_INCOMPAT_FLAGS,
    ELF_ERR_MEM_UNMAPPED,
};

/*
 * Describes an entry in the virtual memory map specified
 * when loading an ELF file.
 *
 * ELF segments must fit completely within a single entry
 * in the memory map. Segments crossing multiple mappings
 * are rejected even if the mappings are contiguous.
 *
 * A segment may also be rejected if it has flags (R/W/X)
 * not present in the corresponding memory map entry.
 */
struct elf_memory_map
{
    uintptr_t addr;
    size_t    size;
    uint32_t  flags;
};

struct elf_load_context
{
    const struct elf_memory_map *mmap;
    size_t num_mmap;
};

typedef int (*elf_read_callback_t) (intptr_t arg, off_t pos, void *buf, size_t size);

int elf_load(elf_read_callback_t read_cb,
             intptr_t read_arg,
             const struct elf_load_context *ctx,
             void **entrypoint);

int elf_loadfd(int fd,
               const struct elf_load_context *ctx,
               void **entrypoint);
int elf_read_fd_callback(intptr_t fd, off_t pos, void *buf, size_t size);

int elf_loadpath(const char *filename,
                 const struct elf_load_context *ctx,
                 void **entrypoint);

#endif /* __ELF_LOADER_H__ */
