/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
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

/*
 * .sb file parser and chunk extractor
 *
 * Based on amsinfo, which is
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 */

#define _ISOC99_SOURCE /* snprintf() */
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
#include "misc.h"

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

#define crypto_cbc(...) \
    do { int ret = crypto_cbc(__VA_ARGS__); \
        if(ret != CRYPTO_ERROR_SUCCESS) \
            bug("crypto_cbc error: %d\n", ret); \
    }while(0)

/* globals */

char *g_out_prefix;

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

static void elf_write(void *user, uint32_t addr, const void *buf, size_t count)
{
    FILE *f = user;
    fseek(f, addr, SEEK_SET);
    fwrite(buf, count, 1, f);
}

static void extract_elf_section(struct elf_params_t *elf, int count, uint32_t id)
{
    char name[5];
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sb_fill_section_name(name, id);
    sprintf(filename, "%s%s.%d.elf", g_out_prefix, name, count);
    if(g_debug)
        printf("Write boot section %s to %s\n", name, filename);
    
    FILE *fd = fopen(filename, "wb");
    free(filename);
    
    if(fd == NULL)
        return ;
    elf_write_file(elf, elf_write, elf_printf, fd);
    fclose(fd);
}

static void extract_sb_section(struct sb_section_t *sec)
{
    if(sec->is_data)
    {
        char sec_name[5];
        char *filename = xmalloc(strlen(g_out_prefix) + 32);
        sb_fill_section_name(sec_name, sec->identifier);
        sprintf(filename, "%s%s.bin", g_out_prefix, sec_name);
        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
            bugp("Cannot open %s for writing\n", filename);
        if(g_debug)
            printf("Write data section %s to %s\n", sec_name, filename);
        free(filename);
        
        for(int j = 0; j < sec->nr_insts; j++)
        {
            assert(sec->insts[j].inst == SB_INST_DATA);
            fwrite(sec->insts[j].data, sec->insts[j].size, 1, fd);
        }
        fclose(fd);
    }

    int elf_count = 0;
    struct elf_params_t elf;
    elf_init(&elf);

    for(int i = 0; i < sec->nr_insts; i++)
    {
        struct sb_inst_t *inst = &sec->insts[i];
        switch(inst->inst)
        {
            case SB_INST_LOAD:
                elf_add_load_section(&elf, inst->addr, inst->size, inst->data);
                break;
            case SB_INST_FILL:
                elf_add_fill_section(&elf, inst->addr, inst->size, inst->pattern);
                break;
            case SB_INST_CALL:
            case SB_INST_JUMP:
                elf_set_start_addr(&elf, inst->addr);
                extract_elf_section(&elf, elf_count++, sec->identifier);
                elf_release(&elf);
                elf_init(&elf);
                break;
            default:
                /* ignore mode and nop */
                break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf_section(&elf, elf_count, sec->identifier);
    elf_release(&elf);
}

static void extract_sb_file(struct sb_file_t *file)
{
    for(int i = 0; i < file->nr_sections; i++)
        extract_sb_section(&file->sections[i]);
}

static void usage(void)
{
    printf("Usage: sbtoelf [options] sb-file\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <prefix>\tEnable output and set prefix\n");
    printf("  -d/--debug\tEnable debug output*\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  -r\t\tUse raw command mode\n");
    printf("  -a/--add-key <key>\tAdd single key (hex or usbotp)\n");
    printf("  -n/--no-color\tDisable output colors\n");
    printf("  -l/--loopback <file>\tProduce sb file out of extracted description*\n");
    printf("Options marked with a * are for debug purpose only\n");
    exit(1);
}

static void sb_printf(void *user, bool error, color_t c, const char *fmt, ...)
{
    (void) user;
    (void) error;
    va_list args;
    va_start(args, fmt);
    color(c);
    vprintf(fmt, args);
    va_end(args);
}

static struct crypto_key_t g_zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

int main(int argc, char **argv)
{
    bool raw_mode = false;
    const char *loopback = NULL;
    
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {"no-color", no_argument, 0, 'n'},
            {"loopback", required_argument, 0, 'l'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:k:zra:nl:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'l':
                if(loopback)
                    bug("Only one loopback file can be specified !\n");
                loopback = optarg;
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
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'k':
            {
                if(!add_keys_from_file(optarg))
                    bug("Cannot add keys from %s\n", optarg);
                break;
            }
            case 'z':
            {
                add_keys(&g_zero_key, 1);
                break;
            }
            case 'r':
                raw_mode = true;
                break;
            case 'a':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key specified as argument");
                if(*s != 0)
                    bug("Trailing characters after key specified as argument");
                add_keys(&key, 1);
                break;
            }
            default:
                abort();
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    const char *sb_filename = argv[optind];

    enum sb_error_t err;
    struct sb_file_t *file = sb_read_file(sb_filename, raw_mode, NULL, sb_printf, &err);
    if(file == NULL)
    {
        color(OFF);
        printf("SB read failed: %d\n", err);
        return 1;
    }
    
    color(OFF);
    if(g_out_prefix)
        extract_sb_file(file);
    if(g_debug)
    {
        color(GREY);
        printf("[Debug output]\n");
        sb_dump(file, NULL, sb_printf);
    }
    if(loopback)
    {
        /* sb_read_file will fill real key and IV but we don't want to override
         * them when looping back otherwise the output will be inconsistent and
         * garbage */
        file->override_real_key = false;
        file->override_crypto_iv = false;
        sb_write_file(file, loopback);
    }
    sb_free(file);
    clear_keys();
    
    return 0;
}
