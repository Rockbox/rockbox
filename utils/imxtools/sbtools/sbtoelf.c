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
#include "sb1.h"
#include "misc.h"
#include "dbparser.h"

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* globals */

static char *g_out_prefix;
static bool g_elf_simplify = true;

static void extract_elf_section(struct elf_params_t *elf, int count, uint32_t id,
    struct cmd_file_t *cmd_file, struct cmd_section_t *section, bool is_call, uint32_t arg)
{
    char name[5];
    char fileid[16];
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sb_fill_section_name(name, id);
    sb_fill_section_name(fileid, id);
    sprintf(fileid + strlen(fileid), "%d", count);
    sprintf(filename, "%s%s.%d.elf", g_out_prefix, name, count);
    db_add_source(cmd_file, fileid, filename + strlen(g_out_prefix));
    db_add_inst_id(section, CMD_LOAD, fileid, 0);
    if(elf_get_start_addr(elf, NULL))
        db_add_inst_id(section, is_call ? CMD_CALL : CMD_JUMP, fileid, arg);

    if(g_debug)
        printf("Write boot section %s to %s\n", name, filename);

    FILE *fd = fopen(filename, "wb");
    free(filename);

    if(fd == NULL)
        return;
    if(g_elf_simplify)
        elf_simplify(elf);
    elf_write_file(elf, elf_std_write, generic_std_printf, fd);
    fclose(fd);
}

static void extract_sb_section(struct sb_section_t *sec, struct cmd_file_t *cmd_file)
{
    struct cmd_section_t *db_sec = db_add_section(cmd_file, sec->identifier, sec->is_data);
    db_add_int_opt(&db_sec->opt_list, "alignment", sec->alignment);
    db_add_int_opt(&db_sec->opt_list, "cleartext", sec->is_cleartext);
    db_add_int_opt(&db_sec->opt_list, "sectionFlags", sec->other_flags);

    if(sec->is_data)
    {
        char sec_name[5];
        char *filename = xmalloc(strlen(g_out_prefix) + 32);
        sb_fill_section_name(sec_name, sec->identifier);
        sprintf(filename, "%s%s.bin", g_out_prefix, sec_name);
        db_add_source(cmd_file, sec_name, filename + strlen(g_out_prefix));
        db_sec->source_id = strdup(sec_name);

        FILE *fd = fopen(filename, "wb");
        if(fd == NULL)
            bugp("Cannot open %s for writing\n", filename);
        if(g_debug)
            printf("Write data section %s to %s\n", sec_name, filename);
        free(filename);

        for(int j = 0; j < sec->nr_insts; j++)
        {
            if(sec->insts[j].inst != SB_INST_DATA)
                bug("Internal errror: should be a data section\n");
            fwrite(sec->insts[j].data, sec->insts[j].size, 1, fd);
        }
        fclose(fd);
    }

    int elf_count = 0;
    struct elf_params_t elf;
    elf_init(&elf);

    int bss_idx = 0, text_idx = 0;
    char secname[32];
    for(int i = 0; i < sec->nr_insts; i++)
    {
        struct sb_inst_t *inst = &sec->insts[i];
        switch(inst->inst)
        {
            case SB_INST_LOAD:
                sprintf(secname, ".text%d", text_idx++);
                elf_add_load_section(&elf, inst->addr, inst->size, inst->data, secname);
                break;
            case SB_INST_FILL:
                sprintf(secname, ".bss%d", bss_idx++);
                elf_add_fill_section(&elf, inst->addr, inst->size, inst->pattern, secname);
                break;
            case SB_INST_CALL:
            case SB_INST_JUMP:
                elf_set_start_addr(&elf, inst->addr);
                extract_elf_section(&elf, elf_count++, sec->identifier, cmd_file, db_sec,
                    inst->inst == SB_INST_CALL, inst->argument);
                elf_release(&elf);
                elf_init(&elf);
                bss_idx = text_idx = 0;
                break;
            default:
                /* ignore mode and nop */
                break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf_section(&elf, elf_count, sec->identifier, cmd_file, db_sec, false, 0);
    elf_release(&elf);
}

static void extract_sb_file(struct sb_file_t *file)
{
    char buffer[64];
    struct cmd_file_t *cmd_file = xmalloc(sizeof(struct cmd_file_t));
    memset(cmd_file, 0, sizeof(struct cmd_file_t));
    db_generate_sb_version(&file->product_ver, buffer, sizeof(buffer));
    db_add_str_opt(&cmd_file->opt_list, "productVersion", buffer);
    db_generate_sb_version(&file->component_ver, buffer, sizeof(buffer));
    db_add_str_opt(&cmd_file->opt_list, "componentVersion", buffer);
    db_add_int_opt(&cmd_file->opt_list, "driveTag", file->drive_tag);
    db_add_int_opt(&cmd_file->opt_list, "flags", file->flags);
    db_add_int_opt(&cmd_file->opt_list, "timestampLow", file->timestamp & 0xffffffff);
    db_add_int_opt(&cmd_file->opt_list, "timestampHigh", file->timestamp >> 32);
    db_add_int_opt(&cmd_file->opt_list, "sbMinorVersion", file->minor_version);

    for(int i = 0; i < file->nr_sections; i++)
        extract_sb_section(&file->sections[i], cmd_file);

    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sprintf(filename, "%smake.db", g_out_prefix);
    if(g_debug)
        printf("Write command file to %s\n", filename);
    db_generate_file(cmd_file, filename, NULL, generic_std_printf);
    db_free(cmd_file);
    free(filename);
}

static void extract_elf(struct elf_params_t *elf, int count)
{
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sprintf(filename, "%s%d.elf", g_out_prefix, count);
    if(g_debug)
        printf("Write boot content to %s\n", filename);

    FILE *fd = fopen(filename, "wb");
    free(filename);

    if(fd == NULL)
        return;
    if(g_elf_simplify)
        elf_simplify(elf);
    elf_write_file(elf, elf_std_write, generic_std_printf, fd);
    fclose(fd);
}

static void extract_sb1_file(struct sb1_file_t *file)
{
    int elf_count = 0;
    struct elf_params_t elf;
    elf_init(&elf);

    int bss_idx = 0, text_idx = 0;
    char secname[32];
    for(int i = 0; i < file->nr_insts; i++)
    {
        struct sb1_inst_t *inst = &file->insts[i];
        switch(inst->cmd)
        {
            case SB1_INST_LOAD:
                sprintf(secname, ".text%d", text_idx++);
                elf_add_load_section(&elf, inst->addr, inst->size, inst->data, secname);
                break;
            case SB1_INST_FILL:
                sprintf(secname, ".bss%d", bss_idx++);
                elf_add_fill_section(&elf, inst->addr, inst->size, inst->pattern, secname);
                break;
            case SB1_INST_CALL:
            case SB1_INST_JUMP:
                elf_set_start_addr(&elf, inst->addr);
                extract_elf(&elf, elf_count++);
                elf_release(&elf);
                elf_init(&elf);
                break;
            default:
                /* ignore mode and nop */
                break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf(&elf, elf_count);
    elf_release(&elf);
}

static void usage(void)
{
    printf("Usage: sbtoelf [options] sb-file\n");
    printf("Options:\n");
    printf("  -h/--help             Display this message\n");
    printf("  -o <prefix>           Enable output and set prefix\n");
    printf("  -d/--debug            Enable debug output*\n");
    printf("  -k <file>             Add key file\n");
    printf("  -z                    Add zero key\n");
    printf("  -r                    Use raw command mode\n");
    printf("  -a/--add-key <key>    Add single key\n");
    printf("  -n/--no-color         Disable output colors\n");
    printf("  -l/--loopback <file>  Produce sb file out of extracted description*\n");
    printf("  -f/--force            Force reading even without a key*\n");
    printf("  -1/--v1               Force to read file as a version 1 file\n");
    printf("  -2/--v2               Force to read file as a version 2 file\n");
    printf("  -s/--no-simpl         Prevent elf files from being simplified*\n");
    printf("  -x                    Use default sb1 key\n");
    printf("  -b                    Brute force key\n");
    printf("  --ignore-sha1         Ignore SHA-1 mismatch*\n");
    printf("Options marked with a * are for debug purpose only\n");
    exit(1);
}

int main(int argc, char **argv)
{
    unsigned flags = 0;
    const char *loopback = NULL;
    bool force_sb1 = false;
    bool force_sb2 = false;
    bool brute_force = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {"no-color", no_argument, 0, 'n'},
            {"loopback", required_argument, 0, 'l'},
            {"force", no_argument, 0, 'f'},
            {"v1", no_argument, 0, '1'},
            {"v2", no_argument, 0, '2'},
            {"no-simpl", no_argument, 0, 's'},
            {"ignore-sha1", no_argument,  0, 254},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "hdo:k:zra:nl:f12xsb", long_options, NULL);
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
            case 'h':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'f':
                g_force = true;
                break;
            case 'k':
            {
                if(!add_keys_from_file(optarg))
                    bug("Cannot add keys from %s\n", optarg);
                break;
            }
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
            case 'r':
                flags |= SB_RAW_MODE;
                break;
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
            case '1':
                force_sb1 = true;
                break;
            case '2':
                force_sb2 = true;
                break;
            case 's':
                g_elf_simplify = false;
                break;
            case 'b':
                brute_force = true;
                break;
            case 254:
                flags |= SB_IGNORE_SHA1;
                break;
            default:
                bug("Internal error: unknown option '%c'\n", c);
        }
    }

    if(force_sb1 && force_sb2)
        bug("You cannot force both version 1 and 2\n");

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    const char *sb_filename = argv[optind];

    enum sb_version_guess_t ver = guess_sb_version(sb_filename);
    if(ver == SB_VERSION_ERR)
    {
        printf("Cannot open/read SB file: %m\n");
        return 1;
    }

    if(force_sb2 || ver == SB_VERSION_2)
    {
        enum sb_error_t err;
        struct sb_file_t *file = sb_read_file(sb_filename, flags, NULL, generic_std_printf, &err);
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
            sb_dump(file, NULL, generic_std_printf);
        }
        if(loopback)
            sb_write_file(file, loopback, 0, generic_std_printf);
        sb_free(file);
    }
    else if(force_sb1 || ver == SB_VERSION_1)
    {
        if(brute_force)
        {
            struct crypto_key_t key;
            enum sb1_error_t err;
            if(!sb1_brute_force(sb_filename, NULL, generic_std_printf, &err, &key))
            {
                color(OFF);
                printf("Brute force failed: %d\n", err);
                return 1;
            }
            color(RED);
            printf("Key found:");
            color(YELLOW);
            for(int i = 0; i < 32; i++)
                printf(" %08x", key.u.xor_key[i / 16].k[i % 16]);
            color(OFF);
            printf("\n");
            color(RED);
            printf("Key: ");
            color(YELLOW);
            for(int i = 0; i < 128; i++)
                printf("%02x", key.u.xor_key[i / 64].key[i % 64]);
            color(OFF);
            printf("\n");
            add_keys(&key, 1);
        }

        enum sb1_error_t err;
        struct sb1_file_t *file = sb1_read_file(sb_filename, NULL, generic_std_printf, &err);
        if(file == NULL)
        {
            color(OFF);
            printf("SB read failed: %d\n", err);
            return 1;
        }

        color(OFF);
        if(g_out_prefix)
            extract_sb1_file(file);
        if(g_debug)
        {
            color(GREY);
            printf("[Debug output]\n");
            sb1_dump(file, NULL, generic_std_printf);
        }
        if(loopback)
            sb1_write_file(file, loopback);

        sb1_free(file);
    }
    else
    {
        color(OFF);
        printf("Cannot guess file type, are you sure it's a valid image ?\n");
        return 1;
    }
    clear_keys();

    return 0;
}
