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
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *type_to_str(enum type type)
{
    switch (type)
    {
    case TYPE_REG:   return "reg";
    case TYPE_ENUM:  return "enum";
    case TYPE_BLOCK: return "block";
    default:         return NULL;
    }
}

char *vformat_string(const char *fmt, va_list ap_in)
{
    char *buf = NULL;
    size_t buflen = 0;
    for (;;)
    {
        int ret;
        va_list ap;
        va_copy(ap, ap_in);
        ret = vsnprintf(buf, buflen, fmt, ap);
        va_end(ap);

        if (ret < 0)
            return NULL;

        if ((size_t)ret >= buflen)
        {
            buflen = ret + 1;
            buf = realloc(buf, buflen);
            continue;
        }

        return buf;
    }
}

char *format_string(const char *fmt, ...)
{
    char *buf;
    va_list ap;
    va_start(ap, fmt);
    buf = vformat_string(fmt, ap);
    va_end(ap);

    return buf;
}

char *format_source_loc(const struct source_loc *loc)
{
    if (loc == NULL)
        return NULL;

    if (loc->filename == NULL)
        return NULL;

    return format_string("%s:%d:%d", loc->filename, loc->line, loc->column);
}

void context_verror(struct context *ctx,
                    const struct source_loc *loc,
                    const char *msg, va_list ap_in)
{
    char *loc_str = format_source_loc(loc);
    if (loc_str)
    {
        fprintf(stderr, "%s: ", loc_str);
        free(loc_str);
    }

    va_list ap;
    va_copy(ap, ap_in);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    ctx->num_errors++;
}

void context_error(struct context *ctx,
                   const struct source_loc *loc,
                   const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    context_verror(ctx, loc, msg, ap);
    va_end(ap);
}

void context_free(struct context *ctx)
{
    struct type_member *member;
    ARRAY_FOREACH(&ctx->memblist, member)
        free(member);

    struct type_def *tdef;
    ARRAY_FOREACH(&ctx->typelist, tdef)
    {
        hashmap_free(&tdef->members);
        array_free(&tdef->members_sorted);
        free(tdef);
    }

    char *str;
    HASHMAP_FOREACH(&ctx->strtab, str)
        free(str);

    hashmap_free(&ctx->types);
    hashmap_free(&ctx->strtab);
    array_free(&ctx->memblist);
    array_free(&ctx->typelist);
    array_free(&ctx->root_instances);
}

/* Intern a static string; creates a duplicate of the input string */
const char *intern_static_string(struct context *ctx, const char *str)
{
    char *istr = hashmap_lookup(&ctx->strtab, str);
    if (istr)
        return istr;

    istr = strdup(str);
    hashmap_insert(&ctx->strtab, istr, istr);
    return istr;
}

/*
 * Intern a dynamically allocated string; frees the input string
 * or transfers ownership to the context
 */
const char *intern_string(struct context *ctx, char *str)
{
    char *istr = hashmap_lookup(&ctx->strtab, str);
    if (istr)
    {
        free(str);
        return istr;
    }

    hashmap_insert(&ctx->strtab, str, str);
    return str;
}

/* Intern a string generated from a format string */
const char *intern_stringf(struct context *ctx, const char *fmt, ...)
{
    char *buf;
    va_list ap;
    va_start(ap, fmt);
    buf = vformat_string(fmt, ap);
    va_end(ap);

    return intern_string(ctx, buf);
}

/* Construct a new type definition */
struct type_def *type_def_init(struct context *ctx, const char *name,
                               enum type type, enum word_width width)
{
    struct type_def *tdef = calloc(1, sizeof(*tdef));

    tdef->name = name;
    tdef->type = type;
    tdef->width = width;

    array_push(&ctx->typelist, tdef);
    return tdef;
}

/* Map symbolic word types to real types for the machine */
enum word_width map_word_type(struct context *ctx, enum word_width width)
{
    if (width == WIDTH_UNSPECIFIED)
        return ctx->config.machine_word_width;
    if (width == WIDTH_ADDRESS)
        return map_word_type(ctx, ctx->config.address_word_width);

    return width;
}

/* Get number of bits in word type */
int get_word_bits(struct context *ctx, enum word_width width)
{
    switch (map_word_type(ctx, width))
    {
    case WIDTH_8:  return 8;
    case WIDTH_16: return 16;
    case WIDTH_32: return 32;
    case WIDTH_64: return 64;
    default:       return 0;
    }
}

/* Return suffix for an unsigned integer literal */
const char *get_word_literal_suffix(struct context *ctx, enum word_width width)
{
    switch (map_word_type(ctx, width))
    {
    case WIDTH_8:  return "u";
    case WIDTH_16: return "u";
    case WIDTH_32: return "ul";
    case WIDTH_64: return "ull";
    default:       return "";
    }
}
