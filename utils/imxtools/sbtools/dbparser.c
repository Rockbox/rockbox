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

#define _POSIX_C_SOURCE 200809L /* for strdup */
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "dbparser.h"
#include "misc.h"

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
    LEX_RANGLE,
    LEX_OR,
    LEX_LSHIFT,
    LEX_COLON,
    LEX_LE,
    LEX_EOF
};

struct lexem_t
{
    enum lexem_type_t type;
    /* if str is not NULL, it must be a malloc'd pointer */
    char *str;
    uint32_t num;
    int line;
    const char *file;
};

struct context_t
{
    const char *file;
    char *begin;
    char *end;
    char *ptr;
    int line;
};

#define parse_error(ctx, ...) \
    do { fprintf(stderr, "%s:%d: ", ctx->file, ctx->line); \
        fprintf(stderr, __VA_ARGS__); exit(2); } while(0)

static void advance(struct context_t *ctx, int nr_chars)
{
    while(nr_chars--)
    {
        if(*(ctx->ptr++) == '\n')
            ctx->line++;
    }
}

static inline bool eof(struct context_t *ctx)
{
    return ctx->ptr == ctx->end;
}

static inline bool next_valid(struct context_t *ctx, int nr)
{
    return ctx->ptr + nr < ctx->end;
}

static inline char cur_char(struct context_t *ctx)
{
    return *ctx->ptr;
}

static inline char next_char(struct context_t *ctx, int nr)
{
    return ctx->ptr[nr];
}

static inline void locate_lexem(struct lexem_t *lex, struct context_t *ctx)
{
    lex->file = ctx->file;
    lex->line = ctx->line;
}

static void __parse_string(struct context_t *ctx, void *user, void (*emit_fn)(void *user, char c))
{
    while(!eof(ctx))
    {
        if(cur_char(ctx) == '"')
            break;
        else if(cur_char(ctx) == '\\')
        {
            advance(ctx, 1);
            if(eof(ctx))
                parse_error(ctx, "Unfinished string\n");
            if(cur_char(ctx) == '\\') emit_fn(user, '\\');
            else if(cur_char(ctx) == '\'') emit_fn(user, '\'');
            else if(cur_char(ctx) == '\"') emit_fn(user, '\"');
            else parse_error(ctx, "Unknown escape sequence \\%c\n", cur_char(ctx));
            advance(ctx, 1);
        }
        else
        {
            emit_fn(user, cur_char(ctx));
            advance(ctx, 1);
        }
    }
    if(eof(ctx) || cur_char(ctx) != '"')
        parse_error(ctx, "Unfinished string\n");
    advance(ctx, 1);
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

static void parse_string(struct context_t *ctx, struct lexem_t *lexem)
{
    locate_lexem(lexem, ctx);
    /* skip " */
    advance(ctx, 1);
    /* compute length */
    struct context_t cpy_ctx = *ctx;
    int length = 0;
    __parse_string(&cpy_ctx, (void *)&length, __parse_string_count);
    /* parse again */
    lexem->type = LEX_STRING;
    lexem->str = xmalloc(length + 1);
    lexem->str[length] = 0;
    char *pstr = lexem->str;
    __parse_string(ctx, (void *)&pstr, __parse_string_emit);
}

static void parse_ascii_number(struct context_t *ctx, struct lexem_t *lexem)
{
    locate_lexem(lexem, ctx);
    /* skip ' */
    advance(ctx, 1);
    /* we expect n<=4 character and then '  */
    int len = 0;
    uint32_t value = 0;
    while(!eof(ctx))
    {
        if(cur_char(ctx) != '\'')
        {
            value = value << 8 | cur_char(ctx);
            len++;
            advance(ctx, 1);
        }
        else
            break;
    }
    if(eof(ctx) || cur_char(ctx) != '\'')
        parse_error(ctx, "Unterminated ascii number literal\n");
    if(len == 0 || len > 4)
        parse_error(ctx, "Invalid ascii number literal length: only 1 to 4 characters allowed\n");
    /* skip ' */
    advance(ctx, 1);
    lexem->type = LEX_NUMBER;
    lexem->num = value;
}

static void parse_number(struct context_t *ctx, struct lexem_t *lexem)
{
    locate_lexem(lexem, ctx);
    /* check base */
    int base = 10;
    if(cur_char(ctx) == '0' && next_valid(ctx, 1) && next_char(ctx, 1) == 'x')
    {
        advance(ctx, 2);
        base = 16;
    }

    lexem->type = LEX_NUMBER;
    lexem->num = 0;
    while(!eof(ctx) && isxdigit(cur_char(ctx)))
    {
        if(base == 10 && !isdigit(cur_char(ctx)))
            break;
        byte v;
        if(convxdigit(cur_char(ctx), &v))
            break;
        lexem->num = base * lexem->num + v;
        advance(ctx, 1);
    }
}

static void parse_identifier(struct context_t *ctx, struct lexem_t *lexem)
{
    locate_lexem(lexem, ctx);
    /* remember position */
    char *old = ctx->ptr;
    while(!eof(ctx) && (isalnum(cur_char(ctx)) || cur_char(ctx) == '_'))
        advance(ctx, 1);
    lexem->type = LEX_IDENTIFIER;
    int len = ctx->ptr - old;
    lexem->str = xmalloc(len + 1);
    lexem->str[len] = 0;
    memcpy(lexem->str, old, len);
}

static void next_lexem(struct context_t *ctx, struct lexem_t *lexem)
{
    #define ret_simple(t, adv) \
        do {locate_lexem(lexem, ctx); \
            lexem->type = t; \
            advance(ctx, adv); \
            return;} while(0)
    while(!eof(ctx))
    {
        char c = cur_char(ctx);
        /* skip whitespace */
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            advance(ctx, 1);
            continue;
        }
        /* skip C++ style comments */
        if(c == '/' && next_valid(ctx, 1) && next_char(ctx, 1) == '/')
        {
            while(!eof(ctx) && cur_char(ctx) != '\n')
                advance(ctx, 1);
            continue;
        }
        /* skip C-style comments */
        if(c == '/' && next_valid(ctx, 1) && next_char(ctx, 1) == '*')
        {
            advance(ctx, 2);
            while(true)
            {
                if(!next_valid(ctx, 1))
                    parse_error(ctx, "Unterminated comment");
                if(cur_char(ctx) == '*' && next_char(ctx, 1) == '/')
                {
                    advance(ctx, 2);
                    break;
                }
                advance(ctx, 1);
            }
            continue;
        }
        break;
    }
    if(eof(ctx)) ret_simple(LEX_EOF, 0);
    char c = cur_char(ctx);
    bool nv = next_valid(ctx, 1);
    char nc = nv  ? next_char(ctx, 1) : 0;
    if(c == '(') ret_simple(LEX_LPAREN, 1);
    if(c == ')') ret_simple(LEX_RPAREN, 1);
    if(c == '{') ret_simple(LEX_LBRACE, 1);
    if(c == '}') ret_simple(LEX_RBRACE, 1);
    if(c == '>') ret_simple(LEX_RANGLE, 1);
    if(c == '=') ret_simple(LEX_EQUAL, 1);
    if(c == ';') ret_simple(LEX_SEMICOLON, 1);
    if(c == ',') ret_simple(LEX_COLON, 1);
    if(c == '|') ret_simple(LEX_OR, 1);
    if(c == '<' && nv && nc == '<') ret_simple(LEX_LSHIFT, 2);
    if(c == '<' && nv && nc == '=') ret_simple(LEX_LE, 2);
    if(c == '"') return parse_string(ctx, lexem);
    if(c == '\'') return parse_ascii_number(ctx, lexem);
    if(isdigit(c)) return parse_number(ctx, lexem);
    if(isalpha(c) || c == '_') return parse_identifier(ctx, lexem);
    parse_error(ctx, "Unexpected character '%c'\n", c);
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
        case LEX_OR: printf("|"); break;
        case LEX_LSHIFT: printf("<<"); break;
        default: printf("<unk>");
    }
}
#endif

struct cmd_option_t *db_add_opt(struct cmd_option_t **opt, const char *identifier, bool is_str)
{
    while(*opt)
        opt = &(*opt)->next;
    *opt = xmalloc(sizeof(struct cmd_option_t));
    memset(*opt, 0, sizeof(struct cmd_option_t));
    (*opt)->name = strdup(identifier);
    (*opt)->is_string = is_str;
    return *opt;
}

void db_add_str_opt(struct cmd_option_t **opt, const char *name, const char *value)
{
    db_add_opt(opt, name, true)->str = strdup(value);
}

void db_add_int_opt(struct cmd_option_t **opt, const char *name, uint32_t value)
{
    db_add_opt(opt, name, false)->val = value;
}

static struct cmd_source_t *db_add_src(struct cmd_source_t **src, const char *identifier, bool is_extern)
{
    while(*src)
        src = &(*src)->next;
    *src = xmalloc(sizeof(struct cmd_source_t));
    memset(*src, 0, sizeof(struct cmd_source_t));
    (*src)->identifier = strdup(identifier);
    (*src)->is_extern = is_extern;
    return *src;
}

void db_add_source(struct cmd_file_t *cmd_file, const char *identifier, const char *filename)
{
    db_add_src(&cmd_file->source_list, identifier, false)->filename = strdup(filename);
}

void db_add_extern_source(struct cmd_file_t *cmd_file, const char *identifier, int extern_nr)
{
    db_add_src(&cmd_file->source_list, identifier, true)->extern_nr = extern_nr;
}

static struct cmd_inst_t *db_add_inst(struct cmd_inst_t **list, enum cmd_inst_type_t type,
    uint32_t argument)
{
    while(*list)
        list = &(*list)->next;
    *list = xmalloc(sizeof(struct cmd_inst_t));
    memset(*list, 0, sizeof(struct cmd_inst_t));
    (*list)->type = type;
    (*list)->argument = argument;
    return *list;
}

void db_add_inst_id(struct cmd_section_t *cmd_section, enum cmd_inst_type_t type,
    const char *identifier, uint32_t argument)
{
    db_add_inst(&cmd_section->inst_list, type, argument)->identifier = strdup(identifier);
}

void db_add_inst_addr(struct cmd_section_t *cmd_section, enum cmd_inst_type_t type,
    uint32_t addr, uint32_t argument)
{
    db_add_inst(&cmd_section->inst_list, type, argument)->addr = addr;
}

struct cmd_section_t *db_add_section(struct cmd_file_t *cmd_file, uint32_t identifier, bool data)
{
    struct cmd_section_t **prev = &cmd_file->section_list;
    while(*prev)
        prev = &(*prev)->next;
    *prev = xmalloc(sizeof(struct cmd_section_t));
    memset(*prev, 0, sizeof(struct cmd_section_t));
    (*prev)->identifier = identifier;
    (*prev)->is_data = data;
    return *prev;
}

struct cmd_source_t *db_find_source_by_id(struct cmd_file_t *cmd_file, const char *id)
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

struct cmd_option_t *db_find_option_by_id(struct cmd_option_t *opt, const char *name)
{
    while(opt)
    {
        if(strcmp(opt->name, name) == 0)
            return opt;
        opt = opt->next;
    }
    return NULL;
}

#define INVALID_SB_SUBVERSION   0xffff

static const char *parse_sb_subversion(const char *str, uint16_t *v)
{
    int len = 0;
    *v = 0;
    while(isdigit(str[len]) && len < 3)
        *v = (*v) << 4 | (str[len++] - '0');
    if(len == 0)
        *v = INVALID_SB_SUBVERSION;
    return str + len;
}

bool db_parse_sb_version(struct sb_version_t *ver, const char *str)
{
    str = parse_sb_subversion(str, &ver->major);
    if(ver->major == INVALID_SB_SUBVERSION || *str != '.')
        return false;
    str = parse_sb_subversion(str + 1, &ver->minor);
    if(ver->minor == INVALID_SB_SUBVERSION || *str != '.')
        return false;
    str = parse_sb_subversion(str + 1, &ver->revision);
    if(ver->revision == INVALID_SB_SUBVERSION || *str != 0)
        return false;
    return true;
}

static bool db_generate_sb_subversion(uint16_t subver, char *str)
{
    str[0] = '0' + ((subver >> 8) & 0xf);
    str[1] = '0' + ((subver >> 4) & 0xf);
    str[2] = '0' + (subver & 0xf);
    return true;
}

bool db_generate_sb_version(struct sb_version_t *ver, char *str, int size)
{
    if(size < 12)
        return false;
    str[3] = '.';
    str[7] = '.';
    str[11] = 0;
    return db_generate_sb_subversion(ver->major, str) &&
        db_generate_sb_subversion(ver->minor, str + 4) &&
        db_generate_sb_subversion(ver->revision, str + 8);
}

#undef parse_error
#define parse_error(lexem, ...) \
    do { fprintf(stderr, "%s:%d: ", lexem.file, lexem.line); \
        fprintf(stderr, __VA_ARGS__); exit(2); } while(0)

struct lex_ctx_t
{
    struct context_t ctx;
    struct lexem_t lexem;
};

/* When lexems hold strings (like identifier), it might be useful to steal
 * the pointer and don't clean the lexem but in other case, one don't want
 * to keep the pointer to the string and just want to release the memory.
 * Thus clean_lexem should be true except when one keeps a pointer */
static inline void next(struct lex_ctx_t *ctx, bool clean_lexem)
{
    if(clean_lexem)
        free(ctx->lexem.str);
    memset(&ctx->lexem, 0, sizeof(struct lexem_t));
    next_lexem(&ctx->ctx, &ctx->lexem);
}

static uint32_t parse_term_expr(struct lex_ctx_t *ctx, struct cmd_option_t *const_list)
{
    uint32_t ret = 0;
    if(ctx->lexem.type == LEX_NUMBER)
        ret = ctx->lexem.num;
    else if(ctx->lexem.type == LEX_IDENTIFIER)
    {
        struct cmd_option_t *c = db_find_option_by_id(const_list, ctx->lexem.str);
        if(c == NULL)
            parse_error(ctx->lexem, "Undefined reference to constant '%s'\n", ctx->lexem.str);
        if(c->is_string)
            parse_error(ctx->lexem, "Internal error: constant '%s' is not an integer\n", ctx->lexem.str);
        ret = c->val;
    }
    else
        parse_error(ctx->lexem, "Number or constant identifier expected\n");
    next(ctx, true);
    return ret;
}

static uint32_t parse_shift_expr(struct lex_ctx_t *ctx, struct cmd_option_t *const_list)
{
    uint32_t v = parse_term_expr(ctx, const_list);
    while(ctx->lexem.type == LEX_LSHIFT)
    {
        next(ctx, true);
        v <<= parse_term_expr(ctx, const_list);
    }
    return v;
}

static uint32_t parse_or_expr(struct lex_ctx_t *ctx, struct cmd_option_t *const_list)
{
    uint32_t v = parse_shift_expr(ctx, const_list);
    while(ctx->lexem.type == LEX_OR)
    {
        next(ctx, true);
        v |= parse_shift_expr(ctx, const_list);
    }
    return v;
}

static uint32_t parse_intexpr(struct lex_ctx_t *ctx, struct cmd_option_t *const_list)
{
    return parse_or_expr(ctx, const_list);
}

#define NR_INITIAL_CONSTANTS    4
static char *init_const_name[NR_INITIAL_CONSTANTS] = {"true", "false", "yes", "no"};
static uint32_t init_const_value[NR_INITIAL_CONSTANTS] = {1, 0, 1, 0};

struct cmd_file_t *db_parse_file(const char *file)
{
    size_t size;
    FILE *f = fopen(file, "r");
    if(f == NULL)
    {
        if(g_debug)
            perror("Cannot open db file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = xmalloc(size);
    if(fread(buf, size, 1, f) != 1)
    {
        if(g_debug)
            perror("Cannot read db file");
        return NULL;
    }
    fclose(f);

    if(g_debug)
        printf("Parsing db file '%s'\n", file);
    struct cmd_file_t *cmd_file = xmalloc(sizeof(struct cmd_file_t));
    memset(cmd_file, 0, sizeof(struct cmd_file_t));

    /* add initial constants */
    for(int i = 0; i < NR_INITIAL_CONSTANTS; i++)
        db_add_int_opt(&cmd_file->constant_list, init_const_name[i], init_const_value[i]);

    struct lex_ctx_t lctx;
    lctx.ctx.file = file;
    lctx.ctx.line = 1;
    lctx.ctx.begin = buf;
    lctx.ctx.ptr = buf;
    lctx.ctx.end = buf + size;
    #define next(clean_lexem) next(&lctx, clean_lexem)
    #define lexem lctx.lexem
    /* init lexer */
    next(false); /* don't clean init lexem because it doesn't exist */
    /* constants ? */
    if(lexem.type == LEX_IDENTIFIER && !strcmp(lexem.str, "constants"))
    {
        next(true);
        if(lexem.type != LEX_LBRACE)
            parse_error(lexem, "'{' expected after 'constants'\n");

        while(true)
        {
            next(true);
            if(lexem.type == LEX_RBRACE)
                break;
            if(lexem.type != LEX_IDENTIFIER)
                parse_error(lexem, "Identifier expected in constants\n");
            const char *name = lexem.str;
            next(false); /* lexem string is kept as option name */
            if(lexem.type != LEX_EQUAL)
                parse_error(lexem, "'=' expected after identifier\n");
            next(true);
            db_add_int_opt(&cmd_file->constant_list, name, parse_intexpr(&lctx, cmd_file->constant_list));
            if(lexem.type != LEX_SEMICOLON)
               parse_error(lexem, "';' expected after string\n");
        }
        next(true);
    }
    /* options ? */
    if(lexem.type == LEX_IDENTIFIER && !strcmp(lexem.str, "options"))
    {
        next(true);
        if(lexem.type != LEX_LBRACE)
            parse_error(lexem, "'{' expected after 'options'\n");

        while(true)
        {
            next(true);
            if(lexem.type == LEX_RBRACE)
                break;
            if(lexem.type != LEX_IDENTIFIER)
                parse_error(lexem, "Identifier expected in options\n");
            const char *name = lexem.str;
            next(false); /* lexem string is kept as option name */
            if(lexem.type != LEX_EQUAL)
                parse_error(lexem, "'=' expected after identifier\n");
            next(true);
            if(lexem.type == LEX_STRING)
            {
                db_add_str_opt(&cmd_file->opt_list, name, lexem.str);
                next(true);
            }
            else
                db_add_int_opt(&cmd_file->opt_list, name, parse_intexpr(&lctx, cmd_file->constant_list));
            if(lexem.type != LEX_SEMICOLON)
               parse_error(lexem, "';' expected after string\n");
        }
        next(true);
    }
    /* sources */
    if(lexem.type != LEX_IDENTIFIER || strcmp(lexem.str, "sources"))
        parse_error(lexem, "'sources' expected\n");
    next(true);
    if(lexem.type != LEX_LBRACE)
        parse_error(lexem, "'{' expected after 'sources'\n");

    while(true)
    {
        next(true);
        if(lexem.type == LEX_RBRACE)
            break;
        if(lexem.type != LEX_IDENTIFIER)
            parse_error(lexem, "identifier expected in sources\n");
        const char *srcid = lexem.str;
        if(db_find_source_by_id(cmd_file, srcid) != NULL)
            parse_error(lexem, "Duplicate source identifier\n");
        next(false); /* lexem string is kept as source name */
        if(lexem.type != LEX_EQUAL)
            parse_error(lexem, "'=' expected after identifier\n");
        next(true);
        if(lexem.type == LEX_STRING)
        {
            db_add_source(cmd_file, srcid, lexem.str);
            next(true);
        }
        else if(lexem.type == LEX_IDENTIFIER && !strcmp(lexem.str, "extern"))
        {
            next(true);
            if(lexem.type != LEX_LPAREN)
                parse_error(lexem, "'(' expected after 'extern'\n");
            next(true);
            db_add_extern_source(cmd_file, srcid, parse_intexpr(&lctx, cmd_file->constant_list));
            if(lexem.type != LEX_RPAREN)
                parse_error(lexem, "')' expected\n");
            next(true);
        }
        else
            parse_error(lexem, "String or 'extern' expected after '='\n");
        if(lexem.type != LEX_SEMICOLON)
            parse_error(lexem, "';' expected\n");
    }

    /* sections */
    while(true)
    {
        next(true);
        if(lexem.type == LEX_EOF)
            break;
        if(lexem.type != LEX_IDENTIFIER || strcmp(lexem.str, "section") != 0)
            parse_error(lexem, "'section' expected\n");
        next(true);
        if(lexem.type != LEX_LPAREN)
            parse_error(lexem, "'(' expected after 'section'\n");
        next(true);
        /* can be any number */
        struct cmd_section_t *sec = db_add_section(cmd_file, parse_intexpr(&lctx, cmd_file->constant_list), false);
        /* options ? */
        if(lexem.type == LEX_SEMICOLON)
        {
            do
            {
                next(true);
                if(lexem.type != LEX_IDENTIFIER)
                    parse_error(lexem, "Identifier expected for section option\n");
                const char *name = lexem.str;
                next(false); /* lexem string is kept as option name */
                if(lexem.type != LEX_EQUAL)
                    parse_error(lexem, "'=' expected after option identifier\n");
                next(true);
                if(lexem.type == LEX_STRING)
                {
                    db_add_str_opt(&sec->opt_list, name, lexem.str);
                    next(true);
                }
                else
                    db_add_int_opt(&sec->opt_list, name, parse_intexpr(&lctx, cmd_file->constant_list));
            }while(lexem.type == LEX_COLON);
        }
        if(lexem.type != LEX_RPAREN)
            parse_error(lexem, "')' expected after section identifier\n");
        next(true);
        if(lexem.type == LEX_LBRACE)
        {
            sec->is_data = false;
            /* commands */
            while(true)
            {
                next(true);
                if(lexem.type == LEX_RBRACE)
                    break;
                struct cmd_inst_t *inst = db_add_inst(&sec->inst_list, CMD_LOAD, 0);
                if(lexem.type != LEX_IDENTIFIER)
                    parse_error(lexem, "Instruction expected in section\n");
                if(strcmp(lexem.str, "load") == 0)
                    inst->type = CMD_LOAD;
                else if(strcmp(lexem.str, "call") == 0)
                    inst->type = CMD_CALL;
                else if(strcmp(lexem.str, "jump") == 0)
                    inst->type = CMD_JUMP;
                else if(strcmp(lexem.str, "mode") == 0)
                    inst->type = CMD_MODE;
                else
                    parse_error(lexem, "Instruction expected in section\n");
                next(true);

                if(inst->type == CMD_LOAD)
                {
                    if(lexem.type != LEX_IDENTIFIER)
                        parse_error(lexem, "Identifier expected after instruction\n");
                    inst->identifier = lexem.str;
                    if(db_find_source_by_id(cmd_file, inst->identifier) == NULL)
                        parse_error(lexem, "Undefined reference to source '%s'\n", inst->identifier);
                    next(false); /* lexem string kept as identifier  */
                    if(lexem.type == LEX_RANGLE)
                    {
                        // load at
                        inst->type = CMD_LOAD_AT;
                        next(true);
                        inst->addr = parse_intexpr(&lctx, cmd_file->constant_list);
                    }
                    if(lexem.type != LEX_SEMICOLON)
                        parse_error(lexem, "';' expected after command\n");
                }
                else if(inst->type == CMD_CALL || inst->type == CMD_JUMP)
                {
                    if(lexem.type == LEX_IDENTIFIER)
                    {
                        inst->identifier = lexem.str;
                        if(db_find_source_by_id(cmd_file, inst->identifier) == NULL)
                            parse_error(lexem, "Undefined reference to source '%s'\n", inst->identifier);
                        next(false); /* lexem string kept as identifier  */
                    }
                    else
                    {
                        inst->type = (inst->type == CMD_CALL) ? CMD_CALL_AT : CMD_JUMP_AT;
                        inst->addr = parse_intexpr(&lctx, cmd_file->constant_list);
                    }

                    if(lexem.type == LEX_LPAREN)
                    {
                        next(true);
                        inst->argument = parse_intexpr(&lctx, cmd_file->constant_list);
                        if(lexem.type != LEX_RPAREN)
                            parse_error(lexem, "Expected closing brace\n");
                        next(true);
                    }
                    if(lexem.type != LEX_SEMICOLON)
                        parse_error(lexem, "';' expected after command\n");
                }
                else if(inst->type == CMD_MODE)
                {
                    inst->argument = parse_intexpr(&lctx, cmd_file->constant_list);
                    if(lexem.type != LEX_SEMICOLON)
                        parse_error(lexem, "Expected ';' after command\n");
                }
                else
                    parse_error(lexem, "Internal error");
            }
        }
        else if(lexem.type == LEX_LE)
        {
            sec->is_data = true;
            next(true);
            if(lexem.type != LEX_IDENTIFIER)
                parse_error(lexem, "Identifier expected after '<='\n");
            sec->source_id = lexem.str;
            next(false); /* lexem string is kept as source id */
            if(lexem.type != LEX_SEMICOLON)
                parse_error(lexem, "';' expected after identifier\n");
        }
        else
            parse_error(lexem, "'{' or '<=' expected after section directive\n");
    }
    #undef lexem
    #undef next

    free(buf);
    return cmd_file;
}

void db_free_option_list(struct cmd_option_t *opt_list)
{
    while(opt_list)
    {
        struct cmd_option_t *next = opt_list->next;
        fflush(stdout);
        free(opt_list->name);
        free(opt_list->str);
        free(opt_list);
        opt_list = next;
    }
}

static bool db_generate_options(FILE *f, const char *secname, struct cmd_option_t *list)
{
    fprintf(f, "%s\n", secname);
    fprintf(f, "{\n");
    while(list)
    {
        fprintf(f, "    %s = ", list->name);
        if(list->is_string)
            fprintf(f, "\"%s\";\n", list->str); // FIXME handle escape
        else
            fprintf(f, "0x%x;\n", list->val);
        list = list->next;
    }
    fprintf(f, "}\n");
    return true;
}

static bool db_generate_section_options(FILE *f, struct cmd_option_t *list)
{
    bool first = true;
    while(list)
    {
        fprintf(f, "%c %s = ", first ? ';' : ',', list->name);
        if(list->is_string)
            fprintf(f, "\"%s\"", list->str); // FIXME handle escape
        else
            fprintf(f, "0x%x", list->val);
        first = false;
        list = list->next;
    }
    return true;
}

static bool db_generate_sources(FILE *f, struct cmd_source_t *list)
{
    fprintf(f, "sources\n"),
    fprintf(f, "{\n");
    while(list)
    {
        fprintf(f, "    %s = ", list->identifier);
        if(list->is_extern)
            fprintf(f, "extern(%d);\n", list->extern_nr);
        else
            fprintf(f, "\"%s\";\n", list->filename); // FIXME handle escape
        list = list->next;
    }
    fprintf(f, "}\n");
    return true;
}

static bool db_generate_section(FILE *f, struct cmd_section_t *section)
{
    fprintf(f, "section(%#x", section->identifier);
    db_generate_section_options(f, section->opt_list);
    if(section->is_data)
    {
        fprintf(f, ") <= %s;\n", section->source_id);
        return true;
    }
    fprintf(f, ")\n{\n");
    struct cmd_inst_t *inst = section->inst_list;
    while(inst)
    {
        fprintf(f, "    ");
        switch(inst->type)
        {
            case CMD_LOAD:
                fprintf(f, "load %s;\n", inst->identifier);
                break;
            case CMD_LOAD_AT:
                fprintf(f, "load %s > %#x;\n", inst->identifier, inst->addr);
                break;
            case CMD_CALL:
                fprintf(f, "call %s(%#x);\n", inst->identifier, inst->argument);
                break;
            case CMD_CALL_AT:
                fprintf(f, "call %#x(%#x);\n", inst->addr, inst->argument);
                break;
            case CMD_JUMP:
                fprintf(f, "jump %s(%#x);\n", inst->identifier, inst->argument);
                break;
            case CMD_JUMP_AT:
                fprintf(f, "jump %#x(%#x);\n", inst->addr, inst->argument);
                break;
            case CMD_MODE:
                fprintf(f, "mode %#x;\n", inst->argument);
                break;
            default:
                bug("die");
        }
        inst = inst->next;
    }
    fprintf(f, "}\n");
    return true;
}

static bool db_generate_sections(FILE *f, struct cmd_section_t *section)
{
    while(section)
        if(!db_generate_section(f, section))
            return false;
        else
            section = section->next;
    return true;
}

bool db_generate_file(struct cmd_file_t *file, const char *filename, void *user, db_color_printf printf)
{
    FILE *f = fopen(filename, "w");
    if(f == NULL)
        return printf(user, true, GREY, "Cannot open '%s' for writing: %m\n", filename), false;
    if(!db_generate_options(f, "constants", file->constant_list))
        goto Lerr;
    if(!db_generate_options(f, "options", file->opt_list))
        goto Lerr;
    if(!db_generate_sources(f, file->source_list))
        goto Lerr;
    if(!db_generate_sections(f, file->section_list))
        goto Lerr;

    fclose(f);
    return true;

    Lerr:
    fclose(f);
    return false;
}

void db_free(struct cmd_file_t *file)
{
    db_free_option_list(file->opt_list);
    db_free_option_list(file->constant_list);
    struct cmd_source_t *src = file->source_list;
    while(src)
    {
        struct cmd_source_t *next = src->next;
        free(src->identifier);
        fflush(stdout);
        free(src->filename);
        if(src->loaded)
        {
            if(src->type == CMD_SRC_BIN)
                free(src->bin.data);
            if(src->type == CMD_SRC_ELF)
                elf_release(&src->elf);
        }
        free(src);
        src = next;
    }
    struct cmd_section_t *sec = file->section_list;
    while(sec)
    {
        struct cmd_section_t *next = sec->next;
        db_free_option_list(sec->opt_list);
        free(sec->source_id);
        struct cmd_inst_t *inst = sec->inst_list;
        while(inst)
        {
            struct cmd_inst_t *next = inst->next;
            free(inst->identifier);
            free(inst);
            inst = next;
        }
        free(sec);
        sec = next;
    }
    free(file);
}
