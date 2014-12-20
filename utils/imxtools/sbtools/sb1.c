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
#include "sb1.h"

static int sdram_size_table[] = {2, 8, 16, 32, 64};

#define NR_SDRAM_ENTRIES    (int)(sizeof(sdram_size_table) / sizeof(sdram_size_table[0]))

int sb1_sdram_size_by_index(int index)
{
    if(index < 0 || index >= NR_SDRAM_ENTRIES)
        return -1;
    return sdram_size_table[index];
}

int sb1_sdram_index_by_size(int size)
{
    for(int i = 0; i < NR_SDRAM_ENTRIES; i++)
        if(sdram_size_table[i] == size)
            return i;
    return -1;
}

static uint16_t swap16(uint16_t t)
{
    return (t << 8) | (t >> 8);
}

static void fix_version(struct sb1_version_t *ver)
{
    ver->major = swap16(ver->major);
    ver->minor = swap16(ver->minor);
    ver->revision = swap16(ver->revision);
}

enum sb1_error_t sb1_write_file(struct sb1_file_t *sb, const char *filename)
{
    if(sb->key.method != CRYPTO_XOR_KEY)
        return SB1_NO_VALID_KEY;
    /* compute image size (without userdata) */
    uint32_t image_size = 0;
    image_size += sizeof(struct sb1_header_t);
    for(int i = 0; i < sb->nr_insts; i++)
    {
        switch(sb->insts[i].cmd)
        {
            case SB1_INST_LOAD:
                image_size += 8 + ROUND_UP(sb->insts[i].size, 4);
                break;
            case SB1_INST_FILL:
            case SB1_INST_JUMP:
            case SB1_INST_CALL:
                image_size += 12;
                break;
            case SB1_INST_MODE:
            case SB1_INST_SDRAM:
                image_size += 8;
                break;
            default:
                bugp("Internal error: unknown SB instruction: %#x\n", sb->insts[i].cmd);
        }
    }
    // now take crypto marks and sector size into account:
    // there is one crypto mark per sector, ie 4 bytes for 508 = 512 (sector)
    image_size += 4 * ((image_size + SECTOR_SIZE - 5) / (SECTOR_SIZE - 4));
    image_size = ROUND_UP(image_size, SECTOR_SIZE);

    /* allocate buffer and fill it (ignoring crypto for now) */
    void *buf = xmalloc(image_size);
    struct sb1_header_t *header = buf;
    memset(buf, 0, image_size);
    header->rom_version = sb->rom_version;
    header->image_size = image_size + sb->userdata_size;
    header->header_size = sizeof(struct sb1_header_t);
    header->userdata_offset = sb->userdata ? image_size : 0;
    memcpy(&header->product_ver, &sb->product_ver, sizeof(sb->product_ver));
    fix_version(&header->product_ver);
    memcpy(&header->component_ver, &sb->component_ver, sizeof(sb->component_ver));
    fix_version(&header->component_ver);
    header->drive_tag = sb->drive_tag;
    strncpy((void *)header->signature, "STMP", 4);

    struct sb1_cmd_header_t *cmd = (void *)(header + 1);
    for(int i = 0; i < sb->nr_insts; i++)
    {
        int bytes = 0;
        int size = 0;
        switch(sb->insts[i].cmd)
        {
            case SB1_INST_LOAD:
                bytes = sb->insts[i].size;
                cmd->addr = sb->insts[i].addr;
                memcpy(cmd + 1, sb->insts[i].data, sb->insts[i].size);
                memset((void *)(cmd + 1) + sb->insts[i].size, 0,
                    bytes - sb->insts[i].size);
                break;
            case SB1_INST_FILL:
                bytes = sb->insts[i].size;
                size = 2;
                memcpy(cmd + 1, &sb->insts[i].pattern, 4);
                cmd->addr = sb->insts[i].addr;
                break;
            case SB1_INST_JUMP:
            case SB1_INST_CALL:
                bytes = 4;
                cmd->addr = sb->insts[i].addr;
                memcpy(cmd + 1, &sb->insts[i].argument, 4);
                break;
            case SB1_INST_MODE:
                bytes = 0;
                cmd->addr = sb->insts[i].mode;
                break;
            case SB1_INST_SDRAM:
                bytes = 0;
                cmd->addr = SB1_MK_ADDR_SDRAM(sb->insts[i].sdram.chip_select,
                    sb->insts[i].sdram.size_index);
                break;
            default:
                bugp("Internal error: unknown SB instruction: %#x\n", sb->insts[i].cmd);
        }

        /* handle most common cases */
        if(size == 0)
            size = ROUND_UP(bytes, 4) / 4 + 1;

        cmd->cmd = SB1_MK_CMD(sb->insts[i].cmd, sb->insts[i].datatype,
            bytes, sb->insts[i].critical,
            size);

        cmd = (void *)cmd + 4 + size * 4;
    }

    /* move everything to prepare crypto marks (start at the end !) */
    for(int i = image_size / SECTOR_SIZE - 1; i >= 0; i--)
        memmove(buf + i * SECTOR_SIZE, buf + i * (SECTOR_SIZE - 4), SECTOR_SIZE - 4);

    union xorcrypt_key_t key[2];
    memcpy(key, sb->key.u.xor_key, sizeof(sb->key.u.xor_key));
    void *ptr = header + 1;
    int offset = header->header_size;
    for(unsigned i = 0; i < image_size / SECTOR_SIZE; i++)
    {
        int size = SECTOR_SIZE - 4 - offset;
        uint32_t mark = xor_encrypt(key, ptr, size);
        *(uint32_t *)(ptr + size) = mark;

        ptr += size + 4;
        offset = 0;
    }

    FILE *fd = fopen(filename, "wb");
    if(fd == NULL)
        return SB1_OPEN_ERROR;
    if(fwrite(buf, image_size, 1, fd) != 1)
    {
        free(buf);
        return SB1_WRITE_ERROR;
    }
    free(buf);
    if(sb->userdata)
        fwrite(sb->userdata, sb->userdata_size, 1, fd);
    fclose(fd);

    return SB1_SUCCESS;
}

struct sb1_file_t *sb1_read_file(const char *filename, void *u,
    generic_printf_t cprintf, enum sb1_error_t *err)
{
    return sb1_read_file_ex(filename, 0, -1, u, cprintf, err);
}

struct sb1_file_t *sb1_read_file_ex(const char *filename, size_t offset, size_t size, void *u,
    generic_printf_t cprintf, enum sb1_error_t *err)
{
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            free(buf); \
            return NULL; } while(0)

    FILE *f = fopen(filename, "rb");
    void *buf = NULL;
    if(f == NULL)
        fatal(SB1_OPEN_ERROR, "Cannot open file for reading\n");
    fseek(f, 0, SEEK_END);
    size_t read_size = ftell(f);
    fseek(f, offset, SEEK_SET);
    if(size != (size_t)-1)
        read_size = size;
    buf = xmalloc(read_size);
    if(fread(buf, read_size, 1, f) != 1)
    {
        fclose(f);
        fatal(SB1_READ_ERROR, "Cannot read file\n");
    }
    fclose(f);

    struct sb1_file_t *ret = sb1_read_memory(buf, read_size, u, cprintf, err);
    free(buf);
    return ret;

    #undef fatal
}

static const char *sb1_cmd_name(int cmd)
{
    switch(cmd)
    {
        case SB1_INST_LOAD: return "load";
        case SB1_INST_FILL: return "fill";
        case SB1_INST_JUMP: return "jump";
        case SB1_INST_CALL: return "call";
        case SB1_INST_MODE: return "mode";
        case SB1_INST_SDRAM: return "sdram";
        default: return "unknown";
    }
}

static const char *sb1_datatype_name(int cmd)
{
    switch(cmd)
    {
        case SB1_DATATYPE_UINT32: return "uint32";
        case SB1_DATATYPE_UINT16: return "uint16";
        case SB1_DATATYPE_UINT8: return "uint8";
        default: return "unknown";
    }
}

/* Quick and dirty way to check a key is valid.
 * We don't do any form of format checking because we are trying to bruteforce
 * the key anyway. Assume buffer is of size SECTOR_SIZE */
bool sb1_is_key_valid_fast(void *buffer, union xorcrypt_key_t _key[2])
{
    struct sb1_header_t *header = (struct sb1_header_t *)buffer;
    union xorcrypt_key_t key[2];

    uint8_t sector[SECTOR_SIZE];
    /* copy key and data because it's modified by the crypto code */
    memcpy(key, _key, sizeof(key));
    memcpy(sector, header + 1, SECTOR_SIZE - header->header_size);
    /* try to decrypt the first sector */
    uint32_t mark = xor_decrypt(key, sector, SECTOR_SIZE - 4 - header->header_size);
    /* copy key again it's modified by the crypto code */
    return mark == *(uint32_t *)&sector[SECTOR_SIZE - 4 - header->header_size];
}

bool sb1_brute_force(const char *filename, void *u, generic_printf_t cprintf,
    enum sb1_error_t *err, struct crypto_key_t *key)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    uint8_t sector[SECTOR_SIZE];
    FILE *f = fopen(filename, "rb");
    if(f == NULL)
    {
        printf("Cannot open file '%s' for reading: %m\n", filename);
        *err = SB1_OPEN_ERROR;
        return false;
    }
    if(fread(sector, sizeof(sector), 1, f) != 1)
    {
        printf("Cannot read file '%s': %m\n", filename);
        *err = SB1_READ_ERROR;
        fclose(f);
        return false;
    }
    fclose(f);

    printf(BLUE, "Brute forcing key...\n");
    time_t start_time = time(NULL);
    uint32_t laserfuse[3] = {0, 0, 0};
    unsigned last_print = 0;
    do
    {
        for(int i = 0; i < 0x10000; i++)
        {
            laserfuse[2] = (i & 0xff00) << 8 | (i & 0xff);
            xor_generate_key(laserfuse, key->u.xor_key);
            if(g_debug)
            {
                printf(GREEN, "Trying key");
                printf(GREEN, "[");
                printf(RED, "%08x", laserfuse[0]);
                printf(GREEN, ",");
                printf(RED, "%08x", laserfuse[1]);
                printf(GREEN, ",");
                printf(RED, "%08x", laserfuse[2]);
                printf(GREEN, "]:");
                for(int j = 0; j < 32; j++)
                    printf(YELLOW, " %08x", key->u.xor_key[j / 16].k[j % 16]);
            }
            if(sb1_is_key_valid_fast(sector, key->u.xor_key))
            {
                if(g_debug)
                    printf(RED, " Ok\n");
                return true;
            }
            else
            {
                if(g_debug)
                    printf(RED, " No\n");
            }
        }
        laserfuse[0]++;

        if(laserfuse[0] / 1000 != last_print)
        {
            time_t cur_time = time(NULL);
            float key_per_sec = laserfuse[0] / (float)(cur_time - start_time);
            float tot = 0x1000000LL / key_per_sec;
            time_t eta_time = start_time + tot;

            printf(YELLOW, "%llu", laserfuse[0] * 0x10000LL);
            printf(GREEN, " out of ");
            printf(BLUE, "%llu", 0x1000000LL * 0x10000LL);
            printf(GREEN, " tested (");
            printf(RED, "%f%%", laserfuse[0] / (float)0x1000000LL * 100.0);
            printf(GREEN, "), ");
            printf(YELLOW, "%d", cur_time - start_time);
            printf(GREEN, " seconds elapsed, ");
            printf(BLUE, "%d", eta_time - cur_time);
            printf(GREEN, " seconds remaining, [");
            printf(RED, "%f", key_per_sec);
            printf(GREEN, " keys/s], ETA ");
            printf(YELLOW, "%s", ctime(&eta_time));
            last_print = laserfuse[0] / 1000;
        }
    }while(laserfuse[0] != 0);

    *err = SB1_NO_VALID_KEY;
    return false;
    #undef printf
}

struct sb1_file_t *sb1_read_memory(void *_buf, size_t filesize, void *u,
    generic_printf_t cprintf, enum sb1_error_t *err)
{
    struct sb1_file_t *file = NULL;
    uint8_t *buf = _buf;

    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            sb1_free(file); \
            return NULL; } while(0)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    file = xmalloc(sizeof(struct sb1_file_t));
    memset(file, 0, sizeof(struct sb1_file_t));
    struct sb1_header_t *header = (struct sb1_header_t *)buf;

    if(memcmp(header->signature, "STMP", 4) != 0)
        fatal(SB1_FORMAT_ERROR, "Bad signature\n");
    if(header->image_size > filesize)
        fatal(SB1_FORMAT_ERROR, "File too small (should be at least %d bytes)\n",
            header->image_size);
    if(header->header_size != sizeof(struct sb1_header_t))
        fatal(SB1_FORMAT_ERROR, "Bad header size\n");

    printf(BLUE, "Basic info:\n");
    printf(GREEN, "  ROM version: ");
    printf(YELLOW, "%x\n", header->rom_version);
    printf(GREEN, "  Userdata offset: ");
    printf(YELLOW, "%x\n", header->userdata_offset);
    printf(GREEN, "  Pad: ");
    printf(YELLOW, "%x\n", header->pad2);

    struct sb1_version_t product_ver = header->product_ver;
    fix_version(&product_ver);
    struct sb1_version_t component_ver = header->component_ver;
    fix_version(&component_ver);

    printf(GREEN, "  Product version: ");
    printf(YELLOW, "%X.%X.%X\n", product_ver.major, product_ver.minor, product_ver.revision);
    printf(GREEN, "  Component version: ");
    printf(YELLOW, "%X.%X.%X\n", component_ver.major, component_ver.minor, component_ver.revision);

    printf(GREEN, "  Drive tag: ");
    printf(YELLOW, "%x\n", header->drive_tag);

    /* copy rom version, padding and drive tag */
    /* copy versions */
    memcpy(&file->product_ver, &product_ver, sizeof(product_ver));
    memcpy(&file->component_ver, &component_ver, sizeof(component_ver));
    file->rom_version = header->rom_version;
    file->pad2 = header->pad2;
    file->drive_tag = header->drive_tag;

    /* reduce size w.r.t to userdata part */
    uint32_t userdata_size = 0;
    if(header->userdata_offset != 0)
    {
        userdata_size = header->image_size - header->userdata_offset;
        header->image_size -= userdata_size;
    }

    if(header->image_size % SECTOR_SIZE)
    {
        if(!g_force)
            printf(GREY, "Image size is not a multiple of sector size\n");
        else
            fatal(SB1_FORMAT_ERROR, "Image size is not a multiple of sector size\n");
    }

    /* find key */
    union xorcrypt_key_t key[2];
    memset(key, 0, sizeof(key));
    bool valid_key = false;
    uint8_t sector[SECTOR_SIZE];

    for(int i = 0; i < g_nr_keys; i++)
    {
        if(g_key_array[i].method != CRYPTO_XOR_KEY)
            continue;
        /* copy key and data because it's modified by the crypto code */
        memcpy(key, g_key_array[i].u.xor_key, sizeof(key));
        memcpy(sector, header + 1, SECTOR_SIZE - header->header_size);
        /* try to decrypt the first sector */
        uint32_t mark = xor_decrypt(key, sector, SECTOR_SIZE - 4 - header->header_size);
        /* copy key again it's modified by the crypto code */
        memcpy(key, g_key_array[i].u.xor_key, sizeof(key));
        if(mark != *(uint32_t *)&sector[SECTOR_SIZE - 4 - header->header_size])
            continue;
        /* found ! */
        valid_key = true;
        break;
    }

    if(!valid_key)
    {
        if(!g_force)
            fatal(SB1_NO_VALID_KEY, "No valid key found\n");
        printf(GREY, "No valid key found: forced to continue but this will fail\n");
    }

    printf(BLUE, "Crypto\n");
    for(int i = 0; i < 2; i++)
    {
        printf(RED, "  Key %d\n", i);
        printf(OFF, "    ");
        for(int j = 0; j < 64; j++)
        {
            printf(YELLOW, "%02x ", key[i].key[j]);
            if((j + 1) % 16 == 0)
            {
                printf(OFF, "\n");
                if(j + 1 != 64)
                    printf(OFF, "    ");
            }
        }
    }

    memcpy(file->key.u.xor_key, key, sizeof(key));

    /* decrypt image in-place (and removing crypto markers) */
    void *ptr = header + 1;
    void *copy_ptr = header + 1;
    int offset = header->header_size;
    for(unsigned i = 0; i < header->image_size / SECTOR_SIZE; i++)
    {
        int size = SECTOR_SIZE - 4 - offset;
        uint32_t mark = xor_decrypt(key, ptr, size);
        if(mark != *(uint32_t *)(ptr + size) && !g_force)
            fatal(SB1_CHECKSUM_ERROR, "Crypto mark mismatch\n");
        memmove(copy_ptr, ptr, size);

        ptr += size + 4;
        copy_ptr += size;
        offset = 0;
    }

    /* reduce image size given the removed marks */
    header->image_size -= header->image_size / SECTOR_SIZE;

    printf(BLUE, "Commands\n");
    struct sb1_cmd_header_t *cmd = (void *)(header + 1);
    while((void *)cmd < (void *)header + header->image_size)
    {
        printf(GREEN, "  Command");
        printf(YELLOW, " %#x\n", cmd->cmd);
        printf(YELLOW, "    Size:");
        printf(RED, " %#x\n", SB1_CMD_SIZE(cmd->cmd));
        printf(YELLOW, "    Critical:");
        printf(RED, " %d\n", SB1_CMD_CRITICAL(cmd->cmd));
        printf(YELLOW, "    Data Type:");
        printf(RED, " %#x ", SB1_CMD_DATATYPE(cmd->cmd));
        printf(GREEN, "(%s)\n", sb1_datatype_name(SB1_CMD_DATATYPE(cmd->cmd)));
        printf(YELLOW, "    Bytes:");
        printf(RED, " %#x\n", SB1_CMD_BYTES(cmd->cmd));
        printf(YELLOW, "    Boot:");
        printf(RED, " %#x ", SB1_CMD_BOOT(cmd->cmd));
        printf(GREEN, "(%s)\n", sb1_cmd_name(SB1_CMD_BOOT(cmd->cmd)));

        /* copy command */
        struct sb1_inst_t inst;
        memset(&inst, 0, sizeof(inst));
        inst.cmd = SB1_CMD_BOOT(cmd->cmd);
        inst.critical = SB1_CMD_CRITICAL(cmd->cmd);
        inst.datatype = SB1_CMD_DATATYPE(cmd->cmd);
        inst.size = SB1_CMD_BYTES(cmd->cmd);

        switch(SB1_CMD_BOOT(cmd->cmd))
        {
            case SB1_INST_SDRAM:
                inst.sdram.chip_select = SB1_ADDR_SDRAM_CS(cmd->addr);
                inst.sdram.size_index = SB1_ADDR_SDRAM_SZ(cmd->addr);
                printf(YELLOW, "    Ram:");
                printf(RED, " %#x", inst.addr);
                printf(GREEN, " (Chip Select=%d, Size=%d)\n", SB1_ADDR_SDRAM_CS(cmd->addr),
                    sb1_sdram_size_by_index(SB1_ADDR_SDRAM_SZ(cmd->addr)));
                break;
            case SB1_INST_MODE:
                inst.mode = cmd->addr;
                printf(YELLOW, "    Mode:");
                printf(RED, " %#x\n", inst.mode);
                break;
            case SB1_INST_LOAD:
                inst.data = malloc(inst.size);
                memcpy(inst.data, cmd + 1, inst.size);
                inst.addr = cmd->addr;
                printf(YELLOW, "    Addr:");
                printf(RED, " %#x\n", inst.addr);
                break;
            case SB1_INST_FILL:
                inst.addr = cmd->addr;
                inst.pattern = *(uint32_t *)(cmd + 1);
                printf(YELLOW, "    Addr:");
                printf(RED, " %#x\n", cmd->addr);
                printf(YELLOW, "    Pattern:");
                printf(RED, " %#x\n", inst.pattern);
                break;
            case SB1_INST_CALL:
            case SB1_INST_JUMP:
                inst.addr = cmd->addr;
                inst.argument = *(uint32_t *)(cmd + 1);
                printf(YELLOW, "    Addr:");
                printf(RED, " %#x\n", cmd->addr);
                printf(YELLOW, "    Argument:");
                printf(RED, " %#x\n", inst.argument);
                break;
            default:
                printf(GREY, "WARNING: unknown SB command !\n");
                break;
        }

        file->insts = augment_array(file->insts, sizeof(inst), file->nr_insts, &inst, 1);
        file->nr_insts++;

        /* last instruction ? */
        if(SB1_CMD_BOOT(cmd->cmd) == SB1_INST_JUMP ||
                SB1_CMD_BOOT(cmd->cmd) == SB1_INST_MODE)
            break;

        cmd = (void *)cmd + 4 + 4 * SB1_CMD_SIZE(cmd->cmd);
    }

    /* copy userdata */
    file->userdata_size = userdata_size;
    if(userdata_size > 0)
    {
        file->userdata = malloc(userdata_size);
        memcpy(file->userdata, (void *)header + header->userdata_offset, userdata_size);
    }

    return file;
    #undef printf
    #undef fatal
    #undef print_hex
}

void sb1_free(struct sb1_file_t *file)
{
    if(!file) return;

    for(int i = 0; i < file->nr_insts; i++)
        free(file->insts[i].data);
    free(file->insts);
    free(file->userdata);
    free(file);
}

void sb1_dump(struct sb1_file_t *file, void *u, generic_printf_t cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define print_hex(c, p, len, nl) \
        do { printf(c, ""); print_hex(p, len, nl); } while(0)

    #define TREE    RED
    #define HEADER  GREEN
    #define TEXT    YELLOW
    #define TEXT2   BLUE
    #define TEXT3   RED
    #define SEP     OFF

    printf(BLUE, "SB1 File\n");
    printf(TREE, "+-");
    printf(HEADER, "Rom Ver: ");
    printf(TEXT, "%x\n", file->rom_version);
    printf(TREE, "+-");
    printf(HEADER, "Pad: ");
    printf(TEXT, "%x\n", file->pad2);
    printf(TREE, "+-");
    printf(HEADER, "Drive Tag: ");
    printf(TEXT, "%x\n", file->drive_tag);
    printf(TREE, "+-");
    printf(HEADER, "Product Version: ");
    printf(TEXT, "%X.%X.%X\n", file->product_ver.major, file->product_ver.minor,
        file->product_ver.revision);
    printf(TREE, "+-");
    printf(HEADER, "Component Version: ");
    printf(TEXT, "%X.%X.%X\n", file->component_ver.major, file->component_ver.minor,
        file->component_ver.revision);

    for(int j = 0; j < file->nr_insts; j++)
    {
        struct sb1_inst_t *inst = &file->insts[j];
        printf(TREE, "+-");
        printf(HEADER, "Command\n");
        printf(TREE, "|  +-");
        switch(inst->cmd)
        {
            case SB1_INST_CALL:
            case SB1_INST_JUMP:
                printf(HEADER, "%s", inst->cmd == SB1_INST_CALL ? "CALL" : "JUMP");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x", inst->addr);
                printf(SEP, " | ");
                printf(TEXT2, "arg=0x%08x\n", inst->argument);
                break;
            case SB1_INST_LOAD:
                printf(HEADER, "LOAD");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x", inst->addr);
                printf(SEP, " | ");
                printf(TEXT2, "len=0x%08x\n", inst->size);
                break;
            case SB1_INST_FILL:
                printf(HEADER, "FILL");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "addr=0x%08x", inst->addr);
                printf(SEP, " | ");
                printf(TEXT2, "len=0x%08x", inst->size);
                printf(SEP, " | ");
                printf(TEXT2, "pattern=0x%08x\n", inst->pattern);
                break;
            case SB1_INST_MODE:
                printf(HEADER, "MODE");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "mode=0x%08x\n", inst->addr);
                break;
            case SB1_INST_SDRAM:
                printf(HEADER, "SRAM");
                printf(SEP, " | ");
                printf(TEXT3, "crit=%d", inst->critical);
                printf(SEP, " | ");
                printf(TEXT, "chip_select=%d", inst->sdram.chip_select);
                printf(SEP, " | ");
                printf(TEXT2, "chip_size=%d\n", sb1_sdram_size_by_index(inst->sdram.size_index));
                break;
            default:
                printf(GREY, "[Unknown instruction %x]\n", inst->cmd);
                break;
        }
    }

    #undef printf
    #undef print_hex
}

static struct crypto_key_t g_default_xor_key =
{
    .method = CRYPTO_XOR_KEY,
    .u.xor_key =
    {
        {.k = {0x67ECAEF6, 0xB31FB961, 0x118A9F4C, 0xA32A97DA,
        0x6CC39617, 0x5BC00314, 0x9D430685, 0x4D7DB502,
        0xA347685E, 0x3C87E86C, 0x8987AAA0, 0x24B78EF1,
        0x893B9605, 0x9BB8C2BE, 0x6D9544E2, 0x375B525C}},
        {.k = {0x3F424704, 0x53B5A331, 0x6AD345A5, 0x20DCEC51,
        0x743C8D3B, 0x444B3792, 0x0AF429569, 0xB7EE1111,
        0x583BF768, 0x9683BF9A, 0x0B032D799, 0xFE4E78ED,
        0xF20D08C2, 0xFA0BE4A2, 0x4D89C317, 0x887B2D6F}}
    }
};

void sb1_get_default_key(struct crypto_key_t *key)
{
    memcpy(key, &g_default_xor_key, sizeof(g_default_xor_key));
    /* decrypt the xor key which is xor'ed */
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 16; j++)
            key->u.xor_key[i].k[j] ^= 0xaa55aa55;
}