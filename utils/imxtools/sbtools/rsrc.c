/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include "misc.h"
#include "crypto.h"
#include "rsrc.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

const char crypto_key[16] = "SanDiskSlotRadi";

static void rsrc_crypt(void *buf, int size)
{
    if(size % 16)
        printf("Size must be a multiple of 16 !\n");
    uint8_t *p = buf;
    for(int i = 0; i < size; i+= 16, p += 16)
    {
        for(int j = 0; j < 16; j++)
            p[j] ^= crypto_key[j];
    }
}

enum rsrc_error_t rsrc_write_file(struct rsrc_file_t *rsrc, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if(f == NULL)
        return RSRC_OPEN_ERROR;
    fwrite(rsrc->data, 1, rsrc->size, f);
    fclose(f);
    return RSRC_SUCCESS;
}

struct rsrc_file_t *rsrc_read_file(const char *filename, void *u,
    rsrc_color_printf cprintf, enum rsrc_error_t *err)
{
    return rsrc_read_file_ex(filename, 0, -1, u, cprintf, err);
}

struct rsrc_file_t *rsrc_read_file_ex(const char *filename, size_t offset, size_t size, void *u,
    rsrc_color_printf cprintf, enum rsrc_error_t *err)
{
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            free(buf); \
            return NULL; } while(0)

    FILE *f = fopen(filename, "rb");
    void *buf = NULL;
    if(f == NULL)
        fatal(RSRC_OPEN_ERROR, "Cannot open file for reading\n");
    fseek(f, 0, SEEK_END);
    size_t read_size = ftell(f);
    fseek(f, offset, SEEK_SET);
    if(size != (size_t)-1)
        read_size = size;
    buf = xmalloc(read_size);
    if(fread(buf, read_size, 1, f) != 1)
    {
        fclose(f);
        fatal(RSRC_READ_ERROR, "Cannot read file\n");
    }
    fclose(f);

    struct rsrc_file_t *ret = rsrc_read_memory(buf, read_size, u, cprintf, err);
    free(buf);
    return ret;

    #undef fatal
}

static const char *rsrc_table_entry_type_str(int type)
{
    switch(type)
    {
        case RSRC_TYPE_NONE: return "empty";
        case RSRC_TYPE_NESTED: return "nested";
        case RSRC_TYPE_IMAGE: return "image";
        case RSRC_TYPE_VALUE: return "value";
        case RSRC_TYPE_AUDIO: return "audio";
        case RSRC_TYPE_DATA: return "data";
        default: return "unknown";
    }
}

static bool read_entries(struct rsrc_file_t *f, void *u,
    rsrc_color_printf cprintf, enum rsrc_error_t *err,
    int offset, uint32_t base_index, int level, char *prefix)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            return e; } while(0)

    if(offset >= f->size)
        fatal(RSRC_FORMAT_ERROR, "Out of bounds at off=%x base=%x level=%d\n ouch\n");
    if(level < 0)
        fatal(RSRC_FORMAT_ERROR, "Out of levels at off=%x base=%x level=%d\n aie\n");
    for(int i = 0; i < 256; i++)
    {
        uint32_t te = *(uint32_t *)(f->data + offset + 4 * i);
        if(RSRC_TABLE_ENTRY_TYPE(te) == RSRC_TYPE_NONE)
            continue;
        uint32_t sz = 0;
        uint32_t off_off = 0;
        if(RSRC_TABLE_ENTRY_TYPE(te) == RSRC_TYPE_VALUE)
            sz = 2;
        else if(RSRC_TABLE_ENTRY_TYPE(te) == RSRC_TYPE_NESTED)
            sz = 4 * 256;
        else
        {
            sz = *(uint32_t *)(f->data + RSRC_TABLE_ENTRY_OFFSET(te));
            off_off = 4;
        }

        uint32_t index =  base_index | i << (level * 8);

        struct rsrc_entry_t ent;
        memset(&ent, 0, sizeof(ent));
        ent.id = index;
        ent.offset = RSRC_TABLE_ENTRY_OFFSET(te) + off_off;
        ent.size = sz;

        augment_array_ex((void **)&f->entries, sizeof(ent), &f->nr_entries, &f->capacity, &ent, 1);

        printf(OFF, "%s+-%s%#08x %s[%s]%s[size=%#x]", prefix, YELLOW, index, BLUE,
            rsrc_table_entry_type_str(RSRC_TABLE_ENTRY_TYPE(te)),
            GREEN, sz);

        if(RSRC_TABLE_ENTRY_TYPE(te) != RSRC_TYPE_VALUE &&
                RSRC_TABLE_ENTRY_TYPE(te) == RSRC_TYPE_NESTED)
        {
            uint8_t *p = f->data + ent.offset;
            printf(OFF, "  ");
            for(unsigned i = 0; i < MIN(sz, 16); i++)
                printf(RED, "%c", isprint(p[i]) ? p[i] : '.');
        }
        printf(OFF, "\n");

        if(RSRC_TABLE_ENTRY_TYPE(te) == RSRC_TYPE_NESTED)
        {
            char *p = prefix + strlen(prefix);
            sprintf(p, "%s|  ", RED);

            bool ok = read_entries(f, u, cprintf, err,
                RSRC_TABLE_ENTRY_OFFSET(te), index,
                level - 1, prefix);
            if(!ok)
                return false;
            *p = 0;
        }
    }

    return true;
    #undef printf
    #undef fatal
}

struct rsrc_file_t *rsrc_read_memory(void *_buf, size_t filesize, void *u,
    rsrc_color_printf cprintf, enum rsrc_error_t *err)
{
    struct rsrc_file_t *rsrc_file = NULL;
    uint8_t *buf = _buf;

    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            rsrc_free(rsrc_file); \
            return NULL; } while(0)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    rsrc_file = xmalloc(sizeof(struct rsrc_file_t));
    memset(rsrc_file, 0, sizeof(struct rsrc_file_t));

    /* There is a padding sector at the beginning of the file with a RSRC string.
     * It is unclear if this is a signature since no code I've disassembled
     * actually checks it. Allow use of -force to bypass. */
    if(memcmp(buf + 20, "RSRC", 4) != 0)
    {
        if(g_force)
            printf(GREY, "Missing RSRC signature\n");
        else
            fatal(RSRC_FORMAT_ERROR, "Missing RSRC signature\n");
    }

    rsrc_file->data = malloc(filesize);
    memcpy(rsrc_file->data, _buf, filesize);
    rsrc_file->size = filesize;

    printf(BLUE, "Entries\n");
    char prefix[1024];
    sprintf(prefix, "%s", RED);
    bool ok = read_entries(rsrc_file, u, cprintf, err, RSRC_SECTOR_SIZE, 0, 3, prefix);
    if(!ok)
        fatal(*err, "Error while parsing rsrc table\n");

    return rsrc_file;
    #undef printf
    #undef fatal
    #undef print_hex
}

void rsrc_free(struct rsrc_file_t *file)
{
    if(!file) return;
    free(file->data);
    free(file->entries);
    free(file);
}

void rsrc_dump(struct rsrc_file_t *file, void *u, rsrc_color_printf cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    #define TREE    RED
    #define HEADER  GREEN
    #define TEXT    YELLOW
    #define TEXT2   BLUE
    #define SEP     OFF

    printf(HEADER, "RSRC File\n");
    (void) file;

    #undef printf
    #undef print_hex
}

