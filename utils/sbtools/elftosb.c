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

#include "crypto.h"
#include "elf.h"
#include "sb.h"

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

bool g_debug = false;

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
    while(*ptr != end)
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

#if 0
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
#endif

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

    if(g_debug)
        printf("Parsing command file '%s'...\n", file);
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
        memset(src, 0, sizeof(struct cmd_source_t));
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
    /* for production use */
    uint32_t padding_size;
    uint8_t *padding;
};

struct sb_section_t
{
    uint32_t identifier;
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
    if(g_debug)
        printf("Loading ELF file '%s'...\n", src->filename);
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
                    bug("cannot jump/call '%s' because it has no starting point !\n", cinst->identifier);
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
                sec->sec_size += sizeof(struct sb_instruction_call_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_FILL)
            {
                if(g_debug)
                    printf("FILL | addr=0x%08x | len=0x%08x | pattern=0x%08x\n",
                        inst->addr, inst->size, inst->pattern);
                sb->image_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_fill_t) / BLOCK_SIZE;
            }
            else if(inst->inst == SB_INST_LOAD)
            {
                if(g_debug)
                    printf("LOAD | addr=0x%08x | len=0x%08x\n", inst->addr, inst->size);
                /* load header */
                sb->image_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                sec->sec_size += sizeof(struct sb_instruction_load_t) / BLOCK_SIZE;
                /* data + alignment */
                sb->image_size += (inst->size + inst->padding_size) / BLOCK_SIZE;
                sec->sec_size += (inst->size + inst->padding_size) / BLOCK_SIZE;
            }
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

void generate_version(struct sb_version_t *ver)
{
    ver->major = 0x999;
    ver->pad0 = 0;
    ver->minor = 0x999;
    ver->pad1 = 0;
    ver->revision = 0x999;
    ver->pad2 = 0;
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
    generate_version(&sb_hdr->product_ver);
    generate_version(&sb_hdr->component_ver);
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
    sec_hdr->flags = SECTION_BOOTABLE;
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
    tag->flags = SECTION_BOOTABLE;
    tag->hdr.checksum = instruction_checksum(&tag->hdr);
}

void produce_sb_instruction(struct sb_inst_t *inst,
    struct sb_instruction_common_t *cmd)
{
    cmd->hdr.flags = 0;
    cmd->hdr.opcode = inst->inst;
    cmd->addr = inst->addr;
    cmd->len = inst->size;
    switch(inst->inst)
    {
        case SB_INST_CALL:
        case SB_INST_JUMP:
            cmd->len = 0;
            cmd->data = 0;
            break;
        case SB_INST_FILL:
            cmd->data = inst->pattern;
            break;
        case SB_INST_LOAD:
            cmd->data = crc_continue(crc(inst->data, inst->size),
                inst->padding, inst->padding_size);
            break;
        default:
            break;
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
            struct sb_instruction_common_t cmd;
            produce_sb_instruction(inst, &cmd);
            if(g_nr_keys > 0)
                cbc_mac((byte *)&cmd, (byte *)&cmd, sizeof(cmd) / BLOCK_SIZE,
                    real_key, cur_cbc_mac, &cur_cbc_mac, 1);
            sha_1_update(&file_sha1, (byte *)&cmd, sizeof(cmd));
            write(fd, &cmd, sizeof(cmd));
            /* data */
            if(inst->inst == SB_INST_LOAD)
            {
                uint32_t sz = inst->size + inst->padding_size;
                byte *data = xmalloc(sz);
                memcpy(data, inst->data, inst->size);
                memcpy(data + inst->size, inst->padding, inst->padding_size);
                if(g_nr_keys > 0)
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

int main(int argc, const char **argv)
{
    if(argc != 4)
    {
        printf("Usage: %s <cmd file> <key file> <out file>\n",*argv);
        printf("To enable debug mode, set environement variable SB_DEBUG to YES\n");
        return 1;
    }

    if(strcasecmp(s_getenv("SB_DEBUG"), "YES") == 0)
        g_debug = true;

    g_key_array = read_keys(argv[2], &g_nr_keys);
    struct cmd_file_t *cmd_file = read_command_file(argv[1]);
    struct sb_file_t *sb_file = apply_cmd_file(cmd_file);
    produce_sb_file(sb_file, argv[3]);
    
    return 0;
}
