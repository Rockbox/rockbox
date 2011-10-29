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
#include "misc.h"
#include "crypto.h"
#include "sb.h"

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

static void compute_sb_offsets(struct sb_file_t *sb)
{
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
        /* each section has a preliminary TAG command */
        sb->image_size += sizeof(struct sb_instruction_tag_t) / BLOCK_SIZE;
        /* we might need to pad the section so compute next alignment */
        uint32_t alignment = BLOCK_SIZE;
        if((i + 1) < sb->nr_sections)
            alignment = sb->sections[i + 1].alignment;
        alignment /= BLOCK_SIZE; /* alignment in block sizes */
        
        struct sb_section_t *sec = &sb->sections[i];

        if(g_debug)
        {
            printf("%s section 0x%08x", sec->is_data ? "Data" : "Boot",
                sec->identifier);
            if(sec->is_cleartext)
                printf(" (cleartext)");
            printf("\n");
        }
        
        sec->file_offset = sb->image_size;
        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            if(inst->inst == SB_INST_CALL || inst->inst == SB_INST_JUMP)
            {
                if(g_debug)
                    printf("  %s | addr=0x%08x | arg=0x%08x\n",
                        inst->inst == SB_INST_CALL ? "CALL" : "JUMP", inst->addr, inst->argument);
                sb->image_size += sizeof(struct sb_instruction_call_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_call_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_FILL)
            {
                if(g_debug)
                    printf("  FILL | addr=0x%08x | len=0x%08x | pattern=0x%08x\n",
                        inst->addr, inst->size, inst->pattern);
                sb->image_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_LOAD)
            {
                if(g_debug)
                    printf("  LOAD | addr=0x%08x | len=0x%08x\n", inst->addr, inst->size);
                /* load header */
                sb->image_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                /* data + alignment */
                sb->image_size += (inst->size + inst->padding_size) / BLOCK_SIZE;
                sec->sec_size += (inst->size + inst->padding_size) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_MODE)
            {
                if(g_debug)
                    printf("  MODE | mod=0x%08x", inst->addr);
                sb->image_size += sizeof(struct sb_instruction_mode_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_mode_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_DATA)
            {
                if(g_debug)
                    printf("  DATA | size=0x%08x\n", inst->size);
                sb->image_size += ROUND_UP(inst->size, BLOCK_SIZE) / BLOCK_SIZE;
                sec->sec_size += ROUND_UP(inst->size, BLOCK_SIZE) / BLOCK_SIZE;
            }
            else
                bug("die on inst %d\n", inst->inst);
        }
        /* we need to make sure next section starts on the right alignment.
         * Since each section starts with a boot tag, we thus need to ensure
         * that this sections ends at adress X such that X+BLOCK_SIZE is
         * a multiple of the alignment.
         * For data sections, we just add random data, otherwise we add nops */
        uint32_t missing_sz = alignment - ((sb->image_size + 1) % alignment);
        if(missing_sz != alignment)
        {
            struct sb_inst_t *aug_insts;
            int nr_aug_insts = 0;

            if(sb->sections[i].is_data)
            {
                nr_aug_insts = 1;
                aug_insts = malloc(sizeof(struct sb_inst_t));
                memset(aug_insts, 0, sizeof(struct sb_inst_t));
                aug_insts[0].inst = SB_INST_DATA;
                aug_insts[0].size = missing_sz * BLOCK_SIZE;
                aug_insts[0].data = xmalloc(missing_sz * BLOCK_SIZE);
                generate_random_data(aug_insts[0].data, missing_sz * BLOCK_SIZE);
                if(g_debug)
                    printf("  DATA | size=0x%08x\n", aug_insts[0].size);
            }
            else
            {
                nr_aug_insts = missing_sz;
                aug_insts = malloc(sizeof(struct sb_inst_t) * nr_aug_insts);
                memset(aug_insts, 0, sizeof(struct sb_inst_t) * nr_aug_insts);
                for(int j = 0; j < nr_aug_insts; j++)
                {
                    aug_insts[j].inst = SB_INST_NOP;
                    if(g_debug)
                        printf("  NOOP\n");
                }
            }

            sb->sections[i].insts = augment_array(sb->sections[i].insts, sizeof(struct sb_inst_t),
                sb->sections[i].nr_insts, aug_insts, nr_aug_insts);
            sb->sections[i].nr_insts += nr_aug_insts;

            /* augment image and section size */
            sb->image_size += missing_sz;
            sec->sec_size += missing_sz;
        }
    }
    /* final signature */
    sb->image_size += 2;
}

static uint64_t generate_timestamp()
{
    struct tm tm_base = {0, 0, 0, 1, 0, 100, 0, 0, 1, 0, NULL}; /* 2000/1/1 0:00:00 */
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
    sb_hdr->minor_ver = IMAGE_MINOR_VERSION;
    sb_hdr->flags = 0;
    sb_hdr->image_size = sb->image_size;
    sb_hdr->header_size = sizeof(struct sb_header_t) / BLOCK_SIZE;
    sb_hdr->first_boot_sec_id = sb->sections[0].identifier;
    sb_hdr->nr_keys = g_nr_keys;
    sb_hdr->nr_sections = sb->nr_sections;
    sb_hdr->sec_hdr_size = sizeof(struct sb_section_header_t) / BLOCK_SIZE;
    sb_hdr->key_dict_off = sb_hdr->header_size +
        sb_hdr->sec_hdr_size * sb_hdr->nr_sections;
    sb_hdr->first_boot_tag_off = sb_hdr->key_dict_off +
        sizeof(struct sb_key_dictionary_entry_t) * sb_hdr->nr_keys / BLOCK_SIZE;
    generate_random_data(sb_hdr->rand_pad0, sizeof(sb_hdr->rand_pad0));
    generate_random_data(sb_hdr->rand_pad1, sizeof(sb_hdr->rand_pad1));
    sb_hdr->timestamp = generate_timestamp();
    sb_hdr->product_ver = sb->product_ver;
    fix_version(&sb_hdr->product_ver);
    sb_hdr->component_ver = sb->component_ver;
    fix_version(&sb_hdr->component_ver);
    sb_hdr->drive_tag = 0;

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
        | (sec->is_cleartext ? SECTION_CLEARTEXT : 0);
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
    tag->len = sec->sec_size;
    tag->flags = (sec->is_data ? 0 : SECTION_BOOTABLE)
        | (sec->is_cleartext ? SECTION_CLEARTEXT : 0);
    tag->hdr.checksum = instruction_checksum(&tag->hdr);
}

void produce_sb_instruction(struct sb_inst_t *inst,
    struct sb_instruction_common_t *cmd)
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
            bug("die\n");
    }
    cmd->hdr.checksum = instruction_checksum(&cmd->hdr);
}

void sb_produce_file(struct sb_file_t *sb, const char *filename)
{
    FILE *fd = fopen(filename, "wb");
    if(fd == NULL)
        bugp("cannot open output file");

    struct crypto_key_t real_key;
    real_key.method = CRYPTO_KEY;
    byte crypto_iv[16];
    byte (*cbc_macs)[16] = xmalloc(16 * g_nr_keys);
    /* init CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
        memset(cbc_macs[i], 0, 16);

    fill_gaps(sb);
    compute_sb_offsets(sb);

    generate_random_data(real_key.u.key, 16);

    /* global SHA-1 */
    struct sha_1_params_t file_sha1;
    sha_1_init(&file_sha1);
    /* produce and write header */
    struct sb_header_t sb_hdr;
    produce_sb_header(sb, &sb_hdr);
    sha_1_update(&file_sha1, (byte *)&sb_hdr, sizeof(sb_hdr));
    fwrite(&sb_hdr, 1, sizeof(sb_hdr), fd);
    
    memcpy(crypto_iv, &sb_hdr, 16);

    /* update CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
        crypto_cbc((byte *)&sb_hdr, NULL, sizeof(sb_hdr) / BLOCK_SIZE, &g_key_array[i],
            cbc_macs[i], &cbc_macs[i], 1);
    
    /* produce and write section headers */
    for(int i = 0; i < sb_hdr.nr_sections; i++)
    {
        struct sb_section_header_t sb_sec_hdr;
        produce_sb_section_header(&sb->sections[i], &sb_sec_hdr);
        sha_1_update(&file_sha1, (byte *)&sb_sec_hdr, sizeof(sb_sec_hdr));
        fwrite(&sb_sec_hdr, 1, sizeof(sb_sec_hdr), fd);
        /* update CBC-MACs */
        for(int j = 0; j < g_nr_keys; j++)
            crypto_cbc((byte *)&sb_sec_hdr, NULL, sizeof(sb_sec_hdr) / BLOCK_SIZE,
                &g_key_array[j], cbc_macs[j], &cbc_macs[j], 1);
    }
    /* produce key dictionary */
    for(int i = 0; i < g_nr_keys; i++)
    {
        struct sb_key_dictionary_entry_t entry;
        memcpy(entry.hdr_cbc_mac, cbc_macs[i], 16);
        crypto_cbc(real_key.u.key, entry.key, 1, &g_key_array[i],
            crypto_iv, NULL, 1);
        
        fwrite(&entry, 1, sizeof(entry), fd);
        sha_1_update(&file_sha1, (byte *)&entry, sizeof(entry));
    }

    /* HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK */
    /* Image crafting, don't use it unless you understand what you do */
    if(sb->real_key != NULL)
        memcpy(real_key.u.key, *sb->real_key, 16);
    if(sb->crypto_iv != NULL)
        memcpy(crypto_iv, *sb->crypto_iv, 16);
    /* KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH KCAH */
    if(g_debug)
    {
        printf("Real key: ");
        for(int j = 0; j < 16; j++)
            printf("%02x", real_key.u.key[j]);
        printf("\n");
        printf("IV      : ");
        for(int j = 0; j < 16; j++)
            printf("%02x", crypto_iv[j]);
        printf("\n");
    }
    /* produce sections data */
    for(int i = 0; i< sb_hdr.nr_sections; i++)
    {
        /* produce tag command */
        struct sb_instruction_tag_t tag_cmd;
        produce_section_tag_cmd(&sb->sections[i], &tag_cmd, (i + 1) == sb_hdr.nr_sections);
        if(g_nr_keys > 0)
            crypto_cbc((byte *)&tag_cmd, (byte *)&tag_cmd, sizeof(tag_cmd) / BLOCK_SIZE,
                &real_key, crypto_iv, NULL, 1);
        sha_1_update(&file_sha1, (byte *)&tag_cmd, sizeof(tag_cmd));
        fwrite(&tag_cmd, 1, sizeof(tag_cmd), fd);
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
                produce_sb_instruction(inst, &cmd);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                    crypto_cbc((byte *)&cmd, (byte *)&cmd, sizeof(cmd) / BLOCK_SIZE,
                        &real_key, cur_cbc_mac, &cur_cbc_mac, 1);
                sha_1_update(&file_sha1, (byte *)&cmd, sizeof(cmd));
                fwrite(&cmd, 1, sizeof(cmd), fd);
            }
            /* data */
            if(inst->inst == SB_INST_LOAD || inst->inst == SB_INST_DATA)
            {
                uint32_t sz = inst->size + inst->padding_size;
                byte *data = xmalloc(sz);
                memcpy(data, inst->data, inst->size);
                memcpy(data + inst->size, inst->padding, inst->padding_size);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                    crypto_cbc(data, data, sz / BLOCK_SIZE,
                        &real_key, cur_cbc_mac, &cur_cbc_mac, 1);
                sha_1_update(&file_sha1, data, sz);
                fwrite(data, 1, sz, fd);
                free(data);
            }
        }
    }
    /* write file SHA-1 */
    byte final_sig[32];
    sha_1_finish(&file_sha1);
    sha_1_output(&file_sha1, final_sig);
    generate_random_data(final_sig + 20, 12);
    if(g_nr_keys > 0)
        crypto_cbc(final_sig, final_sig, 2, &real_key, crypto_iv, NULL, 1);
    fwrite(final_sig, 1, 32, fd);
    
    fclose(fd);
}
