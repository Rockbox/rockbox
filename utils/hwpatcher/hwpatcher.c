/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Amaury Pouly
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

#define _ISOC99_SOURCE /* snprintf() */
#define _POSIX_C_SOURCE 200809L /* for strdup */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <getopt.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#if LUA_VERSION_NUM < 502
#warning You need at least lua 5.2
#endif

#include "crypto.h"
#include "elf.h"
#include "sb.h"
#include "sb1.h"
#include "misc.h"
#include "md5.h"

#define ARRAYLEN(arr) (sizeof(arr) / sizeof(arr[0]))

lua_State *g_lua;
bool g_exit = false;

/**
 * FW object library
 */

enum fw_type_t
{
    FW_UNK, FW_ELF, FW_SB1, FW_SB2, FW_BIN, FW_EDOC
};

enum crc_type_t
{
    CRC_RKW
};

struct crc_type_desc_t
{
    enum crc_type_t type;
    const char *lua_name;
    unsigned (*fn)(uint8_t *buf, size_t len);
};

struct bin_file_t
{
    size_t size;
    void *data;
};

struct edoc_section_t
{
    uint32_t addr;
    size_t size;
    void *data;
};

struct edoc_file_t
{
    int nr_sections;
    struct edoc_section_t *sections;
};

struct edoc_header_t
{
    char magic[4];
    uint32_t total_size;
    uint32_t zero;
} __attribute__((packed));

struct edoc_section_header_t
{
    uint32_t addr;
    uint32_t size;
    uint32_t checksum;
} __attribute__((packed));

uint32_t edoc_checksum(void *buffer, size_t size)
{
    uint32_t c = 0;
    uint32_t *p = buffer;
    while(size >= 4)
    {
        c += *p + (*p >> 16);
        p++;
        size -= 4;
    }
    if(size != 0)
        printf("[Checksum section size is not a multiple of 4 bytes !]\n");
    return c & 0xffff;
}

#define FWOBJ_MAGIC     ('F' | 'W' << 8 | 'O' << 16 | 'B' << 24)

struct fw_object_t
{
    uint32_t magic;
    enum fw_type_t type;
    union
    {
        struct sb_file_t *sb;
        struct sb1_file_t *sb1;
        struct elf_params_t *elf;
        struct bin_file_t *bin;
        struct edoc_file_t *edoc;
    }u;
};

typedef struct fw_addr_t
{
    uint32_t addr;
    const char *section;
}fw_addr_t;

#define INVALID_FW_ADDR ((struct fw_addr_t){.addr = (uint32_t)-1, .section = ""})
#define IS_VALID_FW_ADDR(x) !(x.addr == (uint32_t)-1 && x.section && strlen(x.section) == 0)

typedef struct fw_sym_addr_t
{
    const char *name;
    const char *section;
}fw_sym_addr_t;

struct fw_section_info_t
{
    uint32_t addr;
    uint32_t size;
};

static inline struct fw_addr_t make_addr(uint32_t addr, const char *section)
{
    return (struct fw_addr_t){.addr = addr, .section = section};
}

static enum fw_type_t fw_guess(const char *filename)
{
    enum sb_version_guess_t ver = guess_sb_version(filename);
    if(ver == SB_VERSION_ERR)
    {
        printf("Cannot open/read SB file: %m\n");
        return FW_UNK;
    }
    if(ver == SB_VERSION_1) return FW_SB1;
    if(ver == SB_VERSION_2) return FW_SB2;
    FILE *fd = fopen(filename, "rb");
    if(fd == NULL)
    {
        printf("Cannot open '%s' for reading: %m\n", filename);
        return FW_UNK;
    }
    uint8_t sig[4];
    if(fread(sig, 4, 1, fd) == 1)
    {
        if(sig[0] == 'E' && sig[1] == 'D' && sig[2] == 'O' && sig[3] == 'C')
            return FW_EDOC;
    }
    bool is_elf = elf_guess(elf_std_read, fd);
    fclose(fd);
    return is_elf ? FW_ELF : FW_BIN;
}

static const char *fw_type_name(enum fw_type_t t)
{
    switch(t)
    {
        case FW_SB1: return "SB1";
        case FW_SB2: return "SB2";
        case FW_ELF: return "ELF";
        case FW_BIN: return "binary";
        case FW_EDOC: return "EDOC";
        default: return "<unk>";
    }
}

static struct fw_object_t *fw_load(const char *filename, enum fw_type_t force)
{
    if(force == FW_UNK)
        force = fw_guess(filename);
    if(force == FW_UNK)
    {
        printf("Cannot guess type of file: %m. This probably indicates the file cannot be read.\n");
        return NULL;
    }

    struct fw_object_t *obj = malloc(sizeof(struct fw_object_t));
    memset(obj, 0, sizeof(struct fw_object_t));
    obj->magic = FWOBJ_MAGIC;

    printf("Loading '%s' as %s file...\n", filename, fw_type_name(force));
    color(OFF);
    if(force == FW_SB2)
    {
        enum sb_error_t err;
        obj->type = FW_SB2;
        obj->u.sb = sb_read_file(filename, false, NULL, generic_std_printf, &err);
        if(obj->u.sb == NULL)
        {
            printf("SB read failed: %d\n", err);
            goto Lerr;
        }
        return obj;
    }
    else if(force == FW_SB1)
    {
        enum sb1_error_t err;
        obj->type = FW_SB1;
        obj->u.sb1 = sb1_read_file(filename, NULL, generic_std_printf, &err);
        if(obj->u.sb1 == NULL)
        {
            printf("SB1 read failed: %d\n", err);
            goto Lerr;
        }
        return obj;
    }
    else if(force == FW_ELF)
    {
        FILE *fd = fopen(filename, "rb");
        if(fd == NULL)
        {
            printf("cannot open '%s' for reading: %m\n", filename);
            goto Lerr;
        }
        obj->type = FW_ELF;
        obj->u.elf = malloc(sizeof(struct elf_params_t));
        elf_init(obj->u.elf);
        bool loaded = elf_read_file(obj->u.elf, elf_std_read, generic_std_printf, fd);
        fclose(fd);
        if(!loaded)
        {
            printf("error loading elf file '%s'\n", filename);
            free(obj->u.elf);
            goto Lerr;
        }
        return obj;
    }
    else if(force == FW_EDOC)
    {
        FILE *fd = fopen(filename, "rb");
        if(fd == NULL)
        {
            printf("cannot open '%s' for reading: %m\n", filename);
            goto Lerr;
        }
        struct edoc_header_t hdr;
        if(fread(&hdr, sizeof(hdr), 1, fd) != 1)
        {
            printf("cannot read EDOC header: %m\n");
            goto Lerr;
        }
        if(strncmp(hdr.magic, "EDOC", 4) != 0)
        {
            printf("EDOC signature mismatch\n");
            goto Lerr;
        }
        struct edoc_file_t *file = xmalloc(sizeof(struct edoc_file_t));
        memset(file, 0, sizeof(struct edoc_file_t));
        for(size_t pos = sizeof(hdr); pos < hdr.total_size + 8;)
        {
            struct edoc_section_header_t shdr;
            if(fread(&shdr, sizeof(shdr), 1, fd) != 1)
            {
                printf("cannot read EDOC section header: %m\n");
                goto Lerr;
            }
            file->sections = realloc(file->sections, (file->nr_sections + 1) *
                sizeof(struct edoc_section_t));
            file->sections[file->nr_sections].addr = shdr.addr;
            file->sections[file->nr_sections].size = shdr.size;
            file->sections[file->nr_sections].data = xmalloc(shdr.size);
            if(fread(file->sections[file->nr_sections].data, shdr.size, 1, fd) != 1)
            {
                printf("cannot read EDOC section: %m\n");
                goto Lerr;
            }
            if(edoc_checksum(file->sections[file->nr_sections].data, shdr.size) != shdr.checksum)
            {
                printf("EDOC section checksum mismatch\n");
                goto Lerr;
            }
            file->nr_sections++;
            pos += sizeof(shdr) + shdr.size;
        }
        fclose(fd);
        obj->type = FW_EDOC;
        obj->u.edoc = file;
        return obj;
    }
    else
    {
        FILE *fd = fopen(filename, "rb");
        if(fd == NULL)
        {
            printf("cannot open '%s' for reading: %m\n", filename);
            goto Lerr;
        }
        obj->u.bin = malloc(sizeof(struct bin_file_t));
        obj->type = FW_BIN;
        fseek(fd, 0, SEEK_END);
        obj->u.bin->size = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        obj->u.bin->data = xmalloc(obj->u.bin->size);
        if(fread(obj->u.bin->data, obj->u.bin->size, 1, fd) != 1)
        {
            printf("cannot read '%s': %m\n", filename);
            free(obj->u.bin->data);
            free(obj->u.bin);
            goto Lerr;
        }
        fclose(fd);
        return obj;
    }

Lerr:
    free(obj);
    return NULL;
}

static bool fw_save(struct fw_object_t *obj, const char *filename)
{
    if(obj->type == FW_ELF)
    {
        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
        {
            printf("Cannot open '%s' for writing: %m\n", filename);
            return false;
        }
        elf_write_file(obj->u.elf, elf_std_write, generic_std_printf, fd);
        fclose(fd);
        return true;
    }
    else if(obj->type == FW_SB2)
    {
        /* sb_read_file will fill real key and IV but we don't want to override
        * them when looping back otherwise the output will be inconsistent and
        * garbage */
        obj->u.sb->override_real_key = false;
        obj->u.sb->override_crypto_iv = false;
        enum sb_error_t err = sb_write_file(obj->u.sb, filename, NULL, generic_std_printf);
        if(err != SB_SUCCESS)
        {
            printf("Cannot write '%s': %d\n", filename, err);
            return false;
        }
        return true;
    }
    else if(obj->type == FW_BIN)
    {
        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
        {
            printf("Cannot open '%s' for writing: %m\n", filename);
            return false;
        }
        fwrite(obj->u.bin->data, 1, obj->u.bin->size, fd);
        fclose(fd);
        return true;
    }
    else if(obj->type == FW_EDOC)
    {
        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
        {
            printf("Cannot open '%s' for writing: %m\n", filename);
            return false;
        }
        struct edoc_header_t hdr;
        strncpy(hdr.magic, "EDOC", 4);
        hdr.zero = 0;
        hdr.total_size = 4;
        for(int i = 0; i < obj->u.edoc->nr_sections; i++)
            hdr.total_size += sizeof(struct edoc_section_header_t) +
                obj->u.edoc->sections[i].size;
        fwrite(&hdr, sizeof(hdr), 1, fd);
        for(int i = 0; i < obj->u.edoc->nr_sections; i++)
        {
            struct edoc_section_header_t shdr;
            shdr.addr = obj->u.edoc->sections[i].addr;
            shdr.size = obj->u.edoc->sections[i].size;
            shdr.checksum = edoc_checksum(obj->u.edoc->sections[i].data, shdr.size);
            fwrite(&shdr, sizeof(shdr), 1, fd);
            fwrite(obj->u.edoc->sections[i].data, shdr.size, 1, fd);
        }
        fclose(fd);
        return true;
    }
    else
    {
        printf("Unimplemented fw_save\n");
        return false;
    }
}

static void fw_free(struct fw_object_t *obj)
{
    switch(obj->type)
    {
        case FW_SB1:
            sb1_free(obj->u.sb1);
            break;
        case FW_SB2:
            sb_free(obj->u.sb);
            break;
        case FW_ELF:
            elf_release(obj->u.elf);
            free(obj->u.elf);
            break;
        case FW_BIN:
            free(obj->u.bin->data);
            free(obj->u.bin);
        case FW_EDOC:
            for(int i = 0; i < obj->u.edoc->nr_sections; i++)
                free(obj->u.edoc->sections[i].data);
            free(obj->u.edoc->sections);
            free(obj->u.edoc);
        default:
            break;
    }
    free(obj);
}

static struct elf_section_t *elf_find_section(struct elf_params_t *elf, fw_addr_t addr)
{
    struct elf_section_t *match = NULL;
    for(struct elf_section_t *sec = elf->first_section; sec; sec = sec->next)
    {
        if(addr.section && strcmp(addr.section, sec->name) != 0)
            continue;
        if(addr.addr < sec->addr || addr.addr >= sec->addr + sec->size)
            continue;
        if(match != NULL)
        {
            printf("Error: there are several match for address %#x@%s\n",
                (unsigned)addr.addr, addr.section);
            return NULL;
        }
        match = sec;
    }
    if(match == NULL)
    {
        printf("Error: there is no match for address %#x@%s\n", (unsigned)addr.addr,
            addr.section);
    }
    return match;
}

static bool fw_elf_rw(struct elf_params_t *elf, fw_addr_t addr, void *buffer, size_t size, bool read)
{
    struct elf_section_t *sec = elf_find_section(elf, addr);
    if(sec == NULL)
        return false;
    if(addr.addr + size > sec->addr + sec->size)
    {
        printf("Unsupported read/write across section boundary in ELF firmware\n");
        return false;
    }
    if(sec->type != EST_LOAD)
    {
        printf("Error: unimplemented read/write to a fill section (ELF)\n");
        return false;
    }
    void *data = sec->section + addr.addr - sec->addr;
    if(read)
        memcpy(buffer, data, size);
    else
        memcpy(data, buffer, size);
    return true;
}

static struct sb_inst_t *sb2_find_section(struct sb_file_t *sb_file, fw_addr_t addr)
{
    struct sb_inst_t *match = NULL;
    uint32_t sec_id = 0xffffffff;
    int inst_nr = -1;
    if(addr.section)
    {
        /* must be of the form name[.index] */
        const char *mid = strchr(addr.section, '.');
        char *end;
        if(mid)
        {
            inst_nr = strtol(mid + 1, &end, 0);
            if(*end)
            {
                printf("Warning: ignoring invalid section name '%s' (invalid inst nr)\n", addr.section);
                goto Lscan;
            }
        }
        else
            mid = addr.section + strlen(addr.section);
        if(mid - addr.section > 4)
        {
            printf("Warning: ignoring invalid section name '%s' (sec id too long)\n", addr.section);
                goto Lscan;
        }
        sec_id = 0;
        for(int i = 0; i < mid - addr.section; i++)
            sec_id = sec_id << 8 | addr.section[i];
    }

    Lscan:
    for(int i = 0; i < sb_file->nr_sections; i++)
    {
        struct sb_section_t *sec = &sb_file->sections[i];
        if(addr.section && sec->identifier != sec_id)
            continue;
        int cur_blob = 0;
        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            if(inst->inst == SB_INST_CALL || inst->inst == SB_INST_JUMP)
                cur_blob++;
            if(inst_nr >= 0 && cur_blob != inst_nr)
                continue;
            if(inst->inst != SB_INST_LOAD && inst->inst != SB_INST_FILL && inst->inst != SB_INST_DATA)
                continue;
            /* only consider data sections if section has been explicitely stated */
            if(inst->inst == SB_INST_DATA && !addr.section)
                continue;
            /* for data sections, address will be 0 */
            if(addr.addr < inst->addr || addr.addr > inst->addr + inst->size)
                continue;
            if(match != NULL)
            {
                printf("Error: there are several match for address %#x@%s\n",
                    (unsigned)addr.addr, addr.section);
                return NULL;
            }
            match = inst;
        }
    }
    if(match == NULL)
    {
        printf("Error: there is no match for address %#x@%s\n", (unsigned)addr.addr,
            addr.section);
    }
    return match;
}

static bool fw_sb2_rw(struct sb_file_t *sb_file, fw_addr_t addr, void *buffer, size_t size, bool read)
{
    struct sb_inst_t *inst = sb2_find_section(sb_file, addr);
    if(inst == NULL)
        return false;
    if(addr.addr + size > inst->addr + inst->size)
    {
        printf("Unsupported read/write across instruction boundary in SB firmware\n");
        return false;
    }
    if(inst->inst != SB_INST_LOAD && inst->inst != SB_INST_DATA)
    {
        printf("Error: unimplemented read/write to a fill instruction (SB)\n");
        return false;
    }
    void *data = inst->data + addr.addr - inst->addr;
    if(read)
        memcpy(buffer, data, size);
    else
        memcpy(data, buffer, size);
    return true;
}

static bool fw_bin_rw(struct bin_file_t *bin_file, fw_addr_t addr, void *buffer, size_t size, bool read)
{
    if(addr.addr + size > bin_file->size)
    {
        printf("Unsupport read/write accross boundary in binary firmware\n");
        return false;
    }
    void *data = bin_file->data + addr.addr;
    if(read)
        memcpy(buffer, data, size);
    else
        memcpy(data, buffer, size);
    return true;
}

static bool fw_edoc_rw(struct edoc_file_t *edoc_file, fw_addr_t addr, void *buffer, size_t size, bool read)
{
    for(int i = 0; i < edoc_file->nr_sections; i++)
    {
        if(addr.addr < edoc_file->sections[i].addr ||
                addr.addr + size >= edoc_file->sections[i].addr + edoc_file->sections[i].size)
            continue;
        void *data = edoc_file->sections[i].data + addr.addr - edoc_file->sections[i].addr;
        if(read)
            memcpy(buffer, data, size);
        else
            memcpy(data, buffer, size);
        return true;
    }
    printf("Unsupport read/write accross boundary in EDOC firmware\n");
    return false;
}

static bool fw_rw(struct fw_object_t *obj, fw_addr_t addr, void *buffer, size_t size, bool read)
{
    switch(obj->type)
    {
        case FW_ELF: return fw_elf_rw(obj->u.elf, addr, buffer, size, read);
        case FW_SB2: return fw_sb2_rw(obj->u.sb, addr, buffer, size, read);
        case FW_BIN: return fw_bin_rw(obj->u.bin, addr, buffer, size, read);
        case FW_EDOC: return fw_edoc_rw(obj->u.edoc, addr, buffer, size, read);
        default:
            printf("Error: unimplemented read/write for type %d\n", obj->type);
            return false;
    }
}

static bool fw_read(struct fw_object_t *obj, fw_addr_t addr, void *buffer, size_t size)
{
    return fw_rw(obj, addr, buffer, size, true);
}

static bool fw_write(struct fw_object_t *obj, fw_addr_t addr, const void *buffer, size_t size)
{
    return fw_rw(obj, addr, (void *)buffer, size, false);
}

static bool elf_find_sym(struct elf_params_t *elf, fw_sym_addr_t addr, fw_addr_t *out_addr)
{
    bool found = false;
    for(struct elf_symbol_t *cur = elf->first_symbol; cur; cur = cur->next)
    {
        if(strcmp(cur->name, addr.name) != 0)
            continue;
        if(addr.section && strcmp(cur->section, addr.section) != 0)
            continue;
        if(found)
        {
            printf("Error: there are several match for symbol %s@%s\n", addr.name, addr.section);
            return false;
        }
        out_addr->addr = cur->addr;
        out_addr->section = cur->section;
        found = true;
    }
    return found;
}

static bool fw_find_sym(struct fw_object_t *obj, fw_sym_addr_t addr, fw_addr_t *out_addr)
{
    switch(obj->type)
    {
        case FW_ELF: return elf_find_sym(obj->u.elf, addr, out_addr);
        case FW_SB2: case FW_SB1: case FW_BIN: return false;
        default:
            printf("Error: unimplemented find addr for type %d\n", obj->type);
            return false;
    }
}

static bool fw_bin_section_info(struct bin_file_t *obj, const char *sec, struct fw_section_info_t *out)
{
    // the only valid section names are NULL and ""
    if(sec != NULL && strlen(sec) != 0)
        return false;
    out->addr = 0;
    out->size = obj->size;
    return true;
}

static bool fw_section_info(struct fw_object_t *obj, const char *sec, struct fw_section_info_t *out)
{
    switch(obj->type)
    {
        case FW_BIN: return fw_bin_section_info(obj->u.bin, sec, out);
        default:
            printf("Error: unimplemented get section info for type %d\n", obj->type);
            return false;
    }
}

/**
 * LUA library
 */

struct fw_object_t *my_lua_get_object(lua_State *state, int index)
{
    struct fw_object_t *obj = lua_touserdata(state, index);
    if(obj == NULL || obj->magic != FWOBJ_MAGIC)
        luaL_error(state, "invalid parameter: not a firmware object");
    return obj;
}

const char *my_lua_get_string(lua_State *state, int index)
{
    return luaL_checkstring(state, index);
}

lua_Unsigned my_lua_get_unsigned(lua_State *state, int index)
{
    lua_Integer i = luaL_checkinteger(state, index);
    if(i < 0)
        luaL_error(state, "invalid parameter: not an unsigned value");
    return i;
}

fw_addr_t my_lua_get_addr(lua_State *state, int index)
{
    if(!lua_istable(state, index))
        luaL_error(state, "invalid parameter: not an address table");
    lua_getfield(state, index, "addr");
    if(lua_isnil(state, -1))
        luaL_error(state, "invalid parameter: address has not field 'addr'");
    uint32_t addr = my_lua_get_unsigned(state, -1);
    lua_pop(state, 1);

    char *sec = NULL;
    lua_getfield(state, index, "section");
    if(!lua_isnil(state, -1))
        sec = strdup(my_lua_get_string(state, -1));
    lua_pop(state, 1);
    return make_addr(addr, sec);
}

void my_lua_pushbuffer(lua_State *state, void *buffer, size_t len)
{
    uint8_t *p = buffer;
    lua_createtable(state, len, 0);
    for(int i = 0; i < len; i++)
    {
        lua_pushinteger(state, i + 1);
        lua_pushinteger(state, p[i]);
        lua_settable(state, -3);
    }
}

void *my_lua_get_buffer(lua_State *state, int index, size_t *len)
{
    if(!lua_istable(state, index))
        luaL_error(state, "invalid parameter: not a data table");
    *len = lua_rawlen(state, index);
    uint8_t *buf = xmalloc(*len);
    for(int i = 0; i < *len; i++)
    {
        lua_pushinteger(state, i + 1);
        lua_gettable(state, index);
        if(lua_isnil(state, -1))
        {
            free(buf);
            luaL_error(state, "invalid parameter: not a data table, missing some fields");
        }
        int v = luaL_checkinteger(state, -1);
        lua_pop(state, 1);
        if(v < 0 || v > 0xff)
        {
            free(buf);
            luaL_error(state, "invalid parameter: not a data table, field is not a byte");
        }
        buf[i] = v;
    }
    return buf;
}

int my_lua_load_file(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        return luaL_error(state, "load_file takes one argument: a filename");
    enum fw_type_t type = lua_tounsigned(state, lua_upvalueindex(1));
    const char *filename =  my_lua_get_string(state, 1);
    struct fw_object_t *obj = fw_load(filename, type);
    if(obj)
        lua_pushlightuserdata(state, obj);
    else
        lua_pushnil(state);
    return 1;
}

int my_lua_save_file(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 2)
        return luaL_error(state, "load_file takes two arguments: a firmware and a filename");
    struct fw_object_t *obj = my_lua_get_object(state, 1);
    const char *filename =  my_lua_get_string(state, 2);
    lua_pushboolean(state, fw_save(obj, filename));
    return 1;
}

int my_lua_read(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 3)
        return luaL_error(state, "read takes three arguments: a firmware, an address and a length");
    struct fw_object_t *obj = my_lua_get_object(state, 1);
    fw_addr_t addr = my_lua_get_addr(state, 2);
    size_t len = my_lua_get_unsigned(state, 3);
    void *buffer = xmalloc(len);
    bool ret = fw_read(obj, addr, buffer, len);
    if(ret)
        my_lua_pushbuffer(state, buffer, len);
    else
        lua_pushnil(state);
    free(buffer);
    return 1;
}

int my_lua_write(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 3)
        return luaL_error(state, "write takes three arguments: a firmware, an address and a data table");
    struct fw_object_t *obj = my_lua_get_object(state, 1);
    fw_addr_t addr = my_lua_get_addr(state, 2);
    size_t len;
    void *buf = my_lua_get_buffer(state, 3, &len);
    fw_write(obj, addr, buf, len);
    free(buf);
    return 0;
}

int my_lua_section_info(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 2)
        return luaL_error(state, "section_info takes two arguments: a firmware and a section name");
    struct fw_object_t *obj = my_lua_get_object(state, 1);
    const char *secname = my_lua_get_string(state, 2);
    struct fw_section_info_t seci;
    if(fw_section_info(obj, secname, &seci))
    {
        lua_createtable(state, 0, 0);
        lua_pushinteger(state, seci.addr);
        lua_setfield(state, -2, "addr");
        lua_pushinteger(state, seci.size);
        lua_setfield(state, -2, "size");
    }
    else
        lua_pushnil(state);
    return 1;
}

unsigned crc_rkw(uint8_t *buf, size_t len)
{
    /* polynomial 0x04c10db7 */
    static const uint32_t crc32_lookup[16] =
    {   /* lookup table for 4 bits at a time is affordable */
        0x00000000, 0x04C10DB7, 0x09821B6E, 0x0D4316D9,
        0x130436DC, 0x17C53B6B, 0x1A862DB2, 0x1E472005,
        0x26086DB8, 0x22C9600F, 0x2F8A76D6, 0x2B4B7B61,
        0x350C5B64, 0x31CD56D3, 0x3C8E400A, 0x384F4DBD
    };

    uint32_t crc32 = 0;
    unsigned char byte;
    uint32_t t;

    while (len--)
    {
        byte = *buf++; /* get one byte of data */

        /* upper nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte >> 4; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */

        /* lower nibble of our data */
        t = crc32 >> 28; /* extract the 4 most significant bits */
        t ^= byte & 0x0F; /* XOR in 4 bits of data into the extracted bits */
        crc32 <<= 4; /* shift the CRC register left 4 bits */
        crc32 ^= crc32_lookup[t]; /* do the table lookup and XOR the result */
    }

    return crc32;
}

struct crc_type_desc_t crc_types[] =
{
    {CRC_RKW, "RKW", crc_rkw}
};

int my_lua_crc_buf(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 2)
        return luaL_error(state, "crc_buf takes two arguments: a crc type and a buffer");
    unsigned type = lua_tounsigned(state, 1);
    size_t len;
    void *buf = my_lua_get_buffer(state, 2, &len);
    for(int i = 0; i < ARRAYLEN(crc_types); i++)
        if(crc_types[i].type == type)
        {
            lua_pushunsigned(state, crc_types[i].fn(buf, len));
            free(buf);
            return 1;
        }
    free(buf);
    luaL_error(state, "crc_buf: unknown crc type");
    return 0;
}

/* compute MD5 sum of a buffer */
static bool compute_md5sum_buf(void *buf, size_t sz, uint8_t file_md5sum[16])
{
    md5_context ctx;
    md5_starts(&ctx);
    md5_update(&ctx, buf, sz);
    md5_finish(&ctx, file_md5sum);
    return true;
}

/* read a file to a buffer */
static bool read_file(const char *file, void **buffer, size_t *size)
{
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        printf("Error: cannot open file for reading: %m\n");
        return false;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *buffer = xmalloc(*size);
    if(fread(*buffer, *size, 1, f) != 1)
    {
        printf("Error: cannot read file: %m\n");
        free(*buffer);
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

/* compute MD5 of a file */
static bool compute_md5sum(const char *file, uint8_t file_md5sum[16])
{
    void *buf;
    size_t sz;
    if(!read_file(file, &buf, &sz))
        return false;
    compute_md5sum_buf(buf, sz, file_md5sum);
    free(buf);
    return true;
}

int my_lua_md5sum(lua_State *state)
{
    int n = lua_gettop(state);
    if(n != 1)
        return luaL_error(state, "md5sum takes one argument: a filename");
    const char *filename = my_lua_get_string(state, 1);
    uint8_t md5sum[16];
    if(!compute_md5sum(filename, md5sum))
        return luaL_error(state, "cannot compute md5sum of the file");
    my_lua_pushbuffer(state, md5sum, sizeof(md5sum));
    return 1;
}

static bool init_lua_hwp(void)
{
    lua_pushunsigned(g_lua, FW_UNK);
    lua_pushcclosure(g_lua, my_lua_load_file, 1);
    lua_setfield(g_lua, -2, "load_file");

    lua_pushunsigned(g_lua, FW_ELF);
    lua_pushcclosure(g_lua, my_lua_load_file, 1);
    lua_setfield(g_lua, -2, "load_elf_file");

    lua_pushunsigned(g_lua, FW_SB2);
    lua_pushcclosure(g_lua, my_lua_load_file, 1);
    lua_setfield(g_lua, -2, "load_sb_file");

    lua_pushunsigned(g_lua, FW_SB1);
    lua_pushcclosure(g_lua, my_lua_load_file, 1);
    lua_setfield(g_lua, -2, "load_sb1_file");

    lua_pushunsigned(g_lua, FW_BIN);
    lua_pushcclosure(g_lua, my_lua_load_file, 1);
    lua_setfield(g_lua, -2, "load_bin_file");

    lua_pushcfunction(g_lua, my_lua_save_file);
    lua_setfield(g_lua, -2, "save_file");

    lua_pushcfunction(g_lua, my_lua_read);
    lua_setfield(g_lua, -2, "read");

    lua_pushcfunction(g_lua, my_lua_write);
    lua_setfield(g_lua, -2, "write");

    lua_pushcfunction(g_lua, my_lua_section_info);
    lua_setfield(g_lua, -2, "section_info");

    lua_pushcfunction(g_lua, my_lua_md5sum);
    lua_setfield(g_lua, -2, "md5sum");

    lua_newtable(g_lua);
    for(int i = 0; i < ARRAYLEN(crc_types); i++)
    {
        lua_pushunsigned(g_lua, crc_types[i].type);
        lua_setfield(g_lua, -2, crc_types[i].lua_name);
    }
    lua_setfield(g_lua, -2, "CRC");

    lua_pushcfunction(g_lua, my_lua_crc_buf);
    lua_setfield(g_lua, -2, "crc_buf");

    return true;
}

int my_lua_exit(lua_State *state)
{
    g_exit = true;
    return 0;
}

bool my_lua_create_arg(lua_State *state, int argc, char **argv)
{
    lua_newtable(state); // arg
    for(int i = 0; i < argc; i++)
    {
        lua_pushinteger(state, i + 1);
        lua_pushstring(state, argv[i]);
        lua_settable(state, -3);
    }
    lua_setglobal(state, "arg");
    return true;
}

static bool init_lua(void)
{
    g_lua = luaL_newstate();
    if(g_lua == NULL)
    {
        printf("Cannot create lua state\n");
        return 1;
    }
    // open all standard libraires
    luaL_openlibs(g_lua);

    lua_newtable(g_lua); // hwp
    if(!init_lua_hwp())
        return false;
    lua_setglobal(g_lua, "hwp");

    lua_pushcfunction(g_lua, my_lua_exit);
    lua_setglobal(g_lua, "exit");

    lua_pushcfunction(g_lua, my_lua_exit);
    lua_setglobal(g_lua, "quit");

    return true;
}

static void usage(void)
{
    printf("Usage: hwpatcher [options] [--] [arguments]\n");
    printf("Options:\n");
    printf("  -?/--help         Display this message\n");
    printf("  -d/--debug        Enable debug output\n");
    printf("  -n/--no-color     Disable color output\n");
    printf("  -i/--interactive  Enter interactive mode after all files have run\n");
    printf("  -f/--do-file <f>  Do lua file\n");
    printf("  -k <file>         Add key file\n");
    printf("  -z                Add zero key\n");
    printf("  --add-key <key>   Add single key (hex)\n");
    printf("  -x                Use default sb1 key\n");
    printf("All files executed are provided with the extra arguments in the 'arg' table\n");
    exit(1);
}

int main(int argc, char **argv)
{
    bool interactive = false;
    if(argc <= 1)
        usage();
    if(!init_lua())
        return 1;
    char **do_files = xmalloc(argc * sizeof(char *));
    int nr_do_files = 0;
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'n'},
            {"interactive", no_argument, 0, 'i'},
            {"do-file", required_argument, 0, 'f'},
            {"add-key", required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dif:zx", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'n':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'i':
                interactive = true;
                break;
            case 'f':
                do_files[nr_do_files++] = optarg;
                break;
            case 'z':
            {
                struct crypto_key_t g_zero_key;
                sb_get_zero_key(&g_zero_key);
                add_keys(&g_zero_key, 1);
                break;
            }
            case 'x':
            {
                struct crypto_key_t key;
                sb1_get_default_key(&key);
                add_keys(&key, 1);
                break;
            }
            case 'a':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key specified as argument\n");
                if(*s != 0)
                    bug("Trailing characters after key specified as argument\n");
                add_keys(&key, 1);
                break;
            }
            default:
                printf("Internal error: unknown option '%c'\n", c);
                return 1;
        }
    }

    if(!my_lua_create_arg(g_lua, argc - optind, argv + optind))
        return 1;

    for(int i = 0; i < nr_do_files; i++)
    {
        if(luaL_dofile(g_lua, do_files[i]))
        {
            printf("error in %s: %s\n", do_files[i], lua_tostring(g_lua, -1));
            return 1;
        }
        lua_pop(g_lua, lua_gettop(g_lua));
    }

    if(nr_do_files == 0 && optind < argc)
        printf("Warning: extra unused arguments on command lines\n");

    if(interactive)
    {
        printf("Entering interactive mode. You can use 'quit()' or 'exit()' to quit.\n");
        rl_bind_key('\t', rl_complete);
        while(!g_exit)
        {
            char *input = readline("> ");
            if(!input)
                break;
            add_history(input);
            // evaluate string
            if(luaL_dostring(g_lua, input))
                printf("error: %s\n", lua_tostring(g_lua, -1));
            // pop everything to start from a clean stack
            lua_pop(g_lua, lua_gettop(g_lua));
            free(input);
        }
    }

    return 0;
}
