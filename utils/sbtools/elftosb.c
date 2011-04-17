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

#include "crypto.h"
#include "elf.h"
#include "sb.h"

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

bool g_debug = false;

/**
 * Misc
 */

void *xmalloc(size_t s) /* malloc helper, used in elf.c */
{
    void * r = malloc(s);
    if(!r) bugp("malloc");
    return r;
}

static int convxdigit(char digit, byte *val)
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
        pos += 32;
    }
    free(buf);

    return keys;
}

/**
 * Command file parsing
 */

struct cmd_source_t
{
    char *identifier;
    char *filename;
    struct cmd_source_t *next;
    /* for later use */
    bool elf_loaded;
    struct elf_params_t elf;
};

enum cmd_inst_type_t
{
    CMD_LOAD,
    CMD_JUMP,
    CMD_CALL
};

struct cmd_inst_t
{
    enum cmd_inst_type_t type;
    char *identifier;
    struct cmd_inst_t *next;
};

struct cmd_section_t
{
    uint32_t identifier;
    struct cmd_inst_t *inst_list;
    struct cmd_section_t *next;
};

struct cmd_file_t
{
    struct cmd_source_t *source_list;
    struct cmd_section_t *section_list;
};

enum lexem_type_t
{
    LEX_IDENTIFIER,
    LEX_LPAREN,
    LEX_RPAREN,
    LEX_NUMBER,
    LEX_STRING, /* double-quoted string */
    LEX_EQUAL,
    LEX_SEMICOLON,
    LEX_LBRACE,
    LEX_RBRACE,
    LEX_EOF
};

struct lexem_t
{
    enum lexem_type_t type;
    char *str;
    uint32_t num;
};

static void __parse_string(char **ptr, char *end, void *user, void (*emit_fn)(void *user, char c))
{
    while(*ptr != end)
    {
        if(**ptr == '"')
            break;
        else if(**ptr == '\\')
        {
            (*ptr)++;
            if(*ptr == end)
                bug("Unfinished string");
            if(**ptr == '\\') emit_fn(user, '\\');
            else if(**ptr == '\'') emit_fn(user, '\'');
            else if(**ptr == '\"') emit_fn(user, '\"');
            else bug("Unknown escape sequence \\%c", **ptr);
            (*ptr)++;
        }
        else
            emit_fn(user, *(*ptr)++);
    }
    if(*ptr == end || **ptr != '"')
        bug("unfinished string");
    (*ptr)++;
}

static void __parse_string_emit(void *user, char c)
{
    char **pstr = (char **)user;
    *(*pstr)++ = c;
}

static void __parse_string_count(void *user, char c)
{
    (void) c;
    (*(int *)user)++;
}

static void parse_string(char **ptr, char *end, struct lexem_t *lexem)
{
    /* skip " */
    (*ptr)++;
    char *p = *ptr;
    /* compute length */
    int length = 0;
    __parse_string(&p, end, (void *)&length, __parse_string_count);
    /* parse again */
    lexem->type = LEX_STRING;
    lexem->str = xmalloc(length + 1);
    lexem->str[length] = 0;
    char *pstr = lexem->str;
    __parse_string(ptr, end, (void *)&pstr, __parse_string_emit);
}

static void parse_number(char **ptr, char *end, struct lexem_t *lexem)
{
    int base = 10;
    if(**ptr == '0' && (*ptr) + 1 != end && (*ptr)[1] == 'x')
    {
        (*ptr) += 2;
        base = 16;
    }

    lexem->type = LEX_NUMBER;
    lexem->num = 0;
    while(*ptr != end && isxdigit(**ptr))
    {
        if(base == 10 && !isdigit(**ptr))
            break;
        byte v;
        if(convxdigit(**ptr, &v))
            break;
        lexem->num = base * lexem->num + v;
        (*ptr)++;
    }
}

static void parse_identifier(char **ptr, char *end, struct lexem_t *lexem)
{
    /* remember position */
    char *old = *ptr;
    while(*ptr != end && (isalnum(**ptr) || **ptr == '_'))
        (*ptr)++;
    lexem->type = LEX_IDENTIFIER;
    int len = *ptr - old;
    lexem->str = xmalloc(len + 1);
    lexem->str[len] = 0;
    memcpy(lexem->str, old, len);
}

static void next_lexem(char **ptr, char *end, struct lexem_t *lexem)
{
    #define ret_simple(t, advance) ({(*ptr) += advance; lexem->type = t; return;})
    while(true)
    {
        /* skip whitespace */
        if(**ptr == ' ' || **ptr == '\t' || **ptr == '\n' || **ptr == '\r')
        {
            (*ptr)++;
            continue;
        }
        /* skip comments */
        if(**ptr == '/' && (*ptr) + 1 != end && (*ptr)[1] == '/')
        {
            while(*ptr != end && **ptr != '\n')
                (*ptr)++;
            continue;
        }
        break;
    }
    if(*ptr == end) ret_simple(LEX_EOF, 0);
    if(**ptr == '(') ret_simple(LEX_LPAREN, 1);
    if(**ptr == ')') ret_simple(LEX_RPAREN, 1);
    if(**ptr == '{') ret_simple(LEX_LBRACE, 1);
    if(**ptr == '}') ret_simple(LEX_RBRACE, 1);
    if(**ptr == '=') ret_simple(LEX_EQUAL, 1);
    if(**ptr == ';') ret_simple(LEX_SEMICOLON, 1);
    if(**ptr == '"') return parse_string(ptr, end, lexem);
    if(isdigit(**ptr)) return parse_number(ptr, end, lexem);
    if(isalpha(**ptr) || **ptr == '_') return parse_identifier(ptr, end, lexem);
    bug("Unexpected character '%c' in command file\n", **ptr);
    #undef ret_simple
}

static void log_lexem(struct lexem_t *lexem)
{
    switch(lexem->type)
    {
        case LEX_EOF: printf("<eof>"); break;
        case LEX_EQUAL: printf("="); break;
        case LEX_IDENTIFIER: printf("id(%s)", lexem->str); break;
        case LEX_LPAREN: printf("("); break;
        case LEX_RPAREN: printf(")"); break;
        case LEX_LBRACE: printf("{"); break;
        case LEX_RBRACE: printf("}"); break;
        case LEX_SEMICOLON: printf(";"); break;
        case LEX_NUMBER: printf("num(%d)", lexem->num); break;
        case LEX_STRING: printf("str(%s)", lexem->str); break;
        default: printf("<unk>");
    }
}

static struct cmd_source_t *find_source_by_id(struct cmd_file_t *cmd_file, const char *id)
{
    struct cmd_source_t *src = cmd_file->source_list;
    while(src)
    {
        if(strcmp(src->identifier, id) == 0)
            return src;
        src = src->next;
    }
    return NULL;
}

static struct cmd_file_t *read_command_file(const char *file)
{
    int size;
    struct stat st;
    int fd = open(file,O_RDONLY);
    if(fd == -1)
        bugp("opening command file failed");
    if(fstat(fd,&st) == -1)
        bugp("command file stat() failed");
    size = st.st_size;
    char *buf = xmalloc(size);
    if(read(fd, buf, size) != (ssize_t)size)
        bugp("reading command file");
    close(fd);

    struct cmd_file_t *cmd_file = xmalloc(sizeof(struct cmd_file_t));
    memset(cmd_file, 0, sizeof(struct cmd_file_t));

    struct lexem_t lexem;
    char *p = buf;
    char *end = buf + size;
    #define next() next_lexem(&p, end, &lexem)
    /* sources */
    next();
    if(lexem.type != LEX_IDENTIFIER || strcmp(lexem.str, "sources") != 0)
        bug("invalid command file: 'sources' expected");
    next();
    if(lexem.type != LEX_LBRACE)
        bug("invalid command file: '{' expected after 'sources'");

    while(true)
    {
        next();
        if(lexem.type == LEX_RBRACE)
            break;
        struct cmd_source_t *src = xmalloc(sizeof(struct cmd_source_t));
        src->next = cmd_file->source_list;
        if(lexem.type != LEX_IDENTIFIER)
            bug("invalid command file: identifier expected in sources");
        src->identifier = lexem.str;
        next();
        if(lexem.type != LEX_EQUAL)
            bug("invalid command file: '=' expected after identifier");
        next();
        if(lexem.type != LEX_STRING)
            bug("invalid command file: string expected after '='");
        src->filename = lexem.str;
        next();
        if(lexem.type != LEX_SEMICOLON)
            bug("invalid command file: ';' expected after string");
        if(find_source_by_id(cmd_file, src->identifier) != NULL)
            bug("invalid command file: duplicated source identifier");
        cmd_file->source_list = src;
    }

    /* sections */
    struct cmd_section_t *end_sec = NULL;
    while(true)
    {
        struct cmd_section_t *sec = xmalloc(sizeof(struct cmd_section_t));
        struct cmd_inst_t *end_list = NULL;
        memset(sec, 0, sizeof(struct cmd_section_t));
        next();
        if(lexem.type == LEX_EOF)
            break;
        if(lexem.type != LEX_IDENTIFIER || strcmp(lexem.str, "section") != 0)
            bug("invalid command file: 'section' expected");
        next();
        if(lexem.type != LEX_LPAREN)
            bug("invalid command file: '(' expected after 'section'");
        next();
        if(lexem.type != LEX_NUMBER)
            bug("invalid command file: number expected as section identifier");
        sec->identifier = lexem.num;
        next();
        if(lexem.type != LEX_RPAREN)
            bug("invalid command file: ')' expected after section identifier");
        next();
        if(lexem.type != LEX_LBRACE)
            bug("invalid command file: '{' expected after section directive");
        /* commands */
        while(true)
        {
            struct cmd_inst_t *inst = xmalloc(sizeof(struct cmd_inst_t));
            memset(inst, 0, sizeof(struct cmd_inst_t));
            next();
            if(lexem.type == LEX_RBRACE)
                break;
            if(lexem.type != LEX_IDENTIFIER)
                bug("invalid command file: instruction expected in section");
            if(strcmp(lexem.str, "load") == 0)
                inst->type = CMD_LOAD;
            else if(strcmp(lexem.str, "call") == 0)
                inst->type = CMD_CALL;
            else if(strcmp(lexem.str, "jump") == 0)
                inst->type = CMD_JUMP;
            else
                bug("invalid command file: instruction expected in section");
            next();
            if(lexem.type != LEX_IDENTIFIER)
                bug("invalid command file: identifier expected after instruction");
            inst->identifier = lexem.str;
            if(find_source_by_id(cmd_file, inst->identifier) == NULL)
                bug("invalid command file: undefined reference to source '%s'", inst->identifier);
            next();
            if(lexem.type != LEX_SEMICOLON)
                bug("invalid command file: expected ';' after command");

            if(end_list == NULL)
            {
                sec->inst_list = inst;
                end_list = inst;
            }
            else
            {
                end_list->next = inst;
                end_list = inst;
            }
        }

        if(end_sec == NULL)
        {
            cmd_file->section_list = sec;
            end_sec = sec;
        }
        else
        {
            end_sec->next = sec;
            end_sec = sec;
        }
    }
    #undef next

    return cmd_file;
}

/**
 * command file to sb conversion
 */

struct sb_inst_t
{
    uint8_t inst; /* SB_INST_* */
    uint32_t size;
    // <union>
    void *data;
    uint32_t pattern;
    uint32_t addr;
    // </union>
};

struct sb_section_t
{
    uint32_t identifier;
    int nr_insts;
    struct sb_inst_t *insts;
    /* for production use */
    uint32_t file_offset; /* in blocks */
};

struct sb_file_t
{
    int nr_sections;
    struct sb_section_t *sections;
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

static void load_elf_by_id(struct cmd_file_t *cmd_file, const char *id)
{
    struct cmd_source_t *src = find_source_by_id(cmd_file, id);
    if(src == NULL)
        bug("undefined reference to source '%s'\n", id);
    /* avoid reloading */
    if(src->elf_loaded)
        return;
    int fd = open(src->filename, O_RDONLY);
    if(fd < 0)
        bug("cannot open '%s' (id '%s')\n", src->filename, id);
    elf_init(&src->elf);
    src->elf_loaded = elf_read_file(&src->elf, elf_read, elf_printf, &fd);
    close(fd);
    if(!src->elf_loaded)
        bug("error loading elf file '%s' (id '%s')\n", src->filename, id);
}

static struct sb_file_t *apply_cmd_file(struct cmd_file_t *cmd_file)
{
    struct sb_file_t *sb = xmalloc(sizeof(struct sb_file_t));
    memset(sb, 0, sizeof(struct sb_file_t));
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
        /* count instructions */
        struct cmd_inst_t *cinst = csec->inst_list;
        while(cinst)
        {
            load_elf_by_id(cmd_file, cinst->identifier);
            struct elf_params_t *elf = &find_source_by_id(cmd_file, cinst->identifier)->elf;

            if(cinst->type == CMD_LOAD)
                sec->nr_insts += elf_get_nr_sections(elf);
            else if(cinst->type == CMD_JUMP || cinst->type == CMD_CALL)
            {
                if(!elf_get_start_addr(elf, NULL))
                    bug("cannot jump/call '%s' because it has no starting point !", cinst->identifier);
                sec->nr_insts++;
            }
            
            cinst = cinst->next;
        }

        sec->insts = xmalloc(sec->nr_insts * sizeof(struct sb_inst_t));
        memset(sec->insts, 0, sec->nr_insts * sizeof(struct sb_inst_t));
        /* flatten */
        int idx = 0;
        cinst = csec->inst_list;
        while(cinst)
        {
            struct elf_params_t *elf = &find_source_by_id(cmd_file, cinst->identifier)->elf;

            if(cinst->type == CMD_LOAD)
            {
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
                sec->insts[idx].inst = (cinst->type == CMD_JUMP) ? SB_INST_JUMP : SB_INST_CALL;
                sec->insts[idx++].addr = elf->start_addr;
            }
            
            cinst = cinst->next;
        }
    }

    return sb;
}

/**
 * Sb file production
 */

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
        sec->file_offset = sb->image_size;
        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            if(inst->inst == SB_INST_CALL || inst->inst == SB_INST_JUMP)
            {
                if(g_debug)
                    printf("%s | addr=0x%08x | arg=0x%08x\n",
                        inst->inst == SB_INST_CALL ? "CALL" : "JUMP", inst->addr, 0);
                sb->image_size += sizeof(struct sb_instruction_call_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_FILL)
            {
                if(g_debug)
                    printf("FILL | addr=0x%08x | len=0x%08x | pattern=0x%08x\n",
                        inst->addr, inst->size, inst->pattern);
                sb->image_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_LOAD)
            {
                if(g_debug)
                    printf("LOAD | addr=0x%08x | len=0x%08x\n", inst->addr, inst->size);
                /* load header */
                sb->image_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                /* data + alignment */
                sb->image_size += (inst->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
            }
        }
    }
    /* final signature */
    sb->image_size += 2;
}

static void produce_sb_header(struct sb_file_t *sb, struct sb_header_t *sb_hdr)
{
    sb_hdr->signature[0] = 'S';
    sb_hdr->signature[1] = 'T';
    sb_hdr->signature[2] = 'M';
    sb_hdr->signature[3] = 'P';
    sb_hdr->major_ver = IMAGE_MAJOR_VERSION;
    sb_hdr->minor_ver = IMAGE_MINOR_VERSION;
    sb_hdr->flags = 0;
    sb_hdr->image_size = sb->image_size;
}

static void produce_sb_file(struct sb_file_t *sb, const char *filename)
{
    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd < 0)
        bugp("cannot open output file");

    compute_sb_offsets(sb);

    struct sb_header_t sb_hdr;
    produce_sb_header(sb, &sb_hdr);
    
    close(fd);
}

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

static uint8_t instruction_checksum(struct sb_instruction_header_t *hdr)
{
    uint8_t sum = 90;
    byte *ptr = (byte *)hdr;
    for(int i = 1; i < 16; i++)
        sum += ptr[i];
    return sum;
}

int main(int argc, const char **argv)
{
    if(argc != 4)
    {
        printf("Usage: %s <cmd file> <key file> <out file>\n",*argv);
        printf("To enable debug mode, set environement variable SB_DEBUG to YES\n");
        return 1;
    }

    if(getenv("SB_DEBUG") != NULL && strcmp(getenv("SB_DEBUG"), "YES") == 0)
        g_debug = true;

    g_key_array = read_keys(argv[2], &g_nr_keys);
    struct cmd_file_t *cmd_file = read_command_file(argv[1]);
    struct sb_file_t *sb_file = apply_cmd_file(cmd_file);
    produce_sb_file(sb_file, argv[3]);
    
    return 0;
}
