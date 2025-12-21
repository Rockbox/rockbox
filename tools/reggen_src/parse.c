/*
 * This file is part of RegGen -- register definition generator
 * Copyright (C) 2025 Aidan MacDonald
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "parse.h"
#include "common.h"
#include <string.h>
#include <stdlib.h>

#define IDENT_MAXLEN   4096
#define NUMBER_MAXLEN  128

struct parser
{
    struct context *ctx;
    struct array type_scope;

    FILE *input;
    struct source_loc loc;
};

enum inline_type_style
{
    INLINE_KEYWORD,
    INLINE_BLOCK,
};

#define TYPEBIT_ENUM  (1 << TYPE_ENUM)
#define TYPEBIT_REG   (1 << TYPE_REG)
#define TYPEBIT_BLOCK (1 << TYPE_BLOCK)

#define IS_PARSE_OK(s)    ((s) > 0)
#define IS_PARSE_EMPTY(s) ((s) == 0)
#define IS_PARSE_ERR(s)   ((s) < 0)

static const char KEYWORD_INCLUDE[] = "include";

static int parse_type_def(struct parser *p, const char *keyword);
static int parse_type_def_ex(struct parser *p,
                             enum type type,
                             enum word_width width,
                             const char *inline_name,
                             struct type_def **tdefp);
static int parse_inline_type_def(struct parser *p,
                                 const char *inline_name,
                                 enum inline_type_style style,
                                 enum type type,
                                 enum word_width width,
                                 struct type_def **tdefp);

static bool parse_lookup_type_keyword(const char *keyword,
                                      enum type *typep,
                                      enum word_width *rwidthp)
{
    struct keyword_entry {
        const char *keyword;
        enum type type;
        enum word_width width;
    };

    static const struct keyword_entry keyword_mapping[] = {
        {"enum",  TYPE_ENUM,  WIDTH_UNSPECIFIED},
        {"block", TYPE_BLOCK, WIDTH_UNSPECIFIED},
        {"reg",   TYPE_REG,   WIDTH_UNSPECIFIED},
        {"reg8",  TYPE_REG,   WIDTH_8},
        {"reg16", TYPE_REG,   WIDTH_16},
        {"reg32", TYPE_REG,   WIDTH_32},
        {"reg64", TYPE_REG,   WIDTH_64},
    };

    size_t len = sizeof(keyword_mapping) / sizeof(*keyword_mapping);
    for (size_t i = 0; i < len; ++i)
    {
        if (!strcmp(keyword, keyword_mapping[i].keyword))
        {
            if (rwidthp)
                *rwidthp = keyword_mapping[i].width;
            if (typep)
                *typep = keyword_mapping[i].type;

            return true;
        }
    }

    return false;
}

int parse_error(struct parser *p, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    context_verror(p->ctx, &p->loc, msg, ap);
    va_end(ap);

    return PARSE_ERR;
}

const char *parse_get_scoped_type_name(struct parser *p, const char *last, size_t nskip)
{
    char fqn[IDENT_MAXLEN];
    size_t len = 0;

    if (last)
    {
        array_push(&p->type_scope, (void *)last);
        ++nskip;
    }

    for (size_t index = 0; index < array_size(&p->type_scope); ++index)
    {
        if (index >= array_size(&p->type_scope) - nskip &&
            index < array_size(&p->type_scope) - 1)
        {
            continue;
        }

        const char *sn = array_get(&p->type_scope, index);
        size_t slen = strlen(sn);
        if (len + slen + 1 >= IDENT_MAXLEN)
        {
            parse_error(p, "qualified type name too long");
            return NULL;
        }

        if (len != 0)
            fqn[len++] = '_';

        memcpy(&fqn[len], sn, slen);
        len += slen;
    }

    if (last)
        array_pop(&p->type_scope);

    fqn[len] = 0;
    return intern_static_string(p->ctx, fqn);
}

struct type_def *parse_lookup_type(struct parser *p, const char *name, size_t init_skip)
{
    size_t max_skip = array_size(&p->type_scope);
    for (size_t skip = init_skip; skip <= max_skip; ++skip)
    {
        const char *qname = parse_get_scoped_type_name(p, name, skip);
        if (!qname)
            return NULL;

        struct type_def *type = hashmap_lookup(&p->ctx->types, qname);
        if (type)
            return type;
    }

    return NULL;
}

int parse_whitespace(struct parser *p)
{
    bool in_comment = false;
    bool in_multiline_comment = false;

    for (;;)
    {
        int c = fgetc(p->input);
        p->loc.column++;

        if (c == '\n')
        {
            if (!in_multiline_comment)
                in_comment = false;

            p->loc.line++;
            p->loc.column = 0;
        }

        if (c == '/' && !in_comment)
        {
            c = fgetc(p->input);
            p->loc.column++;

            if (c == '*')
                in_multiline_comment = true;
            else if (c != '/')
                return parse_error(p, "invalid comment");

            in_comment = true;
        }

        if (c == '*' && in_multiline_comment)
        {
            c = fgetc(p->input);
            p->loc.column++;

            if (c == '/')
            {
                c = fgetc(p->input);
                p->loc.column++;

                in_multiline_comment = false;
                in_comment = false;
            }
        }

        if (c == EOF)
        {
            if (in_multiline_comment)
                return parse_error(p, "unterminated comment");

            in_comment = false;
        }

        if (!in_comment &&
            c != ' ' && c != '\t' &&
            c != '\n' && c != '\r')
        {
            ungetc(c, p->input);
            p->loc.column--;

            return PARSE_OK;
        }
    }
}

const char *parse_identifier_ex(struct parser *p, bool allow_first_digit)
{
    char ident[IDENT_MAXLEN];
    size_t idlen = 0;
    int c;

    if (parse_whitespace(p) != PARSE_OK)
        return NULL;

    for (;;)
    {
        c = fgetc(p->input);
        p->loc.column++;

        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == '_'))
        {
            ident[idlen++] = c;
        }
        else if (c >= '0' && c <= '9' && (idlen > 0 || allow_first_digit))
        {
            ident[idlen++] = c;
        }
        else
        {
            break;
        }

        if (idlen == IDENT_MAXLEN)
        {
            parse_error(p, "identifier too long");
            return NULL;
        }
    }

    ungetc(c, p->input);
    p->loc.column--;

    if (idlen == 0)
        return NULL;

    ident[idlen] = 0;
    return intern_static_string(p->ctx, ident);
}

const char *parse_identifier(struct parser *p)
{
    return parse_identifier_ex(p, false);
}

int parse_literal_optional(struct parser *p, const char *str)
{
    if (parse_whitespace(p) != PARSE_OK)
        return PARSE_ERR;

    for (size_t index = 0; str[index] != 0; ++index)
    {
        int c = fgetc(p->input);
        p->loc.column++;

        if (c != str[index])
        {
            if (index > 0)
                return parse_error(p, "expected '%s'", str);

            ungetc(c, p->input);
            p->loc.column--;

            return PARSE_EMPTY;
        }
    }

    return PARSE_OK;

}

int parse_literal(struct parser *p, const char *str)
{
    int status = parse_literal_optional(p, str);
    if (status == PARSE_EMPTY)
        return parse_error(p, "missing '%s'", str);

    return status;
}

int parse_number(struct parser *p, uint64_t *val)
{
    enum {
        DEC_NUM,
        HEX_NUM,
        BIN_NUM,
    };

    char buf[NUMBER_MAXLEN];
    size_t len = 0;
    int mode = DEC_NUM;

    if (parse_whitespace(p) != PARSE_OK)
        return PARSE_ERR;

    for (;;)
    {
        bool valid = false;
        int c = fgetc(p->input);
        p->loc.column++;

        if (c >= '0' && c <= '9')
        {
            if (c <= '1' || mode != BIN_NUM)
                valid = true;
        }
        else if (mode == HEX_NUM)
        {
            if (c >= 'a' && c <= 'f')
                valid = true;
            if (c >= 'A' && c <= 'F')
                valid = true;
        }

        if (c == '_' && len > 0)
            valid = true;

        if (!valid)
        {
            if (len == 1 && buf[0] == '0')
            {
                if (c == 'x' || c == 'X')
                {
                    mode = HEX_NUM;
                    len = 0;
                    continue;
                }
                else if (c == 'b' || c == 'B')
                {
                    mode = BIN_NUM;
                    len = 0;
                    continue;
                }
            }

            if (len > 0)
            {
                /* detect silly errors like 0b0123, etc. */
                if ((c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z'))
                    return parse_error(p, "invalid number");
            }

            /* parsed last valid character */
            ungetc(c, p->input);
            p->loc.column--;
            break;
        }

        buf[len++] = c;
        if (len == NUMBER_MAXLEN)
            return parse_error(p, "number too long");
    }

    if (len == 0)
    {
        if (mode != DEC_NUM)
            return parse_error(p, "invalid number");

        return PARSE_EMPTY;
    }

    uint64_t rval = 0;
    for (size_t i = 0; i < len; ++i)
    {
        if (buf[i] == '_')
            continue;

        switch (mode)
        {
        case BIN_NUM:
            rval *= 2;
            break;
        case DEC_NUM:
            rval *= 10;
            break;
        case HEX_NUM:
            rval *= 16;
            break;
        }

        if (buf[i] >= '0' && buf[i] <= '9')
            rval += buf[i] - '0';
        else if (buf[i] >= 'a' && buf[i] <= 'f')
            rval += 10 + buf[i] - 'a';
        else if (buf[i] >= 'A' && buf[i] <= 'F')
            rval += 10 + buf[i] - 'A';
    }

    if (val)
        *val = rval;

    return PARSE_OK;
}

int parse_member_type(struct parser *p,
                      struct type_def **tdef_out,
                      const char *inline_name,
                      int allowed_types)
{
    if (parse_literal_optional(p, ":") == PARSE_EMPTY)
    {
        *tdef_out = NULL;

        parse_literal_optional(p, ";");
        return PARSE_EMPTY;
    }

    struct type_def *tdef = NULL;
    bool is_inline_type = false;
    enum type inline_type;
    enum word_width inline_width;
    enum inline_type_style inline_style;
    int status = 0;

    const char *ident = parse_identifier(p);
    if (!ident)
    {
        if (parse_literal(p, "{") != PARSE_OK)
            return PARSE_ERR;
        if (!inline_name)
            return parse_error(p, "internal error: missing default inline type name");

        if (allowed_types == TYPEBIT_ENUM)
            inline_type = TYPE_ENUM;
        else if (allowed_types == TYPEBIT_REG)
            inline_type = TYPE_REG;
        else if (allowed_types == TYPEBIT_BLOCK)
            inline_type = TYPE_BLOCK;
        else
            return parse_error(p, "type keyword required for inline defintion here");

        is_inline_type = true;
        inline_style = INLINE_BLOCK;
        inline_width = WIDTH_UNSPECIFIED;
    }
    else if (parse_lookup_type_keyword(ident, &inline_type, &inline_width))
    {
        if ((allowed_types & (1 << inline_type)) == 0)
        {
            return parse_error(p, "inline %s not allowed here",
                               type_to_str(inline_type));
        }

        is_inline_type = true;
        inline_style = INLINE_KEYWORD;
    }

    if (is_inline_type)
    {
        status = parse_inline_type_def(p, inline_name, inline_style,
                                       inline_type, inline_width, &tdef);
        if (IS_PARSE_ERR(status))
            return status;
    }
    else
    {
        tdef = parse_lookup_type(p, ident, 0);
        if (tdef == NULL)
            return parse_error(p, "unknown type '%s'", ident);
        else if ((allowed_types & (1 << tdef->type)) == 0)
            return parse_error(p, "%s type not allowed here", type_to_str(tdef->type));
    }

    *tdef_out = tdef;

    parse_literal_optional(p, ";");
    return PARSE_OK;
}

int parse_include(struct parser *p, struct type_def *tdef)
{
    const char *type = parse_identifier(p);
    if (!type)
    {
        return parse_error(p, "missing %s type name for include",
                           type_to_str(tdef->type));
    }

    struct type_def *itdef = parse_lookup_type(p, type, 1);
    if (!itdef)
        return parse_error(p, "unknown type '%s'", type);
    if (itdef->type != tdef->type)
    {
        return parse_error(p, "cannot include %s type '%s' from %s",
                           type_to_str(itdef->type), itdef->name,
                           type_to_str(tdef->type));
    }

    struct type_member *memb;
    HASHMAP_FOREACH(&itdef->members, memb)
    {
        if (hashmap_insert(&tdef->members, memb->name, memb))
        {
            return parse_error(p, "duplicate member '%s' from included %s %s",
                               memb->name, type_to_str(tdef->type), itdef->name);
        }
    }

    parse_literal_optional(p, ";");
    return PARSE_OK;
}

int parse_enum(struct parser *p, struct type_def *tdef)
{
    for (;;)
    {
        int status;
        const char *keyword = parse_identifier(p);
        if (keyword)
        {
            if (strcmp(keyword, KEYWORD_INCLUDE))
                return parse_error(p, "invalid syntax");

            status = parse_include(p, tdef);
            if (status != PARSE_OK)
                return status;

            continue;
        }

        uint64_t value;
        status = parse_number(p, &value);
        if (IS_PARSE_ERR(status))
            return status;
        if (IS_PARSE_EMPTY(status))
            return PARSE_OK;

        if (parse_literal(p, "=") != PARSE_OK)
            return PARSE_ERR;

        const char *valname = parse_identifier_ex(p, true);
        if (!valname)
            return parse_error(p, "missing name for enum value");

        parse_literal_optional(p, ";");

        struct enum_value *eval = calloc(1, sizeof(*eval));
        array_push(&p->ctx->memblist, eval);

        eval->comm.name = valname;
        eval->comm.loc = p->loc;
        eval->value = value;

        if (hashmap_insert(&tdef->members, valname, eval))
            return parse_error(p, "duplicate enum value '%s'", valname);
    }
}

int parse_reg_field(struct parser *p, struct type_def *tdef)
{
    uint64_t num;
    int msb = -1, lsb = -1;
    int status = parse_number(p, &num);
    if (IS_PARSE_OK(status))
        msb = num;
    else if (IS_PARSE_ERR(status))
        return status;
    else if (IS_PARSE_EMPTY(status))
    {
        if (parse_literal_optional(p, "-") == PARSE_EMPTY)
            return PARSE_EMPTY;

        if (parse_literal_optional(p, "-") == PARSE_ERR)
            return PARSE_ERR;
    }

    status = parse_number(p, &num);
    if (IS_PARSE_OK(status))
        lsb = num;
    else if (IS_PARSE_ERR(status))
        return status;
    else if (IS_PARSE_EMPTY(status))
    {
        if (parse_literal_optional(p, "-") == PARSE_ERR)
            return PARSE_ERR;

        if (parse_literal_optional(p, "-") == PARSE_ERR)
            return PARSE_ERR;
    }

    if (msb < 0 && lsb < 0)
        return parse_error(p, "no bits specified for register field");
    else if (msb < 0)
        msb = lsb;
    else if (lsb < 0)
        lsb = msb;

    const char *fieldname = parse_identifier_ex(p, true);
    if (!fieldname)
        return parse_error(p, "expected field name");

    struct reg_field *field = calloc(1, sizeof(*field));
    array_push(&p->ctx->memblist, field);

    field->comm.name = fieldname;
    field->comm.loc = p->loc;
    field->msb = msb;
    field->lsb = lsb;

    status = parse_member_type(p, &field->comm.type, fieldname, TYPEBIT_ENUM);
    if (status < 0)
        return status;

    if (hashmap_insert(&tdef->members, fieldname, field))
        return parse_error(p, "duplicate register field '%s'", fieldname);

    return 1;
}

int parse_reg(struct parser *p, struct type_def *tdef)
{
    for (;;)
    {
        int status = parse_reg_field(p, tdef);
        if (IS_PARSE_ERR(status))
            return status;
        if (IS_PARSE_OK(status))
            continue;

        const char *keyword = parse_identifier(p);
        if (!keyword)
            return PARSE_OK;

        if (!strcmp(keyword, KEYWORD_INCLUDE))
        {
            status = parse_include(p, tdef);
            if (status != PARSE_OK)
                return status;

            continue;
        }

        /* otherwise try to parse nested types */
        status = parse_type_def(p, keyword);
        if (IS_PARSE_OK(status))
            continue;

        return status;
    }
}

int parse_instance_data(struct parser *p, const char *instname, struct instance **instancep)
{
    uint64_t offset;
    int status = parse_number(p, &offset);
    if (IS_PARSE_ERR(status))
        return status;
    if (IS_PARSE_EMPTY(status))
        return parse_error(p, "missing offset for '%s'", instname);

    uint64_t count = 1;
    uint64_t stride = 0;
    if (parse_literal_optional(p, "[") == PARSE_OK)
    {
        status = parse_number(p, &count);
        if (IS_PARSE_ERR(status))
            return status;
        if (IS_PARSE_EMPTY(status))
            return parse_error(p, "missing instance count");
        if (count == 0)
            return parse_error(p, "instance count cannot be 0");

        if (parse_literal(p, ";") != PARSE_OK)
            return PARSE_ERR;

        status = parse_number(p, &stride);
        if (IS_PARSE_ERR(status))
            return status;
        if (IS_PARSE_EMPTY(status))
            return parse_error(p, "missing stride");
        if (stride == 0)
            return parse_error(p, "stride cannot be 0");

        if (parse_literal(p, "]") != PARSE_OK)
            return PARSE_ERR;
    }

    struct instance *instance = calloc(1, sizeof(*instance));
    array_push(&p->ctx->memblist, instance);

    instance->comm.name = instname;
    instance->comm.loc = p->loc;
    instance->offset = offset;
    instance->stride = stride;
    instance->count = count;

    status = parse_member_type(p, &instance->comm.type, instname,
                               TYPEBIT_REG | TYPEBIT_BLOCK);
    if (IS_PARSE_ERR(status))
        return status;
    if (IS_PARSE_EMPTY(status))
        return parse_error(p, "missing type for '%s'", instname);

    *instancep = instance;
    return PARSE_OK;
}

static int parse_block(struct parser *p, struct type_def *tdef)
{
    for (;;)
    {
        int status;
        const char *instname = parse_identifier_ex(p, true);
        if (!instname)
            return PARSE_OK;

        if (!strcmp(instname, KEYWORD_INCLUDE))
        {
            status = parse_include(p, tdef);
            if (status != PARSE_OK)
                return status;

            continue;
        }

        if (parse_literal_optional(p, "@") == PARSE_EMPTY)
        {
            if (parse_type_def(p, instname) != PARSE_OK)
                return PARSE_ERR;
        }
        else
        {
            struct instance *instance;
            if (parse_instance_data(p, instname, &instance) != PARSE_OK)
                return PARSE_ERR;

            if (hashmap_insert(&tdef->members, instname, instance))
                return parse_error(p, "duplicate instance definition");
        }
    }
}

int parse_type_def_ex(struct parser *p,
                      enum type type,
                      enum word_width width,
                      const char *inline_name,
                      struct type_def **tdefp)
{
    int status;
    const char *localname = inline_name;
    if (!localname)
        localname = parse_identifier(p);
    if (!localname)
        return parse_error(p, "%s definition missing name", type_to_str(type));

    array_push(&p->type_scope, (void *)localname);

    const char *tname = parse_get_scoped_type_name(p, NULL, 0);
    if (!tname)
        return PARSE_ERR;

    struct type_def *tdef = type_def_init(p->ctx, tname, type, width);
    tdef->loc = p->loc;

    if (!inline_name && parse_literal(p, "{") != PARSE_OK)
        return PARSE_ERR;

    if (type == TYPE_ENUM)
        status = parse_enum(p, tdef);
    else if (type == TYPE_REG)
        status = parse_reg(p, tdef);
    else if (type == TYPE_BLOCK)
        status = parse_block(p, tdef);
    else
        status = parse_error(p, "internal error: bad type");

    if (status != PARSE_OK)
        return PARSE_ERR;

    if (parse_literal(p, "}") != PARSE_OK)
        return PARSE_ERR;

    if (hashmap_insert(&p->ctx->types, tdef->name, tdef))
        return parse_error(p, "redefinition of type '%s'", tdef->name);

    if (tdefp)
        *tdefp = tdef;

    array_pop(&p->type_scope);
    return PARSE_OK;
}

struct type_def *make_anonymous_reg(struct parser *p, enum word_width width)
{
    width = map_word_type(p->ctx, width);

    const char *regname = intern_stringf(p->ctx, "$anon%d", (int)width);
    struct type_def *tdef = hashmap_lookup(&p->ctx->types, regname);
    if (!tdef)
    {
        tdef = type_def_init(p->ctx, regname, TYPE_REG, width);
        hashmap_insert(&p->ctx->types, tdef->name, tdef);
    }

    return tdef;
}

int parse_inline_type_def(struct parser *p,
                          const char *inline_name,
                          enum inline_type_style style,
                          enum type type,
                          enum word_width width,
                          struct type_def **tdefp)
{
    if (style == INLINE_KEYWORD)
    {
        if (parse_literal_optional(p, "{") == PARSE_EMPTY)
        {
            /* reject bare keyword unless this is a register */
            if (type != TYPE_REG)
                return parse_error(p, "missing '{'");

            /* anonymous registers with no fields are allowed */
            *tdefp = make_anonymous_reg(p, width);
            return 0;
        }
    }

    return parse_type_def_ex(p, type, width, inline_name, tdefp);
}

int parse_type_def(struct parser *p, const char *keyword)
{
    enum type type;
    enum word_width width;
    if (parse_lookup_type_keyword(keyword, &type, &width))
        return parse_type_def_ex(p, type, width, NULL, NULL);

    return parse_error(p, "invalid type '%s'", keyword);
}

int parse_root_instance(struct parser *p, const char *instname)
{
    if (parse_literal(p, "@") != PARSE_OK)
        return PARSE_ERR;

    struct instance *instance;
    if (parse_instance_data(p, instname, &instance) != PARSE_OK)
        return PARSE_ERR;

    array_push(&p->ctx->root_instances, instance);
    return PARSE_OK;
}

int parse(struct context *ctx, const char *filename, FILE *input)
{
    int rc = PARSE_EMPTY;
    struct parser p = {
        .ctx = ctx,
        .input = input,
        .loc = {
            .filename = filename,
            .line = 1,
            .column = 0,
        },
    };

    for (;;)
    {
        const char *ident = parse_identifier(&p);
        if (!ident)
        {
            if (!feof(input))
            {
                rc = parse_error(&p, "unexpected character '%c'", fgetc(input));
                break;
            }

            rc = PARSE_OK;
            break;
        }

        if (parse_lookup_type_keyword(ident, NULL, NULL))
        {
            rc = parse_type_def(&p, ident);
            if (IS_PARSE_ERR(rc))
                break;
        }
        else
        {
            rc = parse_root_instance(&p, ident);
            if (IS_PARSE_ERR(rc))
                break;
        }
    }

    array_free(&p.type_scope);
    return rc;
}
