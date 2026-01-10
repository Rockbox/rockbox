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
#include "elf_loader.h"
#include "elf-target.h"
#include "file.h"

#if defined(ROCKBOX_LITTLE_ENDIAN)
# define NATIVE_ELF_DATA ELFDATA2LSB
#elif defined(ROCKBOX_BIG_ENDIAN)
# define NATIVE_ELF_DATA ELFDATA2MSB
#else
# error "Unknown endianness!"
#endif

_Static_assert(TARGET_ELF_DATA == NATIVE_ELF_DATA,
               "ELF binaries with foreign endianness not supported");

static int elf_validate_header(const struct elf_header *header)
{
    if (header->ei_magic[0] != ELFMAG0 ||
        header->ei_magic[1] != ELFMAG1 ||
        header->ei_magic[2] != ELFMAG2 ||
        header->ei_magic[3] != ELFMAG3)
        return ELF_ERR_BADMAGIC;

    if (header->ei_class != TARGET_ELF_CLASS ||
        header->ei_data != TARGET_ELF_DATA)
        return ELF_ERR_UNSUPP_ARCH;

    if (header->ei_version != EV_CURRENT ||
        header->e_version != EV_CURRENT)
        return ELF_ERR_UNSUPP_VERSION;

    if (header->ei_osabi != TARGET_ELF_OSABI ||
        header->ei_abiversion != TARGET_ELF_ABIVERSION)
        return ELF_ERR_UNSUPP_OS;

    if (header->e_type != ET_EXEC)
        return ELF_ERR_UNSUPP_TYPE;

    if (header->e_machine != TARGET_ELF_MACHINE)
        return ELF_ERR_UNSUPP_ARCH;

    if (header->e_phoff == 0)
        return ELF_ERR_EMPTY;

    if (header->e_phentsize < sizeof(struct elf_phdr))
        return ELF_ERR_UNSUPP_PHSIZE;

    if (header->e_phnum == PN_XNUM)
        return ELF_ERR_TOO_MANY_PHDRS;

    return ELF_OK;
}

static int elf_validate_phdr(const struct elf_phdr *phdr,
                             const struct elf_load_context *ctx)
{
    if (phdr->p_type != PT_LOAD)
        return ELF_ERR_UNSUPP_PHDR;

    if (phdr->p_filesz > phdr->p_memsz)
        return ELF_ERR_INVALID_PHDR;

    /* Ignore headers which occupy zero memory */
    if (phdr->p_memsz == 0)
        return 0;

    for (size_t i = 0; i < ctx->num_mmap; ++i)
    {
        const struct elf_memory_map *entry = &ctx->mmap[i];
        if (phdr->p_vaddr >= entry->addr &&
            phdr->p_vaddr  < entry->addr + entry->size &&
            phdr->p_memsz <= entry->size)
        {
            if ((phdr->p_flags & entry->flags) != phdr->p_flags)
                return ELF_ERR_MEM_INCOMPAT_FLAGS;

            return ELF_OK;
        }
    }

    return ELF_ERR_MEM_UNMAPPED;
}

int elf_load(elf_read_callback_t read_cb,
             intptr_t read_arg,
             const struct elf_load_context *ctx,
             void **entrypoint)
{
    struct elf_header ehdr;
    struct elf_phdr phdr;
    int err;

    if (read_cb(read_arg, 0, &ehdr, sizeof(ehdr)))
        return ELF_ERR_IO;

    err = elf_validate_header(&ehdr);
    if (err != ELF_OK)
        return err;

    /*
     * Iterate over program headers, copying segments to memory
     */
    for (size_t ph_index = 0; ph_index < ehdr.e_phnum; ++ph_index)
    {
        off_t ph_off = ehdr.e_phoff + (ph_index * ehdr.e_phentsize);
        if (read_cb(read_arg, ph_off, &phdr, sizeof(phdr)))
            return ELF_ERR_IO;

        err = elf_validate_phdr(&phdr, ctx);
        if (err)
            return err;

        /* Load file data to memory if needed */
        if (phdr.p_filesz > 0)
        {
            if (read_cb(read_arg, phdr.p_offset, (void *)phdr.p_vaddr, phdr.p_filesz))
                return ELF_ERR_IO;
        }

        /* Zero out any memory not backed by file data */
        if (phdr.p_memsz > phdr.p_filesz)
        {
            void *addr = (void *)phdr.p_vaddr + phdr.p_filesz;
            size_t size = phdr.p_memsz - phdr.p_filesz;

            memset(addr, 0, size);
        }
    }

    *entrypoint = (void *)ehdr.e_entry;
    return ELF_OK;
}

int elf_loadfd(int fd,
               const struct elf_load_context *ctx,
               void **entrypoint)
{
    return elf_load(elf_read_fd_callback, fd, ctx, entrypoint);
}

int elf_read_fd_callback(intptr_t fd, off_t pos, void *buf, size_t size)
{
    if (lseek(fd, pos, SEEK_SET) == (off_t)-1)
        return -1;

    ssize_t nread = read(fd, buf, size);
    if (nread < 0 || (size_t)nread != size)
        return -1;

    return 0;
}

int elf_loadpath(const char *filename,
                 const struct elf_load_context *ctx,
                 void **entrypoint)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        return ELF_ERR_FILE_NOT_FOUND;

    int err = elf_loadfd(fd, ctx, entrypoint);

    close(fd);
    return err;
}
