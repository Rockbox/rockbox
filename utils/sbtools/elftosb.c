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
 
#define _ISOC99_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <getopt.h>

#include "crypto.h"
#include "elf.h"
#include "sb.h"
#include "dbparser.h"
#include "misc.h"

char **g_extern;
int g_extern_count;

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

#define crypto_cbc(...) \
    do { int ret = crypto_cbc(__VA_ARGS__); \
        if(ret != CRYPTO_ERROR_SUCCESS) \
            bug("crypto_cbc error: %d\n", ret); \
    }while(0)

/**
 * command file to sb conversion
 */

#define SB_INST_DATA    0xff

struct sb_inst_t
{
    uint8_t inst; /* SB_INST_* */
    uint32_t size;
    // <union>
    void *data;
    uint32_t pattern;
    uint32_t addr;
    // </union>
    uint32_t argument; // for call and jump
    /* for production use */
    uint32_t padding_size;
    uint8_t *padding;
};

struct sb_section_t
{
    uint32_t identifier;
    bool is_data;
    bool is_cleartext;
    uint32_t alignment;
    // data sections are handled as a single SB_INST_DATA virtual instruction 
    int nr_insts;
    struct sb_inst_t *insts;
    /* for production use */
    uint32_t file_offset; /* in blocks */
    uint32_t sec_size; /* in blocks */
};

struct sb_file_t
{
    int nr_sections;
    struct sb_section_t *sections;
    struct sb_version_t product_ver;
    struct sb_version_t component_ver;
    /* for production use */
    uint32_t image_size; /* in blocks */
};

static bool elf_read(void *user, uint32_t addr, void *buf, size_t count)
{
    if(fseek((FILE *)user, addr, SEEK_SET) == -1)
        return false;
    return fread(buf, 1, count, (FILE *)user) == count;
}

static void elf_printf(void *user, bool error, const char *fmt, ...)
{
    if(!g_debug && !error)
        return;
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

static void resolve_extern(struct cmd_source_t *src)
{
    if(!src->is_extern)
        return;
    src->is_extern = false;
    if(src->extern_nr < 0 || src->extern_nr >= g_extern_count)
        bug("There aren't enough file on command file to resolve extern(%d)\n", src->extern_nr);
    src->filename = g_extern[src->extern_nr];
}

static void load_elf_by_id(struct cmd_file_t *cmd_file, const char *id)
{
    struct cmd_source_t *src = db_find_source_by_id(cmd_file, id);
    if(src == NULL)
        bug("undefined reference to source '%s'\n", id);
    /* avoid reloading */
    if(src->type == CMD_SRC_ELF && src->loaded)
        return;
    if(src->type != CMD_SRC_UNK)
        bug("source '%s' seen both as elf and binary file\n", id);
    /* resolve potential extern file */
    resolve_extern(src);
    /* load it */
    src->type = CMD_SRC_ELF;
    FILE *fd = fopen(src->filename, "rb");
    if(fd == NULL)
        bug("cannot open '%s' (id '%s')\n", src->filename, id);
    if(g_debug)
        printf("Loading ELF file '%s'...\n", src->filename);
    elf_init(&src->elf);
    src->loaded = elf_read_file(&src->elf, elf_read, elf_printf, fd);
    fclose(fd);
    if(!src->loaded)
        bug("error loading elf file '%s' (id '%s')\n", src->filename, id);
    elf_translate_addresses(&src->elf);
}

static void load_bin_by_id(struct cmd_file_t *cmd_file, const char *id)
{
    struct cmd_source_t *src = db_find_source_by_id(cmd_file, id);
    if(src == NULL)
        bug("undefined reference to source '%s'\n", id);
    /* avoid reloading */
    if(src->type == CMD_SRC_BIN && src->loaded)
        return;
    if(src->type != CMD_SRC_UNK)
        bug("source '%s' seen both as elf and binary file\n", id);
    /* resolve potential extern file */
    resolve_extern(src);
    /* load it */
    src->type = CMD_SRC_BIN;
    FILE *fd = fopen(src->filename, "rb");
    if(fd == NULL)
        bug("cannot open '%s' (id '%s')\n", src->filename, id);
    if(g_debug)
        printf("Loading BIN file '%s'...\n", src->filename);
    fseek(fd, 0, SEEK_END);
    src->bin.size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    src->bin.data = xmalloc(src->bin.size);
    fread(src->bin.data, 1, src->bin.size, fd);
    fclose(fd);
    src->loaded = true;
}

static struct sb_file_t *apply_cmd_file(struct cmd_file_t *cmd_file)
{
    struct sb_file_t *sb = xmalloc(sizeof(struct sb_file_t));
    memset(sb, 0, sizeof(struct sb_file_t));

    db_generate_default_sb_version(&sb->product_ver);
    db_generate_default_sb_version(&sb->component_ver);
    
    if(g_debug)
        printf("Applying command file...\n");
    /* count sections */
    struct cmd_section_t *csec = cmd_file->section_list;
    while(csec)
    {
        sb->nr_sections++;
        csec = csec->next;
    }

    sb->sections = xmalloc(sb->nr_sections * sizeof(struct sb_section_t));
    memset(sb->sections, 0, sb->nr_sections * sizeof(struct sb_section_t));
    /* flatten sections */
    csec = cmd_file->section_list;
    for(int i = 0; i < sb->nr_sections; i++, csec = csec->next)
    {
        struct sb_section_t *sec = &sb->sections[i];
        sec->identifier = csec->identifier;

        /* options */
        do
        {
            /* cleartext */
            struct cmd_option_t *opt = db_find_option_by_id(csec->opt_list, "cleartext");
            if(opt != NULL)
            {
                if(opt->is_string)
                    bug("Cleartext section attribute must be an integer\n");
                if(opt->val != 0 && opt->val != 1)
                    bug("Cleartext section attribute must be 0 or 1\n");
                sec->is_cleartext = opt->val;
            }
            /* alignment */
            opt = db_find_option_by_id(csec->opt_list, "alignment");
            if(opt != NULL)
            {
                if(opt->is_string)
                    bug("Cleartext section attribute must be an integer\n");
                // n is a power of 2 iff n & (n - 1) = 0
                // alignement cannot be lower than block size
                if((opt->val & (opt->val - 1)) != 0)
                    bug("Cleartext section attribute must be a power of two\n");
                if(opt->val < BLOCK_SIZE)
                    sec->alignment = BLOCK_SIZE;
                else
                    sec->alignment = opt->val;
            }
            else
                sec->alignment = BLOCK_SIZE;
        }while(0);

        if(csec->is_data)
        {
            sec->is_data = true;
            sec->nr_insts = 1;
            sec->insts = xmalloc(sec->nr_insts * sizeof(struct sb_inst_t));
            memset(sec->insts, 0, sec->nr_insts * sizeof(struct sb_inst_t));

            load_bin_by_id(cmd_file, csec->source_id);
            struct bin_param_t *bin = &db_find_source_by_id(cmd_file, csec->source_id)->bin;

            sec->insts[0].inst = SB_INST_DATA;
            sec->insts[0].size = bin->size;
            sec->insts[0].data = bin->data;
        }
        else
        {
            sec->is_data = false;
            /* count instructions and loads things */
            struct cmd_inst_t *cinst = csec->inst_list;
            while(cinst)
            {
                if(cinst->type == CMD_LOAD)
                {
                    load_elf_by_id(cmd_file, cinst->identifier);
                    struct elf_params_t *elf = &db_find_source_by_id(cmd_file, cinst->identifier)->elf;
                    sec->nr_insts += elf_get_nr_sections(elf);
                }
                else if(cinst->type == CMD_JUMP || cinst->type == CMD_CALL)
                {
                    load_elf_by_id(cmd_file, cinst->identifier);
                    struct elf_params_t *elf = &db_find_source_by_id(cmd_file, cinst->identifier)->elf;
                    if(!elf_get_start_addr(elf, NULL))
                        bug("cannot jump/call '%s' because it has no starting point !\n", cinst->identifier);
                    sec->nr_insts++;
                }
                else if(cinst->type == CMD_CALL_AT || cinst->type == CMD_JUMP_AT)
                {
                    sec->nr_insts++;
                }
                else if(cinst->type == CMD_LOAD_AT)
                {
                    load_bin_by_id(cmd_file, cinst->identifier);
                    sec->nr_insts++;
                }
                else if(cinst->type == CMD_MODE)
                {
                    sec->nr_insts++;
                }
                else
                    bug("die\n");
                
                cinst = cinst->next;
            }

            sec->insts = xmalloc(sec->nr_insts * sizeof(struct sb_inst_t));
            memset(sec->insts, 0, sec->nr_insts * sizeof(struct sb_inst_t));
            /* flatten */
            int idx = 0;
            cinst = csec->inst_list;
            while(cinst)
            {
                if(cinst->type == CMD_LOAD)
                {
                    struct elf_params_t *elf = &db_find_source_by_id(cmd_file, cinst->identifier)->elf;
                    struct elf_section_t *esec = elf->first_section;
                    while(esec)
                    {
                        if(esec->type == EST_LOAD)
                        {
                            sec->insts[idx].inst = SB_INST_LOAD;
                            sec->insts[idx].addr = esec->addr;
                            sec->insts[idx].size = esec->size;
                            sec->insts[idx++].data = esec->section;
                        }
                        else if(esec->type == EST_FILL)
                        {
                            sec->insts[idx].inst = SB_INST_FILL;
                            sec->insts[idx].addr = esec->addr;
                            sec->insts[idx].size = esec->size;
                            sec->insts[idx++].pattern = esec->pattern;
                        }
                        esec = esec->next;
                    }
                }
                else if(cinst->type == CMD_JUMP || cinst->type == CMD_CALL)
                {
                    struct elf_params_t *elf = &db_find_source_by_id(cmd_file, cinst->identifier)->elf;
                    sec->insts[idx].argument = cinst->argument;
                    sec->insts[idx].inst = (cinst->type == CMD_JUMP) ? SB_INST_JUMP : SB_INST_CALL;
                    sec->insts[idx++].addr = elf->start_addr;
                }
                else if(cinst->type == CMD_JUMP_AT || cinst->type == CMD_CALL_AT)
                {
                    sec->insts[idx].argument = cinst->argument;
                    sec->insts[idx].inst = (cinst->type == CMD_JUMP_AT) ? SB_INST_JUMP : SB_INST_CALL;
                    sec->insts[idx++].addr = cinst->addr;
                }
                else if(cinst->type == CMD_LOAD_AT)
                {
                    struct bin_param_t *bin = &db_find_source_by_id(cmd_file, cinst->identifier)->bin;
                    sec->insts[idx].inst = SB_INST_LOAD;
                    sec->insts[idx].addr = cinst->addr;
                    sec->insts[idx].data = bin->data;
                    sec->insts[idx++].size = bin->size;
                }
                else if(cinst->type == CMD_MODE)
                {
                    sec->insts[idx].inst = SB_INST_MODE;
                    sec->insts[idx++].addr = cinst->argument;
                }
                else
                    bug("die\n");
                
                cinst = cinst->next;
            }
        }
    }

    return sb;
}

/**
 * SB file production
 */

/* helper function to augment an array, free old array */
void *augment_array(void *arr, size_t elem_sz, size_t cnt, void *aug, size_t aug_cnt)
{
    void *p = xmalloc(elem_sz * (cnt + aug_cnt));
    memcpy(p, arr, elem_sz * cnt);
    memcpy(p + elem_sz * cnt, aug, elem_sz * aug_cnt);
    free(arr);
    return p;
}

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

static void produce_sb_file(struct sb_file_t *sb, const char *filename)
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
    if(strlen(s_getenv("SB_OVERRIDE_REAL_KEY")) != 0)
    {
        const char *key = s_getenv("SB_OVERRIDE_REAL_KEY");
        if(strlen(key) != 32)
            bugp("Cannot override real key: invalid key length\n");
        for(int i = 0; i < 16; i++)
        {
            byte a, b;
            if(convxdigit(key[2 * i], &a) || convxdigit(key[2 * i + 1], &b))
            bugp("Cannot override real key: key should be a 128-bit key written in hexadecimal\n");
            real_key.u.key[i] = (a << 4) | b;
        }
    }
    if(strlen(s_getenv("SB_OVERRIDE_IV")) != 0)
    {
        const char *iv = s_getenv("SB_OVERRIDE_IV");
        if(strlen(iv) != 32)
            bugp("Cannot override iv: invalid key length\n");
        for(int i = 0; i < 16; i++)
        {
            byte a, b;
            if(convxdigit(iv[2 * i], &a) || convxdigit(iv[2 * i + 1], &b))
            bugp("Cannot override iv: key should be a 128-bit key written in hexadecimal\n");
            crypto_iv[i] = (a << 4) | b;
        }
        
    }
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

void usage(void)
{
    printf("Usage: elftosb [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <file>\tSet output file\n");
    printf("  -c <file>\tSet command file\n");
    printf("  -d/--debug\tEnable debug output\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  --single-key <key>\tAdd single key\n");
    printf("  --usb-otp <vid>:<pid>\tAdd USB OTP device\n");
    exit(1);
}

static struct crypto_key_t g_zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

int main(int argc, char **argv)
{
    char *cmd_filename = NULL;
    char *output_filename = NULL;
    
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"single-key", required_argument, 0, 's'},
            {"usb-otp", required_argument, 0, 'u'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:c:k:z", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                output_filename = optarg;
                break;
            case 'c':
                cmd_filename = optarg;
                break;
            case 'k':
            {
                int kac;
                key_array_t ka = read_keys(optarg, &kac);
                add_keys(ka, kac);
                break;
            }
            case 'z':
            {
                add_keys(&g_zero_key, 1);
                break;
            }
            case 's':
            {
                struct crypto_key_t key;
                key.method = CRYPTO_KEY;
                if(strlen(optarg) != 32)
                    bug("The key given in argument is invalid");
                for(int i = 0; i < 16; i++)
                {
                    byte a, b;
                    if(convxdigit(optarg[2 * i], &a) || convxdigit(optarg[2 * i + 1], &b))
                        bugp("The key given in argument is invalid\n");
                    key.u.key[i] = (a << 4) | b;
                }
                add_keys(&key, 1);
                break;
            }
            case 'u':
            {
                int vid, pid;
                char *p = strchr(optarg, ':');
                if(p == NULL)
                    bug("Invalid VID/PID\n");

                char *end;
                vid = strtol(optarg, &end, 16);
                if(end != p)
                    bug("Invalid VID/PID\n");
                pid = strtol(p + 1, &end, 16);
                if(end != (optarg + strlen(optarg)))
                    bug("Invalid VID/PID\n");
                struct crypto_key_t key;
                key.method = CRYPTO_USBOTP;
                key.u.vid_pid = vid << 16 | pid;
                add_keys(&key, 1);
                break;
            }
            default:
                abort();
        }
    }

    if(!cmd_filename)
        bug("You must specify a command file\n");
    if(!output_filename)
        bug("You must specify an output file\n");

    g_extern = &argv[optind];
    g_extern_count = argc - optind;

    if(g_debug)
    {
        printf("key: %d\n", g_nr_keys);
        for(int i = 0; i < g_nr_keys; i++)
        {
            printf("  ");
            print_key(&g_key_array[i], true);
        }

        for(int i = 0; i < g_extern_count; i++)
            printf("extern(%d)=%s\n", i, g_extern[i]);
    }

    struct cmd_file_t *cmd_file = db_parse_file(cmd_filename);
    struct sb_file_t *sb_file = apply_cmd_file(cmd_file);
    produce_sb_file(sb_file, output_filename);
    
    return 0;
}
