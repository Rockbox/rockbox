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

#include "crypto.h"
#include "elf.h"
#include "sb.h"
#include "dbparser.h"
#include "misc.h"
#include "sb.h"

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
    /* first free the old src->filename content */
    free(src->filename);
    src->filename = strdup(g_extern[src->extern_nr]);
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
            sec->insts[0].data = memdup(bin->data, bin->size);
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
                            sec->insts[idx++].data = memdup(esec->section, esec->size);
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
                    sec->insts[idx].data = memdup(bin->data, bin->size);
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

static void usage(void)
{
    printf("Usage: elftosb [options | file]...\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <file>\tSet output file\n");
    printf("  -c <file>\tSet command file\n");
    printf("  -d/--debug\tEnable debug output\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  --add-key <key>\tAdd single key (hex or usbotp)\n");
    printf("  --real-key <key>\tOverride real key\n");
    printf("  --crypto-iv <iv>\tOverride crypto IV\n");
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
    struct crypto_key_t real_key;
    struct crypto_key_t crypto_iv;
    real_key.method = CRYPTO_NONE;
    crypto_iv.method = CRYPTO_NONE;
    
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {"real-key", required_argument, 0, 'r'},
            {"crypto-iv", required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:c:k:za:", long_options, NULL);
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
                if(!add_keys_from_file(optarg))
                    bug("Cannot keys from %s\n", optarg);
                break;
            }
            case 'z':
            {
                add_keys(&g_zero_key, 1);
                break;
            }
            case 'a':
            case 'r':
            case 'i':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key/iv specified as argument");
                if(*s != 0)
                    bug("Trailing characters after key/iv specified as argument");
                if(c == 'r')
                    memcpy(&real_key, &key, sizeof(key));
                else if(c == 'i')
                    memcpy(&crypto_iv, &key, sizeof(key));
                else
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
    db_free(cmd_file);

    if(real_key.method == CRYPTO_KEY)
    {
        sb_file->override_real_key = true;
        memcpy(sb_file->real_key, real_key.u.key, 16);
    }
    if(crypto_iv.method == CRYPTO_KEY)
    {
        sb_file->override_crypto_iv = true;
        memcpy(sb_file->crypto_iv, crypto_iv.u.key, 16);
    }

    /* fill with default parameters since there is no command file support for them */
    sb_file->drive_tag = 0;
    sb_file->first_boot_sec_id = sb_file->sections[0].identifier;
    sb_file->flags = 0;
    sb_file->minor_version = 1;
    
    sb_write_file(sb_file, output_filename);
    sb_free(sb_file);
    clear_keys();
    
    return 0;
}
