/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Amaury Pouly
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * API
 */
enum elf_section_type_t
{
    EST_LOAD,
    EST_FILL
};

struct elf_section_t
{
    uint32_t addr; /* virtual address */
    uint32_t size; /* virtual size */
    enum elf_section_type_t type;
    char *name;
    /* <union> */
    void *section; /* data */
    uint32_t pattern; /* fill pattern */
    /* </union> */
    struct elf_section_t *next;
    /* Internal to elf_write_file */
    uint32_t offset;
    uint32_t name_offset;
};

struct elf_segment_t
{
    uint32_t vaddr; /* virtual address */
    uint32_t paddr; /* physical address */
    uint32_t vsize; /* virtual size */
    uint32_t psize; /* physical size */
    struct elf_segment_t *next;
};

struct elf_params_t
{
    bool has_start_addr;
    uint32_t start_addr;
    struct elf_section_t *first_section;
    struct elf_section_t *last_section;
    struct elf_segment_t *first_segment;
    struct elf_segment_t *last_segment;
    int unique_index;
};

typedef bool (*elf_read_fn_t)(void *user, uint32_t addr, void *buf, size_t count);
/* write function manages it's own error state */
typedef void (*elf_write_fn_t)(void *user, uint32_t addr, const void *buf, size_t count);
typedef void (*elf_printf_fn_t)(void *user, bool error, const char *fmt, ...);

void elf_init(struct elf_params_t *params);
void elf_add_load_section(struct elf_params_t *params,
    uint32_t load_addr, uint32_t size, const void *section, const char *name);
void elf_add_fill_section(struct elf_params_t *params,
    uint32_t fill_addr, uint32_t size, uint32_t pattern);
uint32_t elf_translate_virtual_address(struct elf_params_t *params, uint32_t addr);
void elf_translate_addresses(struct elf_params_t *params);
void elf_write_file(struct elf_params_t *params, elf_write_fn_t write, elf_printf_fn_t printf, void *user);
bool elf_read_file(struct elf_params_t *params, elf_read_fn_t read, elf_printf_fn_t printf,
    void *user);
bool elf_is_empty(struct elf_params_t *params);
void elf_set_start_addr(struct elf_params_t *params, uint32_t addr);
bool elf_get_start_addr(struct elf_params_t *params, uint32_t *addr);
int elf_get_nr_sections(struct elf_params_t *params);
void elf_release(struct elf_params_t *params);

#endif /* __ELF_H__ */
