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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
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

#define _STR(a) #a
#define STR(a) _STR(a)

#define bug(...) do { fprintf(stderr,"["__FILE__":"STR(__LINE__)"]ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

bool g_debug = false;
char **g_extern;
int g_extern_count;

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

/**
 * Misc
 */

char *s_getenv(const char *name)
{
    char *s = getenv(name);
    return s ? s : "";
}

void generate_random_data(void *buf, size_t sz)
{
    static int rand_fd = -1;
    if(rand_fd == -1)
        rand_fd = open("/dev/urandom", O_RDONLY);
    if(rand_fd == -1)
        bugp("failed to open /dev/urandom");
    if(read(rand_fd, buf, sz) != (ssize_t)sz)
        bugp("failed to read /dev/urandom");
}

void *xmalloc(size_t s) /* malloc helper, used in elf.c */
{
    void * r = malloc(s);
    if(!r) bugp("malloc");
    return r;
}

int convxdigit(char digit, byte *val)
{
    if(digit >= '0' && digit <= '9')
    {
        *val = digit - '0';
        return 0;
    }
    else if(digit >= 'A' && digit <= 'F')
    {
        *val = digit - 'A' + 10;
        return 0;
    }
    else if(digit >= 'a' && digit <= 'f')
    {
        *val = digit - 'a' + 10;
        return 0;
    }
    else
        return 1;
}

/**
 * Key file parsing
 */

typedef byte (*key_array_t)[16];

int g_nr_keys;
key_array_t g_key_array;

static void add_keys(key_array_t ka, int kac)
{
    key_array_t new_ka = xmalloc((g_nr_keys + kac) * 16);
    memcpy(new_ka, g_key_array, g_nr_keys * 16);
    memcpy(new_ka + g_nr_keys, ka, kac * 16);
    free(g_key_array);
    g_key_array = new_ka;
    g_nr_keys += kac;
}

static key_array_t read_keys(const char *key_file, int *num_keys)
{
    int size;
    struct stat st;
    int fd = open(key_file,O_RDONLY);
    if(fd == -1)
        bugp("opening key file failed");
    if(fstat(fd,&st) == -1)
        bugp("key file stat() failed");
    size = st.st_size;
    char *buf = xmalloc(size);
    if(read(fd, buf, size) != (ssize_t)size)
        bugp("reading key file");
    close(fd);

    if(g_debug)
        printf("Parsing key file '%s'...\n", key_file);
    *num_keys = size ? 1 : 0;
    char *ptr = buf;
    /* allow trailing newline at the end (but no space after it) */
    while(ptr != buf + size && (ptr + 1) != buf + size)
    {
        if(*ptr++ == '\n')
            (*num_keys)++;
    }

    key_array_t keys = xmalloc(sizeof(byte[16]) * *num_keys);
    int pos = 0;
    for(int i = 0; i < *num_keys; i++)
    {
        /* skip ws */
        while(pos < size && isspace(buf[pos]))
            pos++;
        /* enough space ? */
        if((pos + 32) > size)
            bugp("invalid key file");
        for(int j = 0; j < 16; j++)
        {
            byte a, b;
            if(convxdigit(buf[pos + 2 * j], &a) || convxdigit(buf[pos + 2 * j + 1], &b))
                bugp(" invalid key, it should be a 128-bit key written in hexadecimal\n");
            keys[i][j] = (a << 4) | b;
        }
        if(g_debug)
        {
            printf("Add key: ");
            for(int j = 0; j < 16; j++)
                printf("%02x", keys[i][j]);
               printf("\n");
        }
        pos += 32;
    }
    free(buf);

    return keys;
}

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
    if(lseek(*(int *)user, addr, SEEK_SET) == (off_t)-1)
        return false;
    return read(*(int *)user, buf, count) == (ssize_t)count;
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
    int fd = open(src->filename, O_RDONLY);
    if(fd < 0)
        bug("cannot open '%s' (id '%s')\n", src->filename, id);
    if(g_debug)
        printf("Loading ELF file '%s'...\n", src->filename);
    elf_init(&src->elf);
    src->loaded = elf_read_file(&src->elf, elf_read, elf_printf, &fd);
    close(fd);
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
    int fd = open(src->filename, O_RDONLY);
    if(fd < 0)
        bug("cannot open '%s' (id '%s')\n", src->filename, id);
    if(g_debug)
        printf("Loading BIN file '%s'...\n", src->filename);
    src->bin.size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    src->bin.data = xmalloc(src->bin.size);
    read(fd, src->bin.data, src->bin.size);
    close(fd);
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
            struct cmd_option_t *opt = db_find_option_by_id(csec->opt_list, "cleartext");
            if(opt != NULL)
            {
                if(opt->is_string)
                    bug("Cleartext section attribute must be an integer\n");
                if(opt->val != 0 && opt->val != 1)
                    bug("Cleartext section attribute must be 0 or 1\n");
                sec->is_cleartext = opt->val;
            }
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
        default:
            bug("die\n");
    }
    cmd->hdr.checksum = instruction_checksum(&cmd->hdr);
}

static void produce_sb_file(struct sb_file_t *sb, const char *filename)
{
    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd < 0)
        bugp("cannot open output file");

    byte real_key[16];
    byte (*cbc_macs)[16] = xmalloc(16 * g_nr_keys);
    /* init CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
        memset(cbc_macs[i], 0, 16);

    fill_gaps(sb);
    compute_sb_offsets(sb);

    generate_random_data(real_key, sizeof(real_key));

    /* global SHA-1 */
    struct sha_1_params_t file_sha1;
    sha_1_init(&file_sha1);
    /* produce and write header */
    struct sb_header_t sb_hdr;
    produce_sb_header(sb, &sb_hdr);
    sha_1_update(&file_sha1, (byte *)&sb_hdr, sizeof(sb_hdr));
    write(fd, &sb_hdr, sizeof(sb_hdr));
    /* update CBC-MACs */
    for(int i = 0; i < g_nr_keys; i++)
        cbc_mac((byte *)&sb_hdr, NULL, sizeof(sb_hdr) / BLOCK_SIZE, g_key_array[i],
            cbc_macs[i], &cbc_macs[i], 1);
    
    /* produce and write section headers */
    for(int i = 0; i < sb_hdr.nr_sections; i++)
    {
        struct sb_section_header_t sb_sec_hdr;
        produce_sb_section_header(&sb->sections[i], &sb_sec_hdr);
        sha_1_update(&file_sha1, (byte *)&sb_sec_hdr, sizeof(sb_sec_hdr));
        write(fd, &sb_sec_hdr, sizeof(sb_sec_hdr));
        /* update CBC-MACs */
        for(int j = 0; j < g_nr_keys; j++)
            cbc_mac((byte *)&sb_sec_hdr, NULL, sizeof(sb_sec_hdr) / BLOCK_SIZE,
                g_key_array[j], cbc_macs[j], &cbc_macs[j], 1);
    }
    /* produce key dictionary */
    for(int i = 0; i < g_nr_keys; i++)
    {
        struct sb_key_dictionary_entry_t entry;
        memcpy(entry.hdr_cbc_mac, cbc_macs[i], 16);
        cbc_mac(real_key, entry.key, sizeof(real_key) / BLOCK_SIZE, g_key_array[i],
            (byte *)&sb_hdr, NULL, 1);
        
        write(fd, &entry, sizeof(entry));
        sha_1_update(&file_sha1, (byte *)&entry, sizeof(entry));
    }
    /* produce sections data */
    for(int i = 0; i< sb_hdr.nr_sections; i++)
    {
        /* produce tag command */
        struct sb_instruction_tag_t tag_cmd;
        produce_section_tag_cmd(&sb->sections[i], &tag_cmd, (i + 1) == sb_hdr.nr_sections);
        if(g_nr_keys > 0)
            cbc_mac((byte *)&tag_cmd, (byte *)&tag_cmd, sizeof(tag_cmd) / BLOCK_SIZE,
                real_key, (byte *)&sb_hdr, NULL, 1);
        sha_1_update(&file_sha1, (byte *)&tag_cmd, sizeof(tag_cmd));
        write(fd, &tag_cmd, sizeof(tag_cmd));
        /* produce other commands */
        byte cur_cbc_mac[16];
        memcpy(cur_cbc_mac, (byte *)&sb_hdr, 16);
        for(int j = 0; j < sb->sections[i].nr_insts; j++)
        {
            struct sb_inst_t *inst = &sb->sections[i].insts[j];
            /* command */
            if(inst->inst != SB_INST_DATA)
            {
                struct sb_instruction_common_t cmd;
                produce_sb_instruction(inst, &cmd);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                    cbc_mac((byte *)&cmd, (byte *)&cmd, sizeof(cmd) / BLOCK_SIZE,
                        real_key, cur_cbc_mac, &cur_cbc_mac, 1);
                sha_1_update(&file_sha1, (byte *)&cmd, sizeof(cmd));
                write(fd, &cmd, sizeof(cmd));
            }
            /* data */
            if(inst->inst == SB_INST_LOAD || inst->inst == SB_INST_DATA)
            {
                uint32_t sz = inst->size + inst->padding_size;
                byte *data = xmalloc(sz);
                memcpy(data, inst->data, inst->size);
                memcpy(data + inst->size, inst->padding, inst->padding_size);
                if(g_nr_keys > 0 && !sb->sections[i].is_cleartext)
                    cbc_mac(data, data, sz / BLOCK_SIZE,
                        real_key, cur_cbc_mac, &cur_cbc_mac, 1);
                sha_1_update(&file_sha1, data, sz);
                write(fd, data, sz);
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
        cbc_mac(final_sig, final_sig, 2, real_key, (byte *)&sb_hdr, NULL, 1);
    write(fd, final_sig, 32);
    
    close(fd);
}

void usage(void)
{
    printf("Usage: elftosb [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help:\t\tDisplay this message\n");
    printf("  -o <file>\tSet output file\n");
    printf("  -c <file>\tSet command file\n");
    printf("  -d/--debug\tEnable debug output\n");
    printf("  -k <file>\t\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    exit(1);
}

static byte g_zero_key[16] = {0};

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
            for(int j = 0; j < 16; j++)
                printf(" %02x", g_key_array[i][j]);
            printf("\n");
        }

        for(int i = 0; i < g_extern_count; i++)
            printf("extern(%d)=%s\n", i, g_extern[i]);
    }

    struct cmd_file_t *cmd_file = db_parse_file(cmd_filename);
    struct sb_file_t *sb_file = apply_cmd_file(cmd_file);
    produce_sb_file(sb_file, output_filename);
    
    return 0;
}
