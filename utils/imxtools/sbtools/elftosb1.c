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

#define _POSIX_C_SOURCE 200809L /* for strdup */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>

#include "crypto.h"
#include "elf.h"
#include "sb1.h"
#include "misc.h"

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

/**
 * Globals
 */

char *g_output_file;
bool g_critical;
bool g_final;
bool g_strict = true;
uint32_t g_jump_arg;

/**
 * Helpers
 */

typedef char* (*get_next_arg_t)(void *user);

struct cmd_line_next_arg_user_t
{
    int argc;
    char **argv;
};

static char *cmd_line_next_arg(void *user)
{
    struct cmd_line_next_arg_user_t *uu = user;
    if(uu->argc == 0)
        return NULL;
    uu->argc--;
    uu->argv++;
    return *(uu->argv - 1);
}

static int sb1_add_inst(struct sb1_file_t *sb, struct sb1_inst_t *insts, int nr_insts)
{
    sb->insts = augment_array(sb->insts, sizeof(struct sb1_inst_t), sb->nr_insts,
        insts, nr_insts);
    sb->nr_insts += nr_insts;
    return 0;
}

static int sb1_add_load(struct sb1_file_t *sb, void *data, int size, uint32_t addr)
{
    while(size > 0)
    {
        int len = MIN(size, SB1_CMD_MAX_LOAD_SIZE);
        struct sb1_inst_t inst;
        memset(&inst, 0, sizeof(inst));
        inst.cmd = SB1_INST_LOAD;
        inst.size = len;
        inst.addr = addr;
        inst.critical = g_critical;
        inst.data = xmalloc(len);
        memcpy(inst.data, data, len);
        if(g_debug)
            printf("Add instruction: load %#x bytes at %#x\n", len, addr);
        int ret = sb1_add_inst(sb, &inst, 1);
        if(ret < 0)
            return ret;
        data += len;
        size -= len;
        addr += len;
    }
    return 0;
}

static int sb1_add_switch(struct sb1_file_t *sb, uint32_t driver)
{
    struct sb1_inst_t inst;
    memset(&inst, 0, sizeof(inst));
    inst.cmd = SB1_INST_MODE;
    inst.critical = g_critical;
    inst.mode = driver;
    if(g_debug)
        printf("Add instruction: switch driver to  %#x\n", driver);
    g_final = true;
    return sb1_add_inst(sb, &inst, 1);
}

static int sb1_add_sdram(struct sb1_file_t *sb, uint32_t cs, uint32_t size)
{
    struct sb1_inst_t inst;
    memset(&inst, 0, sizeof(inst));
    inst.cmd = SB1_INST_SDRAM;
    inst.critical = g_critical;
    inst.sdram.chip_select = cs;
    inst.sdram.size_index = sb1_sdram_index_by_size(size);
    if(sb1_sdram_index_by_size(size) < 0)
        bug("Unknown SDRAM size: %d MB\n", size);
    if(g_debug)
        printf("Add instruction: init SDRAM (chip select=%d, size=%d MB)\n", cs, size);
    return sb1_add_inst(sb, &inst, 1);
}

static int sb1_add_call(struct sb1_file_t *sb, uint32_t addr, uint32_t arg)
{
    struct sb1_inst_t inst;
    memset(&inst, 0, sizeof(inst));
    inst.cmd = SB1_INST_CALL;
    inst.critical = g_critical;
    inst.addr = addr;
    inst.argument = arg;
    if(g_debug)
        printf("Add instruction: call %#x with argument %#x\n", addr, arg);
    return sb1_add_inst(sb, &inst, 1);
}

static int sb1_add_jump(struct sb1_file_t *sb, uint32_t addr, uint32_t arg)
{
    struct sb1_inst_t inst;
    memset(&inst, 0, sizeof(inst));
    inst.cmd = SB1_INST_JUMP;
    inst.critical = g_critical;
    inst.addr = addr;
    inst.argument = arg;
    if(g_debug)
        printf("Add instruction: jump %#x with argument %#x\n", addr, arg);
    g_final = true;
    return sb1_add_inst(sb, &inst, 1);
}

static int sb1_add_fill(struct sb1_file_t *sb, uint32_t pattern, uint32_t size, uint32_t addr)
{
    while(size > 0)
    {
        int len = MIN(size, SB1_CMD_MAX_FILL_SIZE);
        struct sb1_inst_t inst;
        memset(&inst, 0, sizeof(inst));
        inst.cmd = SB1_INST_FILL;
        inst.critical = g_critical;
        inst.size = len;
        inst.addr = addr;
        inst.pattern = pattern;
        inst.datatype = SB1_DATATYPE_UINT32;
        if(g_debug)
            printf("Add instruction: fill %#x bytes with pattern %#x at address %#x\n",
                size, pattern, addr);
        int ret = sb1_add_inst(sb, &inst, 1);
        if(ret < 0)
            return ret;
        size -= len;
        addr += len;
    }

    return 0;
}

/**
 * SB file modification
 */

static void generate_default_sb_version(struct sb1_version_t *ver)
{
    ver->major = ver->minor = ver->revision = 0x9999;
}

static struct sb1_file_t *create_sb1_file(void)
{
    struct sb1_file_t *sb = xmalloc(sizeof(struct sb1_file_t));
    memset(sb, 0, sizeof(struct sb1_file_t));

    /* default versions and key, apply_args() will overwrite if specified */
    generate_default_sb_version(&sb->product_ver);
    generate_default_sb_version(&sb->component_ver);
    sb1_get_default_key(&sb->key);

    return sb;
}

static void *load_file(const char *filename, int *size)
{
    FILE *fd = fopen(filename, "rb");
    if(fd == NULL)
        bug("cannot open '%s' for reading\n", filename);
    if(g_debug)
        printf("Loading binary file '%s'...\n", filename);
    fseek(fd, 0, SEEK_END);
    *size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    void *data = xmalloc(*size);
    fread(data, 1, *size, fd);
    fclose(fd);
    return data;
}

static bool parse_sb_sub_version(uint16_t *ver, char *str)
{
    *ver = 0;
    for(int i = 0; str[i]; i++)
    {
        if(i >= 4)
            return false;
        if(str[i] < '0' || str[i] > '9')
            return false;
        *ver = *ver << 4 | (str[i] - '0');
    }
    return true;
}

static bool parse_sb_version(struct sb1_version_t *ver, char *str)
{
    char *p = strchr(str, '.');
    char *q = strchr(p + 1, '.');
    if(p == NULL || q == NULL) return false;
    *p = *q = 0;
    return parse_sb_sub_version(&ver->major, str) &&
            parse_sb_sub_version(&ver->minor, p + 1) &&
            parse_sb_sub_version(&ver->revision, q + 1);
}

/**
 * Command line parsing
 */

#define MAX_NR_ARGS 2

#define ARG_STR     0
#define ARG_UINT    1

#define CMD_FN(name) \
    int name(struct sb1_file_t *sb, union cmd_arg_t args[MAX_NR_ARGS])

union cmd_arg_t
{
    char *str;
    unsigned long uint;
};

typedef int (*process_arg_t)(struct sb1_file_t *sb, union cmd_arg_t args[MAX_NR_ARGS]);

struct cmd_entry_t
{
    const char *name;
    int nr_args;
    int arg_type[MAX_NR_ARGS];
    process_arg_t fn;
};

/* Callbacks */

static void usage(void);

CMD_FN(cmd_help)
{
    (void) args;
    (void) sb;
    usage();
    return 0;
}

CMD_FN(cmd_debug)
{
    (void) args;
    (void) sb;
    g_debug = true;
    return 0;
}

CMD_FN(cmd_drive_tag)
{
    sb->drive_tag = args[0].uint;
    return 0;
}

CMD_FN(cmd_load_binary)
{
    int size;
    void *data = load_file(args[0].str, &size);
    int ret = sb1_add_load(sb, data, size, args[1].uint);
    free(data);
    return ret;
}

CMD_FN(cmd_output)
{
    (void) sb;
    g_output_file = strdup(args[0].str);
    return 0;
}

CMD_FN(cmd_switch)
{
    return sb1_add_switch(sb, args[0].uint);
}

CMD_FN(cmd_sdram)
{
    return sb1_add_sdram(sb, args[0].uint, args[1].uint);
}

CMD_FN(cmd_critical)
{
    (void) sb;
    (void) args;
    g_critical = true;
    return 0;
}

CMD_FN(cmd_clear_critical)
{
    (void) sb;
    (void) args;
    g_critical = false;
    return 0;
}

CMD_FN(cmd_strict)
{
    (void) sb;
    (void) args;
    g_strict = true;
    return 0;
}

CMD_FN(cmd_clear_strict)
{
    (void) sb;
    (void) args;
    g_strict = false;
    return 0;
}

CMD_FN(cmd_call)
{
    /* FIXME: the proprietary sbtoelf always sets argument to 0 ?! */
    return sb1_add_call(sb, args[0].uint, g_jump_arg);
}

CMD_FN(cmd_jump)
{
    return sb1_add_jump(sb, args[0].uint, g_jump_arg);
}

CMD_FN(cmd_jumparg)
{
    (void) sb;
    g_jump_arg = args[0].uint;
    return 0;
}

static int load_elf(struct sb1_file_t *sb, const char *filename, int act)
{
    struct elf_params_t elf;
    FILE *fd = fopen(filename, "rb");
    if(fd == NULL)
        bug("cannot open '%s'\n", filename);
    if(g_debug)
        printf("Loading elf file '%s'...\n", filename);
    elf_init(&elf);
    bool loaded = elf_read_file(&elf, elf_std_read, generic_std_printf, fd);
    fclose(fd);
    if(!loaded)
        bug("error loading elf file '%s'\n", filename);
    elf_sort_by_address(&elf);

    struct elf_section_t *esec = elf.first_section;
    while(esec)
    {
        if(esec->type == EST_LOAD)
            sb1_add_load(sb, esec->section, esec->size, esec->addr);
        else if(esec->type == EST_FILL)
            sb1_add_fill(sb, esec->pattern, esec->size, esec->addr);
        esec = esec->next;
    }

    int ret = 0;
    if(act == SB1_INST_JUMP || act == SB1_INST_CALL)
    {
        if(!elf.has_start_addr)
            bug("Cannot jump/call: '%s' has no start address!\n", filename);
        if(act == SB1_INST_JUMP)
            ret = sb1_add_jump(sb, elf.start_addr, g_jump_arg);
        else
            ret = sb1_add_call(sb, elf.start_addr, g_jump_arg);
    }

    elf_release(&elf);

    return ret;
}

CMD_FN(cmd_load)
{
    return load_elf(sb, args[0].str, SB1_INST_LOAD);
}

CMD_FN(cmd_loadjump)
{
    return load_elf(sb, args[0].str, SB1_INST_JUMP);
}

CMD_FN(cmd_loadjumpreturn)
{
    return load_elf(sb, args[0].str, SB1_INST_CALL);
}

CMD_FN(cmd_rom)
{
    sb->rom_version = args[0].uint;
    return 0;
}

CMD_FN(cmd_product)
{
    if(!parse_sb_version(&sb->product_ver, args[0].str))
        bug("Invalid version string '%s'\n", args[0].str);
    return 0;
}

CMD_FN(cmd_component)
{
    if(!parse_sb_version(&sb->component_ver, args[0].str))
        bug("Invalid version string '%s'\n", args[0].str);
    return 0;
}

CMD_FN(cmd_keyfile)
{
    if(!add_keys_from_file(args[0].str))
        bug("Cannot add keys from file '%s'\n", args[0].str);
    for(int i = 0; i < g_nr_keys; i++)
        if(g_key_array[i].method == CRYPTO_XOR_KEY)
        {
            memcpy(&sb->key, &g_key_array[i], sizeof(sb->key));
            break;
        }
    return 0;
}

#define CMD(name,fn,nr_args,...) {name,nr_args,{__VA_ARGS__},fn},
struct cmd_entry_t g_cmds[] =
{
    CMD("-d", cmd_debug, 0)
    CMD("-debugon", cmd_debug, 0)
    CMD("-h", cmd_help, 0)
    CMD("-?", cmd_help, 0)
    CMD("-load-binary", cmd_load_binary, 2, ARG_STR, ARG_UINT)
    CMD("-drive-tag", cmd_drive_tag, 1, ARG_UINT)
    CMD("-o", cmd_output, 1, ARG_STR)
    CMD("-w", cmd_switch, 1, ARG_UINT)
    CMD("-switchdriver", cmd_switch, 1, ARG_UINT)
    CMD("-sdram", cmd_sdram, 2, ARG_UINT, ARG_UINT)
    CMD("-c", cmd_critical, 0)
    CMD("-critical", cmd_critical, 0)
    CMD("-C", cmd_clear_critical, 0)
    CMD("-noncritical", cmd_clear_critical, 0)
    CMD("-n", cmd_strict, 0)
    CMD("-strict", cmd_strict, 0)
    CMD("-N", cmd_clear_strict, 0)
    CMD("-nonstrict", cmd_clear_strict, 0)
    CMD("-call", cmd_call, 1, ARG_UINT)
    CMD("-jump", cmd_jump, 1, ARG_UINT)
    CMD("-jumparg", cmd_jumparg, 1, ARG_UINT)
    CMD("-f", cmd_load, 1, ARG_STR)
    CMD("-load", cmd_load, 1, ARG_STR)
    CMD("-r", cmd_loadjumpreturn, 1, ARG_STR)
    CMD("-loadjumpreturn", cmd_loadjumpreturn, 1, ARG_STR)
    CMD("-j", cmd_loadjump, 1, ARG_STR)
    CMD("-loadjump", cmd_loadjump, 1, ARG_STR)
    CMD("-R", cmd_rom, 1, ARG_UINT)
    CMD("-rom", cmd_rom, 1, ARG_UINT)
    CMD("-p", cmd_product, 1, ARG_STR)
    CMD("-product", cmd_product, 1, ARG_STR)
    CMD("-v", cmd_component, 1, ARG_STR)
    CMD("-component", cmd_component, 1, ARG_STR)
    CMD("-k", cmd_keyfile, 1, ARG_STR)
};
#undef CMD

#define NR_CMDS (int)(sizeof(g_cmds) / sizeof(g_cmds[0]))

static int apply_args(struct sb1_file_t *sb, get_next_arg_t next, void *user)
{
    while(true)
    {
        /* next command ? */
        char *cmd = next(user);
        if(cmd == NULL)
            break;
        /* switch */
        int i = 0;
        while(i < NR_CMDS && strcmp(cmd, g_cmds[i].name) != 0)
            i++;
        if(i == NR_CMDS)
            bug("Unknown option '%s'\n", cmd);
        union cmd_arg_t args[MAX_NR_ARGS];
        for(int j = 0; j < g_cmds[i].nr_args; j++)
        {
            args[j].str = next(user);
            if(args[j].str == NULL)
                bug("Option '%s' requires %d arguments, only %d given\n", cmd, g_cmds[i].nr_args, j);
            if(g_cmds[i].arg_type[j] == ARG_UINT)
            {
                char *end;
                args[j].uint = strtoul(args[j].str, &end, 0);
                if(*end)
                    bug("Option '%s' expects an integer as argument %d\n", cmd, j + 1);
            }
        }
        int ret = g_cmds[i].fn(sb, args);
        if(ret < 0)
            return ret;
    }
    return 0;
}

static void usage(void)
{
    printf("Usage: elftosb1 [options]\n");
    printf("Options:\n");
    printf("  -h/-?/-help\t\t\tDisplay this message\n");
    printf("  -o <file>\t\t\tSet output file\n");
    printf("  -d/-debugon\t\t\tEnable debug output\n");
    printf("  -k <file>\t\t\tSet key file\n");
    printf("  -load-binary <file> <addr>\tLoad a binary file at a specified address\n");
    printf("  -drive-tag <tag>\t\tSpecify drive tag\n");
    printf("  -w/-switchdriver <driver>\tSwitch driver\n");
    printf("  -sdram <chip select> <size>\tInit SDRAM\n");
    printf("  -jumparg <uint>\t\tSet jumpparg for jump and jumpreturn\n");
    printf("  -f/-load <file>\t\tLoad a ELF file\n");
    printf("  -r/-loadjumpreturn <file>\tLoad a ELF file and call it\n");
    printf("  -j/-loadjump <file>\t\tLoad a ELF file and jump to it\n");
    printf("  -R/-rom <uint>\t\tSet ROM version\n");
    printf("  -p/-product <ver>\t\tSet product version (xxx.xxx.xxx)\n");
    printf("  -v/-component <ver>\t\tSet component version (xxx.xxx.xxx)\n");
    printf("  -c/-critical\t\t\tSet critical flag\n");
    printf("  -C/-noncritical\t\tClear critical flag\n");
    printf("  -n/-strict\t\t\tSet strict flag\n");
    printf("  -N/-nonstrict\t\t\tClear strict flag\n");
    printf("  -call <addr>\t\t\tCall code at a specified address\n");
    printf("  -jump <addr>\t\t\tJump to code at a specified address\n");

    exit(1);
}

int main(int argc, char **argv)
{
    if(argc <= 1)
        usage();

    struct sb1_file_t *sb = create_sb1_file();

    struct cmd_line_next_arg_user_t u;
    u.argc = argc - 1;
    u.argv = argv + 1;
    int ret = apply_args(sb, &cmd_line_next_arg, &u);
    if(ret < 0)
    {
        sb1_free(sb);
        return ret;
    }

    if(!g_output_file)
        bug("You must specify an output file\n");
    if(!g_final)
    {
        if(g_strict)
            bug("There is no final command in this command stream!\n");
        else
            printf("Warning: there is no final command in this command stream!\n");
    }

    enum sb1_error_t err = sb1_write_file(sb, g_output_file);
    if(err != SB1_SUCCESS)
        printf("Error: %d\n", err);

    return ret;
}

