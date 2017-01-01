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
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "misc.h"
#include "crypto.h"
#include "sb.h"

#define ALIGN_DOWN(n, a)    (((n)/(a))*(a))
#define ALIGN_UP(n, a)      ALIGN_DOWN((n)+((a)-1),a)

static void fill_gaps(struct sb_file_t *sb)
{
    for(int i = 0; i < sb->nr_sections; i++)
    {
        struct sb_section_t *sec = &sb->sections[i];
        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            if(inst->inst != SB_INST_LOAD)
                continue;
            inst->padding_size = ROUND_UP(inst->size, BLOCK_SIZE) - inst->size;
            /* emulate elftosb2 behaviour: generate 15 bytes (that's a safe maximum) */
            inst->padding = xmalloc(15);
            generate_random_data(inst->padding, 15);
        }
    }
}

static void compute_sb_offsets(struct sb_file_t *sb, void *u, generic_printf_t cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    sb->image_size = 0;
    /* sb header */
    sb->image_size += sizeof(struct sb_header_t) / BLOCK_SIZE;
    /* sections headers */
    sb->image_size += sb->nr_sections * sizeof(struct sb_section_header_t) / BLOCK_SIZE;
    /* key dictionary */
    sb->image_size += g_nr_keys * sizeof(struct sb_key_dictionary_entry_t) / BLOCK_SIZE;
    /* sections */
    for(int i = 0; i < sb->nr_sections; i++)
    {
        struct sb_section_t *sec = &sb->sections[i];
        /* we need to make sure section starts on the right alignment,
         * and since each section starts with a boot tag, we need to ensure
         * that the boot tag is at address X such that X+BLOCK_SIZE is a
         * multiple of the alignment */
        uint32_t alignment = sb->sections[i].alignment / BLOCK_SIZE;
        sb->image_size = ALIGN_UP(sb->image_size + 1, alignment) - 1;
        /* update padding of previous section */
        if(i > 0)
            sb->sections[i - 1].pad_size = sb->image_size -
                sb->sections[i - 1].file_offset - sb->sections[i - 1].sec_size;
        /* each section has a preliminary TAG command */
        sb->image_size += 1;
        sec->file_offset = sb->image_size;
        /* compute section size */
        sec->sec_size = 0;
        sec->pad_size = 0;

        char name[5];
        sb_fill_section_name(name, sec->identifier);
        printf(BLUE, "%s", sec->is_data ? "Data" : "Boot");
        printf(GREEN, " Section ");
        printf(YELLOW, "'%s'", name);
        if(sec->is_cleartext)
            printf(RED, " (cleartext)");
        printf(OFF, "\n");

        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            if(inst->inst == SB_INST_CALL || inst->inst == SB_INST_JUMP)
            {
                printf(RED, "  %s", inst->inst == SB_INST_CALL ? "CALL" : "JUMP");
                printf(OFF, " | "); printf(BLUE, "addr=0x%08x", inst->addr);
                printf(OFF, " | "); printf(GREEN, "arg=0x%08x\n", inst->argument);
                sec->sec_size += sizeof(struct sb_instruction_call_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_FILL)
            {
                printf(RED, "  FILL");
                printf(OFF, " | "); printf(BLUE, "addr=0x%08x", inst->addr);
                printf(OFF, " | "); printf(GREEN, "len=0x%08x", inst->size);
                printf(OFF, " | "); printf(YELLOW, "pattern=0x%08x\n", inst->pattern);
                sec->sec_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_LOAD)
            {
                printf(RED, "  LOAD");
                printf(OFF, " | "); printf(BLUE, "addr=0x%08x", inst->addr);
                printf(OFF, " | "); printf(GREEN, "len=0x%08x\n", inst->size);
                /* load header */
                sec->sec_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                /* data + alignment */
                sec->sec_size += (inst->size + inst->padding_size) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_MODE)
            {
                printf(RED, "  MODE");
                printf(OFF, " | "); printf(BLUE, "mod=0x%08x\n", inst->addr);
                sec->sec_size += sizeof(struct sb_instruction_mode_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_DATA)
            {
                printf(RED, "  DATA");
                printf(OFF, " | "); printf(BLUE, "size=0x%08x\n", inst->size);
                sec->sec_size += ROUND_UP(inst->size, BLOCK_SIZE) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_NOP)
            {
                printf(RED, "  NOOP\n");
                sec->sec_size += sizeof(struct sb_instruction_nop_t) / BLOCK_SIZE;
            }
            else
            {
                cprintf(u, true, GREY, "die on inst %d\n", inst->inst);
            }
        }
        sb->image_size += sec->sec_size;
    }
    /* final signature */
    sb->image_size += 2;
    #undef printf
}

uint64_t sb_generate_timestamp(void)
{
    struct tm tm_base;
    memset(&tm_base, 0, sizeof(tm_base));
    /* 2000/1/1 0:00:00 */
    tm_base.tm_mday = 1;
    tm_base.tm_year = 100;
    time_t t = time(NULL) - mktime(&tm_base);
    return (uint64_t)t * 1000000L;
}

static uint16_t swap16(uint16_t t)
{
    return (t << 8) | (t >> 8);
}

static void fix_version(struct sb_version_t *ver)
{
    ver->major = swap16(ver->major);
    ver->minor = swap16(ver->minor);
    ver->revision = swap16(ver->revision);
}

static void produce_sb_header(struct sb_file_t *sb, struct sb_header_t *sb_hdr)
{
    struct sha_1_params_t sha_1_params;

    sb_hdr->signature[0] = 'S';
    sb_hdr->signature[1] = 'T';
    sb_hdr->signature[2] = 'M';
    sb_hdr->signature[3] = 'P';
    sb_hdr->major_ver = IMAGE_MAJOR_VERSION;
    sb_hdr->minor_ver = sb->minor_version;
    sb_hdr->flags = sb->flags;
    sb_hdr->image_size = sb->image_size;
    sb_hdr->header_size = sizeof(struct sb_header_t) / BLOCK_SIZE;
    sb_hdr->first_boot_sec_id = sb->sections[sb->first_boot_sec].identifier;
    sb_hdr->nr_keys = g_nr_keys;
    sb_hdr->nr_sections = sb->nr_sections;
    sb_hdr->sec_hdr_size = sizeof(struct sb_section_header_t) / BLOCK_SIZE;
    sb_hdr->key_dict_off = sb_hdr->header_size +
        sb_hdr->sec_hdr_size * sb_hdr->nr_sections;
    sb_hdr->first_boot_tag_off = sb->sections[sb->first_boot_sec].file_offset - 1;
    generate_random_data(sb_hdr->rand_pad0, sizeof(sb_hdr->rand_pad0));
    generate_random_data(sb_hdr->rand_pad1, sizeof(sb_hdr->rand_pad1));
    /* Version 1.0 has 6 bytes of random padding,
     * Version 1.1 requires the last 4 bytes to be 'sgtl' */
    if(sb->minor_version >= 1)
        memcpy(&sb_hdr->rand_pad0[2], "sgtl", 4);

    sb_hdr->timestamp = sb->timestamp;
    sb_hdr->product_ver = sb->product_ver;
    fix_version(&sb_hdr->product_ver);
    sb_hdr->component_ver = sb->component_ver;
    fix_version(&sb_hdr->component_ver);
    sb_hdr->drive_tag = sb->drive_tag;

    sha_1_init(&sha_1_params);
    sha_1_update(&sha_1_params, &sb_hdr->signature[0],
        sizeof(struct sb_header_t) - sizeof(sb_hdr->sha1_header));
    sha_1_finish(&sha_1_params);
    sha_1_output(&sha_1_params, sb_hdr->sha1_header);
}

static void produce_sb_section_header(struct sb_section_t *sec,
    struct sb_section_header_t *sec_hdr)
{
    sec_hdr->identifier = sec->identifier;
    sec_hdr->offset = sec->file_offset;
    sec_hdr->size = sec->sec_size;
    sec_hdr->flags = (sec->is_data ? 0 : SECTION_BOOTABLE)
        | (sec->is_cleartext ? SECTION_CLEARTEXT : 0)
        | sec->other_flags;
}

static uint8_t instruction_checksum(struct sb_instruction_header_t *hdr)
{
    uint8_t sum = 90;
    byte *ptr = (byte *)hdr;
    for(int i = 1; i < 16; i++)
        sum += ptr[i];
    return sum;
}

static void produce_section_tag_cmd(struct sb_section_t *sec,
    struct sb_instruction_tag_t *tag, bool is_last)
{
    tag->hdr.opcode = SB_INST_TAG;
    tag->hdr.flags = is_last ? SB_INST_LAST_TAG : 0;
    tag->identifier = sec->identifier;
    /* there is a catch here: in the section header at the beginning of the SB
     * file, we put the *useful* length of the section (without padding) but
     * the bootloader will not use those and only use the TAG commande which
     * need to give the *actual* length (with padding) */
    tag->len = sec->sec_size + sec->pad_size;
    tag->flags = (sec->is_data ? 0 : SECTION_BOOTABLE)
        | (sec->is_cleartext ? SECTION_CLEARTEXT : 0)
        | sec->other_flags;
    tag->hdr.checksum = instruction_checksum(&tag->hdr);
}

void produce_sb_instruction(struct sb_inst_t *inst,
    struct sb_instruction_common_t *cmd, void *u, generic_printf_t cprintf)
{
    memset(cmd, 0, sizeof(struct sb_instruction_common_t));
    cmd->hdr.opcode = inst->inst;
    switch(inst->inst)
    {
        case SB_INST_CALL:
        case SB_INST_JUMP:
            cmd->addr = inst->addr;
            cmd->data = inst->argument;
            break;
        case SB_INST_FILL:
            cmd->addr = inst->addr;
            cmd->len = inst->size;
            cmd->data = inst->pattern;
            break;
        case SB_INST_LOAD:
            cmd->addr = inst->addr;
            cmd->len = inst->size;
            cmd->data = crc_continue(crc(inst->data, inst->size),
                inst->padding, inst->padding_size);
            break;
        case SB_INST_MODE:
            cmd->data = inst->addr;
            break;
        case SB_INST_NOP:
            break;
        default:
            if(g_debug)
                cprintf(u, true, GREY, "die on invalid inst %d\n", inst->inst);
    }
    cmd->hdr.checksum = instruction_checksum(&cmd->hdr);
}

enum sb_error_t sb_write_file(struct sb_file_t *sb, const char *filename, void *u,
    generic_printf_t cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    struct crypto_key_t real_key;
    real_key.method = CRYPTO_KEY;
    byte crypto_iv[16];
    byte (*cbc_macs)[16] = xmalloc(16 * g_nr_keys);
    /* init CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
        memset(cbc_macs[i], 0, 16);
    /* fill gaps */
    fill_gaps(sb);
    /* find first bootable section */
    sb->first_boot_sec = -1;
    for(int i = 0; i < sb->nr_sections; i++)
        if(!sb->sections[i].is_data)
        {
            sb->first_boot_sec = i;
            break;
        }
    if(sb->first_boot_sec == -1)
    {
        cprintf(u, true, GREY, "Image contains no bootable section, I cannot handle that.\n");
        return SB_ERROR;
    }
    /* compute section offsets */
    compute_sb_offsets(sb, u, cprintf);
    /* generate random real key */
    generate_random_data(real_key.u.key, 16);

    /* global SHA-1 */
    struct sha_1_params_t file_sha1;
    sha_1_init(&file_sha1);
    /* produce and write header */
    struct sb_header_t sb_hdr;
    produce_sb_header(sb, &sb_hdr);
    /* allocate image */
    byte *buf = xmalloc(sb_hdr.image_size * BLOCK_SIZE);
    byte *buf_p = buf;
    #define write(p, sz) do { memcpy(buf_p, p, sz); buf_p += sz; } while(0)
    #define check_crypto(expr) \
        do { int err = expr; \
            if(err != CRYPTO_ERROR_SUCCESS) { \
                free(cbc_macs); \
                cprintf(u, true, GREY, "Crypto error: %d\n", err); \
                return SB_CRYPTO_ERROR; } } while(0)

    sha_1_update(&file_sha1, (byte *)&sb_hdr, sizeof(sb_hdr));
    write(&sb_hdr, sizeof(sb_hdr));

    memcpy(crypto_iv, &sb_hdr, 16);

    /* update CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
    {
        check_crypto(crypto_setup(&g_key_array[i]));
        check_crypto(crypto_apply((byte *)&sb_hdr, NULL, sizeof(sb_hdr) / BLOCK_SIZE,
            cbc_macs[i], &cbc_macs[i], true));
    }

    /* produce and write section headers */
    for(int i = 0; i < sb_hdr.nr_sections; i++)
    {
        struct sb_section_header_t sb_sec_hdr;
        produce_sb_section_header(&sb->sections[i], &sb_sec_hdr);
        sha_1_update(&file_sha1, (byte *)&sb_sec_hdr, sizeof(sb_sec_hdr));
        write(&sb_sec_hdr, sizeof(sb_sec_hdr));
        /* update CBC-MACs */
        for(int j = 0; j < g_nr_keys; j++)
        {
            check_crypto(crypto_setup(&g_key_array[j]));
            check_crypto(crypto_apply((byte *)&sb_sec_hdr, NULL,
                sizeof(sb_sec_hdr) / BLOCK_SIZE, cbc_macs[j], &cbc_macs[j], true));
        }
    }
    /* produce key dictionary */
    for(int i = 0; i < g_nr_keys; i++)
    {
        struct sb_key_dictionary_entry_t entry;
        memcpy(entry.hdr_cbc_mac, cbc_macs[i], 16);
        check_crypto(crypto_setup(&g_key_array[i]));
        check_crypto(crypto_apply(real_key.u.key, entry.key, 1, crypto_iv, NULL, true));
        write(&entry, sizeof(entry));
        sha_1_update(&file_sha1, (byte *)&entry, sizeof(entry));
    }

    /* HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK */
    /* Image crafting, don't use it unless you understand what you do */
    if(sb->override_real_key)
        memcpy(real_key.u.key, sb->real_key, 16);
    if(sb->override_crypto_iv)
        memcpy(crypto_iv, sb->crypto_iv, 16);
    /* KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH */
    if(g_debug)
    {
        printf(GREEN, "Real key: ");
        for(int j = 0; j < 16; j++)
            printf(YELLOW, "%02x", real_key.u.key[j]);
        printf(OFF, "\n");
        printf(GREEN, "IV      : ");
        for(int j = 0; j < 16; j++)
            printf(YELLOW, "%02x", crypto_iv[j]);
        printf(OFF, "\n");
    }
    /* the first section might not start right after the header, pad with
     * random data */
    unsigned init_gap = (sb->sections[0].file_offset - 1) * BLOCK_SIZE - (buf_p - buf);
    if(init_gap > 0)
    {
        byte *data = xmalloc(init_gap);
        generate_random_data(data, init_gap);
        sha_1_update(&file_sha1, data, init_gap);
        write(data, init_gap);
        free(data);
    }
    /* setup real key */
    check_crypto(crypto_setup(&real_key));
    /* produce sections data */
    for(int i = 0; i< sb_hdr.nr_sections; i++)
    {
        /* produce tag command */
        struct sb_instruction_tag_t tag_cmd;
        produce_section_tag_cmd(&sb->sections[i], &tag_cmd, (i + 1) == sb_hdr.nr_sections);
        if(g_nr_keys > 0)
        {
            check_crypto(crypto_apply((byte *)&tag_cmd, (byte *)&tag_cmd,
                sizeof(tag_cmd) / BLOCK_SIZE, crypto_iv, NULL, true));
        }
        sha_1_update(&file_sha1, (byte *)&tag_cmd, sizeof(tag_cmd));
        write(&tag_cmd, sizeof(tag_cmd));
        /* produce other commands */
        byte cur_cbc_mac[16];
        memcpy(cur_cbc_mac, crypto_iv, 16);
        for(int j = 0; j < sb->sections[i].nr_insts; j++)
        {
            struct sb_inst_t *inst = &sb->sections[i].insts[j];
            /* command */
            if(inst->inst != SB_INST_DATA)
            {
                struct sb_instruction_common_t cmd;
                produce_sb_instruction(inst, &cmd, u, cprintf);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                {
                    check_crypto(crypto_apply((byte *)&cmd, (byte *)&cmd,
                        sizeof(cmd) / BLOCK_SIZE, cur_cbc_mac, &cur_cbc_mac, true));
                }
                sha_1_update(&file_sha1, (byte *)&cmd, sizeof(cmd));
                write(&cmd, sizeof(cmd));
            }
            /* data */
            if(inst->inst == SB_INST_LOAD || inst->inst == SB_INST_DATA)
            {
                uint32_t sz = inst->size + inst->padding_size;
                byte *data = xmalloc(sz);
                memcpy(data, inst->data, inst->size);
                memcpy(data + inst->size, inst->padding, inst->padding_size);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                {
                    check_crypto(crypto_apply(data, data, sz / BLOCK_SIZE,
                        cur_cbc_mac, &cur_cbc_mac, true));
                }
                sha_1_update(&file_sha1, data, sz);
                write(data, sz);
                free(data);
            }
        }
        /* pad section with random data or NOP */
        uint32_t pad_size = sb->sections[i].pad_size;
        if(sb->sections[i].is_data)
        {
            byte *data = xmalloc(pad_size * BLOCK_SIZE);
            generate_random_data(data, pad_size * BLOCK_SIZE);
            sha_1_update(&file_sha1, data, pad_size * BLOCK_SIZE);
            write(data, pad_size * BLOCK_SIZE);
            free(data);
        }
        else
        {
            for(unsigned j = 0; j < pad_size; j++)
            {
                struct sb_instruction_nop_t cmd;
                memset(&cmd, 0, sizeof(cmd));
                cmd.hdr.opcode = SB_INST_NOP;
                cmd.hdr.checksum = instruction_checksum(&cmd.hdr);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                {
                    check_crypto(crypto_apply((byte *)&cmd, (byte *)&cmd,
                        sizeof(cmd) / BLOCK_SIZE, cur_cbc_mac, &cur_cbc_mac, true));
                }
                sha_1_update(&file_sha1, (byte *)&cmd, sizeof(cmd));
                write(&cmd, sizeof(cmd));
            }
        }
    }
    /* write file SHA-1 */
    byte final_sig[32];
    sha_1_finish(&file_sha1);
    sha_1_output(&file_sha1, final_sig);
    generate_random_data(final_sig + 20, 12);
    if(g_nr_keys > 0)
        check_crypto(crypto_apply(final_sig, final_sig, 2, crypto_iv, NULL, true));
    write(final_sig, 32);

    free(cbc_macs);

    if(buf_p - buf != sb_hdr.image_size * BLOCK_SIZE)
    {
        free(buf);
        printf(GREY, "Internal error: SB image buffer was not entirely filled !\n");
        printf(GREY, "Internal error: expected %u blocks, got %u\n",
            (buf_p - buf) / BLOCK_SIZE, sb_hdr.image_size);
        cprintf(u, true, GREY, "Internal error\n");
        return SB_ERROR;
    }

    FILE *fd = fopen(filename, "wb");
    if(fd == NULL)
        return SB_OPEN_ERROR;
    int cnt = fwrite(buf, sb_hdr.image_size * BLOCK_SIZE, 1, fd);
    if(cnt != 1)
        printf(GREY, "Write error: %m\n");
    free(buf);
    fclose(fd);
    if(cnt != 1)
        return SB_WRITE_ERROR;

    return SB_SUCCESS;
    #undef check_crypto
    #undef printf
}

static struct sb_section_t *read_section(bool data_sec, uint32_t id, byte *buf,
    int size, const char *indent, void *u, generic_printf_t cprintf, enum sb_error_t *err)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            sb_free_section(*sec); \
            free(sec); \
            return NULL; } while(0)

    struct sb_section_t *sec = xmalloc(sizeof(struct sb_section_t));
    memset(sec, 0, sizeof(struct sb_section_t));
    sec->identifier = id;
    sec->is_data = data_sec;
    sec->sec_size = ROUND_UP(size, BLOCK_SIZE) / BLOCK_SIZE;

    if(data_sec)
    {
        sec->nr_insts = 1;
        sec->insts = xmalloc(sizeof(struct sb_inst_t));
        memset(sec->insts, 0, sizeof(struct sb_inst_t));
        sec->insts->inst = SB_INST_DATA;
        sec->insts->size = size;
        sec->insts->data = memdup(buf, size);
        return sec;
    }

    /* Pretty print the content */
    int pos = 0;
    while(pos < size)
    {
        struct sb_inst_t inst;
        memset(&inst, 0, sizeof(inst));

        struct sb_instruction_header_t *hdr = (struct sb_instruction_header_t *)&buf[pos];
        inst.inst = hdr->opcode;

        printf(OFF, "%s", indent);
        uint8_t checksum = instruction_checksum(hdr);
        if(checksum != hdr->checksum)
            fatal(SB_CHECKSUM_ERROR, "Bad instruction checksum\n");
        if(hdr->flags != 0)
        {
            printf(GREY, "[");
            printf(BLUE, "f=%x", hdr->flags);
            printf(GREY, "] ");
        }
        if(hdr->opcode == SB_INST_LOAD)
        {
            struct sb_instruction_load_t *load = (struct sb_instruction_load_t *)&buf[pos];
            inst.size = load->len;
            inst.addr = load->addr;
            inst.data = memdup(load + 1, load->len);

            printf(RED, "LOAD");
            printf(OFF, " | ");
            printf(BLUE, "addr=0x%08x", load->addr);
            printf(OFF, " | ");
            printf(GREEN, "len=0x%08x", load->len);
            printf(OFF, " | ");
            printf(YELLOW, "crc=0x%08x", load->crc);
            /* data is padded to 16-byte boundary with random data and crc'ed with it */
            uint32_t computed_crc = crc(&buf[pos + sizeof(struct sb_instruction_load_t)],
                ROUND_UP(load->len, 16));
            if(load->crc == computed_crc)
                printf(RED, "  Ok\n");
            else
            {
                printf(RED, "  Failed (crc=0x%08x)\n", computed_crc);
                fatal(SB_CHECKSUM_ERROR, "Instruction data crc error\n");
            }

            pos += load->len + sizeof(struct sb_instruction_load_t);
        }
        else if(hdr->opcode == SB_INST_FILL)
        {
            struct sb_instruction_fill_t *fill = (struct sb_instruction_fill_t *)&buf[pos];
            inst.pattern = fill->pattern;
            inst.size = fill->len;
            inst.addr = fill->addr;

            printf(RED, "FILL");
            printf(OFF, " | ");
            printf(BLUE, "addr=0x%08x", fill->addr);
            printf(OFF, " | ");
            printf(GREEN, "len=0x%08x", fill->len);
            printf(OFF, " | ");
            printf(YELLOW, "pattern=0x%08x\n", fill->pattern);

            pos += sizeof(struct sb_instruction_fill_t);
        }
        else if(hdr->opcode == SB_INST_CALL ||
                hdr->opcode == SB_INST_JUMP)
        {
            int is_call = (hdr->opcode == SB_INST_CALL);
            struct sb_instruction_call_t *call = (struct sb_instruction_call_t *)&buf[pos];
            inst.addr = call->addr;
            inst.argument = call->arg;

            if(is_call)
                printf(RED, "CALL");
            else
                printf(RED, "JUMP");
            printf(OFF, " | ");
            printf(BLUE, "addr=0x%08x", call->addr);
            printf(OFF, " | ");
            printf(GREEN, "arg=0x%08x\n", call->arg);

            pos += sizeof(struct sb_instruction_call_t);
        }
        else if(hdr->opcode == SB_INST_MODE)
        {
            struct sb_instruction_mode_t *mode = (struct sb_instruction_mode_t *)hdr;
            inst.argument = mode->mode;

            printf(RED, "MODE");
            printf(OFF, " | ");
            printf(BLUE, "mod=0x%08x\n", mode->mode);

            pos += sizeof(struct sb_instruction_mode_t);
        }
        else if(hdr->opcode == SB_INST_NOP)
        {
            printf(RED, "NOOP\n");
            pos += sizeof(struct sb_instruction_mode_t);
        }
        else
        {
            fatal(SB_FORMAT_ERROR, "Unknown instruction %d at address 0x%08lx\n", hdr->opcode, (unsigned long)pos);
            break;
        }

        sec->insts = augment_array(sec->insts, sizeof(struct sb_inst_t), sec->nr_insts++, &inst, 1);
        pos = ROUND_UP(pos, BLOCK_SIZE);
    }

    return sec;
    #undef printf
    #undef fatal
}

void sb_fill_section_name(char name[5], uint32_t identifier)
{
    name[0] = (identifier >> 24) & 0xff;
    name[1] = (identifier >> 16) & 0xff;
    name[2] = (identifier >> 8) & 0xff;
    name[3] = identifier & 0xff;
    for(int i = 0; i < 4; i++)
        if(!isprint(name[i]))
            name[i] = '_';
    name[4] = 0;
}

static uint32_t guess_alignment(uint32_t off)
{
    /* find greatest power of two which divides the offset */
    if(off == 0)
        return 1;
    uint32_t a = 1;
    while(off % (2 * a) == 0)
        a *= 2;
    return a;
}

struct sb_file_t *sb_read_file(const char *filename, unsigned flags, void *u,
    generic_printf_t cprintf, enum sb_error_t *err)
{
    return sb_read_file_ex(filename, 0, -1, flags, u, cprintf, err);
}

struct sb_file_t *sb_read_file_ex(const char *filename, size_t offset, size_t size,
    unsigned flags, void *u, generic_printf_t cprintf, enum sb_error_t *err)
{
    #define fatal(e, ...) \
        do { if(err) *err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            free(buf); \
            return NULL; } while(0)

    FILE *f = fopen(filename, "rb");
    void *buf = NULL;
    if(f == NULL)
        fatal(SB_OPEN_ERROR, "Cannot open file for reading\n");
    fseek(f, 0, SEEK_END);
    size_t read_size = ftell(f);
    fseek(f, offset, SEEK_SET);
    if(size != (size_t)-1)
        read_size = size;
    buf = xmalloc(read_size);
    if(fread(buf, read_size, 1, f) != 1)
    {
        fclose(f);
        fatal(SB_READ_ERROR, "Cannot read file\n");
    }
    fclose(f);

    struct sb_file_t *ret = sb_read_memory(buf, read_size, flags, u, cprintf, err);
    free(buf);
    return ret;

    #undef fatal
}

struct printer_t
{
    void *user;
    generic_printf_t cprintf;
    const char *color;
    bool error;
};

static void sb_printer(void *user, const char *fmt, ...)
{
    struct printer_t *p = user;
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    p->cprintf(p->user, p->error, p->color, "%s", buffer);
    va_end(args);
}

struct sb_file_t *sb_read_memory(void *_buf, size_t filesize, unsigned flags, void *u,
    generic_printf_t cprintf, enum sb_error_t *out_err)
{
    struct sb_file_t *sb_file = NULL;
    uint8_t *buf = _buf;

    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    #define fatal(e, ...) \
        do { if(out_err) *out_err = e; \
            cprintf(u, true, GREY, __VA_ARGS__); \
            free(cbcmacs); \
            sb_free(sb_file); \
            return NULL; } while(0)
    struct printer_t printer = {.user = u, .cprintf = cprintf, .color = OFF, .error = false };
    #define print_hex(c, p, len, nl) \
        do { printer.color = c; print_hex(&printer, sb_printer, p, len, nl); } while(0)
    #define check_crypto(expr) \
        do { int err = expr; \
            if(err != CRYPTO_ERROR_SUCCESS) \
                fatal(SB_CRYPTO_ERROR, "Crypto error: %d\n", err); } while(0)

    struct sha_1_params_t sha_1_params;
    byte (*cbcmacs)[16] = xmalloc(16 * g_nr_keys);
    sb_file = xmalloc(sizeof(struct sb_file_t));
    memset(sb_file, 0, sizeof(struct sb_file_t));
    struct sb_header_t *sb_header = (struct sb_header_t *)buf;

    sb_file->image_size = sb_header->image_size;
    sb_file->minor_version = sb_header->minor_ver;
    sb_file->flags = sb_header->flags;
    sb_file->drive_tag = sb_header->drive_tag;
    sb_file->first_boot_sec_id = sb_header->first_boot_sec_id;

    if(memcmp(sb_header->signature, "STMP", 4) != 0)
        fatal(SB_FORMAT_ERROR, "Bad signature\n");
    if(sb_header->image_size * BLOCK_SIZE > filesize)
        fatal(SB_FORMAT_ERROR, "File too small (should be at least %d bytes)\n",
            sb_header->image_size * BLOCK_SIZE);
    if(sb_header->header_size * BLOCK_SIZE != sizeof(struct sb_header_t))
        fatal(SB_FORMAT_ERROR, "Bad header size\n");
    if(sb_header->sec_hdr_size * BLOCK_SIZE != sizeof(struct sb_section_header_t))
        fatal(SB_FORMAT_ERROR, "Bad section header size\n");

    if(filesize > sb_header->image_size * BLOCK_SIZE)
    {
        printf(GREY, "[Restrict file size from %lu to %d bytes]\n", filesize,
            sb_header->image_size * BLOCK_SIZE);
        filesize = sb_header->image_size * BLOCK_SIZE;
    }

    printf(BLUE, "Basic info:\n");
    printf(GREEN, "  SB version: ");
    printf(YELLOW, "%d.%d\n", sb_header->major_ver, sb_header->minor_ver);
    printf(GREEN, "  Header SHA-1: ");
    byte *hdr_sha1 = sb_header->sha1_header;
    print_hex(YELLOW, hdr_sha1, 20, false);
    /* Check SHA1 sum */
    byte computed_sha1[20];
    sha_1_init(&sha_1_params);
    sha_1_update(&sha_1_params, &sb_header->signature[0],
        sizeof(struct sb_header_t) - sizeof(sb_header->sha1_header));
    sha_1_finish(&sha_1_params);
    sha_1_output(&sha_1_params, computed_sha1);
    if(memcmp(hdr_sha1, computed_sha1, 20) == 0)
        printf(RED, " Ok\n");
    else
    {
        printf(RED, " Failed\n");
        if(!(flags & SB_IGNORE_SHA1))
            fatal(SB_FORMAT_ERROR, "Bad header checksum\n");
    }
    printf(GREEN, "  Flags: ");
    printf(YELLOW, "%x\n", sb_header->flags);
    printf(GREEN, "  Total file size : ");
    printf(YELLOW, "%ld\n", filesize);

    /* Sizes and offsets */
    printf(BLUE, "Sizes and offsets:\n");
    printf(GREEN, "  # of encryption keys = ");
    printf(YELLOW, "%d\n", sb_header->nr_keys);
    printf(GREEN, "  # of sections = ");
    printf(YELLOW, "%d\n", sb_header->nr_sections);

    /* Versions */
    printf(BLUE, "Versions\n");

    printf(GREEN, "  Random 1: ");
    print_hex(YELLOW, sb_header->rand_pad0, sizeof(sb_header->rand_pad0), true);
    printf(GREEN, "  Random 2: ");
    print_hex(YELLOW, sb_header->rand_pad1, sizeof(sb_header->rand_pad1), true);

    uint64_t micros = sb_header->timestamp;
    time_t seconds = (micros / (uint64_t)1000000L);
    struct tm tm_base;
    memset(&tm_base, 0, sizeof(tm_base));
    /* 2000/1/1 0:00:00 */
    tm_base.tm_mday = 1;
    tm_base.tm_year = 100;
    seconds += mktime(&tm_base);
    struct tm *time = gmtime(&seconds);
    printf(GREEN, "  Creation date/time = ");
    printf(YELLOW, "%s", asctime(time));
    sb_file->timestamp = sb_header->timestamp;

    struct sb_version_t product_ver = sb_header->product_ver;
    fix_version(&product_ver);
    struct sb_version_t component_ver = sb_header->component_ver;
    fix_version(&component_ver);

    memcpy(&sb_file->product_ver, &product_ver, sizeof(product_ver));
    memcpy(&sb_file->component_ver, &component_ver, sizeof(component_ver));

    printf(GREEN, "  Product version   = ");
    printf(YELLOW, "%X.%X.%X\n", product_ver.major, product_ver.minor, product_ver.revision);
    printf(GREEN, "  Component version = ");
    printf(YELLOW, "%X.%X.%X\n", component_ver.major, component_ver.minor, component_ver.revision);

    printf(GREEN, "  Drive tag = ");
    printf(YELLOW, "%x\n", sb_header->drive_tag);
    printf(GREEN, "  First boot tag offset = ");
    printf(YELLOW, "%x\n", sb_header->first_boot_tag_off);
    printf(GREEN, "  First boot section ID = ");
    printf(YELLOW, "0x%08x\n", sb_header->first_boot_sec_id);

    /* encryption cbc-mac */
    struct crypto_key_t real_key;
    real_key.method = CRYPTO_KEY;
    bool valid_key = false; /* false until a matching key was found */

    if(sb_header->nr_keys > 0)
    {
        printf(BLUE, "Encryption keys\n");
        for(int i = 0; i < g_nr_keys; i++)
        {
            printf(RED, "  Key %d\n", i),
            printf(GREEN, "    Key: ");
            printer.color = YELLOW;
            print_key(&printer, sb_printer, &g_key_array[i], true);
            printf(GREEN, "    CBC-MAC: ");
            /* check it */
            byte zero[16];
            memset(zero, 0, 16);
            check_crypto(crypto_setup(&g_key_array[i]));
            check_crypto(crypto_apply(buf, NULL, sb_header->header_size +
                sb_header->nr_sections, zero, &cbcmacs[i], true));
            print_hex(YELLOW, cbcmacs[i], 16, true);
        }

        printf(BLUE, "DEK\n");
        for(int i = 0; i < sb_header->nr_keys; i++)
        {
            printf(RED, "  Entry %d\n", i);
            uint32_t ofs = sizeof(struct sb_header_t)
                + sizeof(struct sb_section_header_t) * sb_header->nr_sections
                + sizeof(struct sb_key_dictionary_entry_t) * i;
            struct sb_key_dictionary_entry_t *dict_entry =
                (struct sb_key_dictionary_entry_t *)&buf[ofs];
            /* cbc mac */
            printf(GREEN, "    Encrypted key: ");
            print_hex(YELLOW, dict_entry->key, 16, true);
            printf(GREEN, "    CBC-MAC      : ");
            print_hex(YELLOW, dict_entry->hdr_cbc_mac, 16, false);
            /* check it */
            int idx = 0;
            while(idx < g_nr_keys && memcmp(dict_entry->hdr_cbc_mac, cbcmacs[idx], 16) != 0)
                idx++;
            if(idx != g_nr_keys)
            {
                printf(RED, " Match\n");
                /* decrypt */
                byte decrypted_key[16];
                byte iv[16];
                memcpy(iv, buf, 16); /* uses the first 16-bytes of SHA-1 sig as IV */
                check_crypto(crypto_setup(&g_key_array[idx]));
                check_crypto(crypto_apply(dict_entry->key, decrypted_key, 1, iv, NULL, false));
                printf(GREEN, "    Decrypted key: ");
                print_hex(YELLOW, decrypted_key, 16, false);
                if(valid_key)
                {
                    if(memcmp(real_key.u.key, decrypted_key, 16) == 0)
                        printf(RED, " Cross-Check Ok");
                    else
                        printf(RED, " Cross-Check Failed");
                }
                else
                {
                    memcpy(real_key.u.key, decrypted_key, 16);
                    valid_key = true;
                }
                printf(OFF, "\n");
            }
            else
                printf(RED, " Don't Match\n");
        }

        if(!valid_key)
        {
            if(g_force)
                printf(GREY, "  No valid key found\n");
            else
                fatal(SB_NO_VALID_KEY, "No valid key found\n");
        }

        if(getenv("SB_REAL_KEY") != 0)
        {
            char *env = getenv("SB_REAL_KEY");
            if(!parse_key(&env, &real_key) || *env)
                fatal(SB_ERROR, "Invalid SB_REAL_KEY\n");
            /* assume the key is valid */
            if(valid_key)
                printf(GREY, "  Overriding real key\n");
            else
                printf(GREY, "  Assuming real key is ok\n");
            valid_key = true;
        }

        printf(RED, "  Summary:\n");
        printf(GREEN, "    Real key: ");
        print_hex(YELLOW, real_key.u.key, 16, true);
        printf(GREEN, "    IV      : ");
        print_hex(YELLOW, buf, 16, true);

        memcpy(sb_file->real_key, real_key.u.key, 16);
        memcpy(sb_file->crypto_iv, buf, 16);
        /* setup real key if needed */
        check_crypto(crypto_setup(&real_key));
    }
    else
        valid_key = true;
    /* sections */
    if(!(flags & SB_RAW_MODE))
    {
        sb_file->nr_sections = sb_header->nr_sections;
        sb_file->sections = xmalloc(sb_file->nr_sections * sizeof(struct sb_section_t));
        memset(sb_file->sections, 0, sb_file->nr_sections * sizeof(struct sb_section_t));
        printf(BLUE, "Sections\n");
        for(int i = 0; i < sb_header->nr_sections; i++)
        {
            uint32_t ofs = sb_header->header_size * BLOCK_SIZE + i * sizeof(struct sb_section_header_t);
            struct sb_section_header_t *sec_hdr = (struct sb_section_header_t *)&buf[ofs];

            char name[5];
            sb_fill_section_name(name, sec_hdr->identifier);
            int pos = sec_hdr->offset * BLOCK_SIZE;
            int size = sec_hdr->size * BLOCK_SIZE;
            int data_sec = !(sec_hdr->flags & SECTION_BOOTABLE);
            int encrypted = !(sec_hdr->flags & SECTION_CLEARTEXT) && sb_header->nr_keys > 0;

            printf(GREEN, "  Section ");
            printf(YELLOW, "'%s'\n", name);
            printf(GREEN, "    pos   = ");
            printf(YELLOW, "%8x - %8x\n", pos, pos+size);
            printf(GREEN, "    len   = ");
            printf(YELLOW, "%8x\n", size);
            printf(GREEN, "    flags = ");
            printf(YELLOW, "%8x", sec_hdr->flags);
            if(data_sec)
                printf(RED, "  Data Section");
            else
                printf(RED, "  Boot Section");
            if(encrypted)
                printf(RED, " (Encrypted)");
            printf(OFF, "\n");

            /* skip it if we cannot decrypt it */
            if(encrypted && !valid_key)
            {
                printf(GREY, "  Skipping section content (no valid key)\n");
                continue;
            }

            /* save it */
            byte *sec = xmalloc(size);
            if(encrypted)
                check_crypto(crypto_apply(buf + pos, sec, size / BLOCK_SIZE, buf, NULL, false));
            else
                memcpy(sec, buf + pos, size);

            struct sb_section_t *s = read_section(data_sec, sec_hdr->identifier,
                sec, size, "      ", u, cprintf, out_err);
            free(sec);
            if(s)
            {
                s->other_flags = sec_hdr->flags & ~SECTION_STD_MASK;
                s->is_cleartext = !encrypted;
                s->alignment = guess_alignment(pos);
                memcpy(&sb_file->sections[i], s, sizeof(struct sb_section_t));
                free(s);
            }
            else
                fatal(*out_err, "Error reading section\n");
        }
    }
    else if(valid_key)
    {
        /* advanced raw mode */
        printf(BLUE, "Commands\n");
        uint32_t offset = sb_header->first_boot_tag_off * BLOCK_SIZE;
        byte iv[16];
        const char *indent = "    ";
        while(true)
        {
            /* restart with IV */
            memcpy(iv, buf, 16);
            byte cmd[BLOCK_SIZE];
            if(sb_header->nr_keys > 0)
                check_crypto(crypto_apply(buf + offset, cmd, 1, iv, &iv, false));
            else
                memcpy(cmd, buf + offset, BLOCK_SIZE);
            struct sb_instruction_header_t *hdr = (struct sb_instruction_header_t *)cmd;
            printf(OFF, "%s", indent);
            uint8_t checksum = instruction_checksum(hdr);
            if(checksum != hdr->checksum)
                printf(GREY, "[Bad checksum]");

            if(hdr->opcode == SB_INST_NOP)
            {
                printf(RED, "NOOP\n");
                offset += BLOCK_SIZE;
            }
            else if(hdr->opcode == SB_INST_TAG)
            {
                struct sb_instruction_tag_t *tag = (struct sb_instruction_tag_t *)hdr;
                printf(RED, "BTAG");
                printf(OFF, " | ");
                printf(BLUE, "sec=0x%08x", tag->identifier);
                printf(OFF, " | ");
                printf(GREEN, "cnt=0x%08x", tag->len);
                printf(OFF, " | ");
                printf(YELLOW, "flg=0x%08x", tag->flags);
                if(tag->hdr.flags & SB_INST_LAST_TAG)
                {
                    printf(OFF, " | ");
                    printf(RED, " Last section");
                }
                printf(OFF, "\n");
                offset += sizeof(struct sb_instruction_tag_t);

                char name[5];
                sb_fill_section_name(name, tag->identifier);
                int pos = offset;
                int size = tag->len * BLOCK_SIZE;
                int data_sec = !(tag->flags & SECTION_BOOTABLE);
                int encrypted = !(tag->flags & SECTION_CLEARTEXT) && sb_header->nr_keys > 0;

                printf(GREEN, "%sSection ", indent);
                printf(YELLOW, "'%s'\n", name);
                printf(GREEN, "%s  pos   = ", indent);
                printf(YELLOW, "%8x - %8x\n", pos, pos+size);
                printf(GREEN, "%s  len   = ", indent);
                printf(YELLOW, "%8x\n", size);
                printf(GREEN, "%s  flags = ", indent);
                printf(YELLOW, "%8x", tag->flags);
                if(data_sec)
                    printf(RED, "  Data Section");
                else
                    printf(RED, "  Boot Section");
                if(encrypted)
                    printf(RED, " (Encrypted)");
                printf(OFF, "\n");

                /* skip it if we cannot decrypt it */
                if(encrypted && !valid_key)
                {
                    printf(GREY, "  Skipping section content (no valid key)\n");
                    offset += size;
                    continue;
                }

                /* save it */
                byte *sec = xmalloc(size);
                if(encrypted)
                    check_crypto(crypto_apply(buf + pos, sec, size / BLOCK_SIZE, buf, NULL, false));
                else
                    memcpy(sec, buf + pos, size);

                struct sb_section_t *s = read_section(data_sec, tag->identifier,
                    sec, size, "      ", u, cprintf, out_err);
                free(sec);
                if(s)
                {
                    s->other_flags = tag->flags & ~SECTION_STD_MASK;
                    s->is_cleartext = !encrypted;
                    s->alignment = guess_alignment(pos);
                    sb_file->sections = augment_array(sb_file->sections,
                        sizeof(struct sb_section_t), sb_file->nr_sections++,
                        s, 1);
                    free(s);
                }
                else
                    fatal(*out_err, "Error reading section\n");

                /* last one ? */
                if(tag->hdr.flags & SB_INST_LAST_TAG)
                    break;
                offset += size;
            }
            else
            {
                fatal(SB_FORMAT_ERROR, "Unknown instruction %d at address 0x%08lx\n", hdr->opcode, (long)offset);
                break;
            }
        }
    }
    else
    {
        printf(GREY, "Cannot read content in raw mode without a valid key\n");
    }

    /* final signature */
    printf(BLUE, "Final signature:\n");
    byte decrypted_block[32];
    if(sb_header->nr_keys > 0)
    {
        printf(GREEN, "  Encrypted SHA-1:\n");
        byte *encrypted_block = &buf[filesize - 32];
        printf(OFF, "    ");
        print_hex(YELLOW, encrypted_block, 16, true);
        printf(OFF, "    ");
        print_hex(YELLOW, encrypted_block + 16, 16, true);
        /* decrypt it */
        check_crypto(crypto_apply(encrypted_block, decrypted_block, 2, buf, NULL, false));
    }
    else
        memcpy(decrypted_block, &buf[filesize - 32], 32);
    printf(GREEN, "  File SHA-1:\n    ");
    print_hex(YELLOW, decrypted_block, 20, false);
    /* check it */
    sha_1_init(&sha_1_params);
    sha_1_update(&sha_1_params, buf, filesize - 32);
    sha_1_finish(&sha_1_params);
    sha_1_output(&sha_1_params, computed_sha1);
    if(memcmp(decrypted_block, computed_sha1, 20) == 0)
        printf(RED, " Ok\n");
    else if(flags & SB_IGNORE_SHA1)
    {
        /* some weird images produced by some buggy tools have wrong SHA-1,
         * this probably gone unnoticed because the bootloader ignores the SHA-1
         * anyway */
        printf(RED, " Failed\n");
        cprintf(u, true, GREY, "Warning: SHA-1 mismatch ignored per flags\n");
    }
    else
    {
        printf(RED, " Failed\n");
        fatal(SB_CHECKSUM_ERROR, "File SHA-1 error\n");
    }

    free(cbcmacs);
    return sb_file;
    #undef printf
    #undef fatal
    #undef print_hex
}

void sb_generate_default_version(struct sb_version_t *ver)
{
    ver->major = ver->minor = ver->revision = 0x999;
}

void sb_build_default_image(struct sb_file_t *sb)
{
    sb->minor_version = IMAGE_MINOR_VERSION;
    sb->timestamp = sb_generate_timestamp();
    sb_generate_default_version(&sb->product_ver);
    sb_generate_default_version(&sb->component_ver);
}

void sb_free_instruction(struct sb_inst_t inst)
{
    free(inst.padding);
    free(inst.data);
}

void sb_free_section(struct sb_section_t sec)
{
    for(int j = 0; j < sec.nr_insts; j++)
        sb_free_instruction(sec.insts[j]);
    free(sec.insts);
}

void sb_free(struct sb_file_t *file)
{
    if(!file) return;

    for(int i = 0; i < file->nr_sections; i++)
        sb_free_section(file->sections[i]);

    free(file->sections);
    free(file);
}

void sb_dump(struct sb_file_t *file, void *u, generic_printf_t cprintf)
{
    #define printf(c, ...) cprintf(u, false, c, __VA_ARGS__)
    struct printer_t printer = {.user = u, .cprintf = cprintf, .color = OFF, .error = false };
    #define print_hex(c, p, len, nl) \
        do { printer.color = c; print_hex(&printer, sb_printer, p, len, nl); } while(0)

    #define TREE    RED
    #define HEADER  GREEN
    #define TEXT    YELLOW
    #define TEXT2   BLUE
    #define SEP     OFF

    printf(BLUE, "SB File\n");
    printf(TREE, "+-");
    printf(HEADER, "Version: ");
    printf(TEXT, "1.%d\n", file->minor_version);
    printf(TREE, "+-");
    printf(HEADER, "Flags: ");
    printf(TEXT, "%x\n", file->flags);
    printf(TREE, "+-");
    printf(HEADER, "Drive Tag: ");
    printf(TEXT, "%x\n", file->drive_tag);
    printf(TREE, "+-");
    printf(HEADER, "First Boot Section ID: ");
    char name[5];
    sb_fill_section_name(name, file->first_boot_sec_id);
    printf(TEXT, "%08x (%s)\n", file->first_boot_sec_id, name);
    printf(TREE, "+-");
    printf(HEADER, "Timestamp: ");
    printf(TEXT, "%#llx", file->timestamp);
    {
        uint64_t micros = file->timestamp;
        time_t seconds = (micros / (uint64_t)1000000L);
        struct tm tm_base;
        memset(&tm_base, 0, sizeof(tm_base));
        /* 2000/1/1 0:00:00 */
        tm_base.tm_mday = 1;
        tm_base.tm_year = 100;
        seconds += mktime(&tm_base);
        struct tm *time = gmtime(&seconds);
        char *str = asctime(time);
        str[strlen(str) - 1] = 0;
        printf(TEXT2, " (%s)\n", str);
    }

    if(file->override_real_key)
    {
        printf(TREE, "+-");
        printf(HEADER, "Real key: ");
        print_hex(TEXT, file->real_key, 16, true);
    }
    if(file->override_crypto_iv)
    {
        printf(TREE, "+-");
        printf(HEADER, "IV      : ");
        print_hex(TEXT, file->crypto_iv, 16, true);
    }
    printf(TREE, "+-");
    printf(HEADER, "Product Version: ");
    printf(TEXT, "%X.%X.%X\n", file->product_ver.major, file->product_ver.minor,
        file->product_ver.revision);
    printf(TREE, "+-");
    printf(HEADER, "Component Version: ");
    printf(TEXT, "%X.%X.%X\n", file->component_ver.major, file->component_ver.minor,
        file->component_ver.revision);

    for(int i = 0; i < file->nr_sections; i++)
    {
        struct sb_section_t *sec = &file->sections[i];
        printf(TREE, "+-");
        printf(HEADER, "Section\n");
        printf(TREE,"|  +-");
        printf(HEADER, "Identifier: ");
        sb_fill_section_name(name, sec->identifier);
        printf(TEXT, "%08x (%s)\n", sec->identifier, name);
        printf(TREE, "|  +-");
        printf(HEADER, "Type: ");
        printf(TEXT, "%s (%s)\n", sec->is_data ? "Data Section" : "Boot Section",
            sec->is_cleartext ? "Cleartext" : "Encrypted");
        printf(TREE, "|  +-");
        printf(HEADER, "Alignment: ");
        printf(TEXT, "%d (bytes)\n", sec->alignment);
        printf(TREE, "|  +-");
        printf(HEADER, "Other Flags: ");
        printf(TEXT, "%#x\n", sec->other_flags);
        printf(TREE, "|  +-");
        printf(HEADER, "Instructions\n");
        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            printf(TREE, "|  |  +-");
            switch(inst->inst)
            {
                case SB_INST_DATA:
                    printf(HEADER, "DATA");
                    printf(SEP, " | ");
                    printf(TEXT, "size=0x%08x\n", inst->size);
                    break;
                case SB_INST_CALL:
                case SB_INST_JUMP:
                    printf(HEADER, "%s", inst->inst == SB_INST_CALL ? "CALL" : "JUMP");
                    printf(SEP, " | ");
                    printf(TEXT, "addr=0x%08x", inst->addr);
                    printf(SEP, " | ");
                    printf(TEXT2, "arg=0x%08x\n", inst->argument);
                    break;
                case SB_INST_LOAD:
                    printf(HEADER, "LOAD");
                    printf(SEP, " | ");
                    printf(TEXT, "addr=0x%08x", inst->addr);
                    printf(SEP, " | ");
                    printf(TEXT2, "len=0x%08x\n", inst->size);
                    break;
                case SB_INST_FILL:
                    printf(HEADER, "FILL");
                    printf(SEP, " | ");
                    printf(TEXT, "addr=0x%08x", inst->addr);
                    printf(SEP, " | ");
                    printf(TEXT2, "len=0x%08x", inst->size);
                    printf(SEP, " | ");
                    printf(TEXT2, "pattern=0x%08x\n", inst->pattern);
                    break;
                case SB_INST_MODE:
                    printf(HEADER, "MODE");
                    printf(SEP, " | ");
                    printf(TEXT, "mod=0x%08x\n", inst->addr);
                    break;
                case SB_INST_NOP:
                    printf(HEADER, "NOOP\n");
                    break;
                default:
                    printf(GREY, "[Unknown instruction %x]\n", inst->inst);
            }
        }
    }

    #undef printf
    #undef print_hex
}

void sb_get_zero_key(struct crypto_key_t *key)
{
    key->method = CRYPTO_KEY;
    memset(key->u.key, 0, sizeof(key->u.key));
}
