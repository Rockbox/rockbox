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
#ifndef REGGEN_COMMON_H
#define REGGEN_COMMON_H

#include "array.h"
#include "hashmap.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Common types
 */
enum word_width
{
    WIDTH_UNSPECIFIED,
    WIDTH_ADDRESS,
    WIDTH_8,
    WIDTH_16,
    WIDTH_32,
    WIDTH_64,
};

enum type
{
    TYPE_ENUM,
    TYPE_REG,
    TYPE_BLOCK,
};

struct type_def;

/* Reference to a source file location */
struct source_loc
{
    const char *filename;
    int line;
    int column;
};

/* Common data for type members */
struct type_member
{
    /* Name of member */
    const char *name;

    /* Type assigned to this member */
    struct type_def *type;

    /* Source location where member was defined */
    struct source_loc loc;
};

/* Data for enumerators */
struct enum_value
{
    struct type_member comm;
    uint64_t value;
};

/* Data for register fields */
struct reg_field
{
    struct type_member comm;
    int msb;
    int lsb;
};

/* Data for instances */
struct instance
{
    struct type_member comm;
    uint64_t offset;
    uint64_t stride;
    size_t count;

    /*
     * Number of unique paths that reach this instance.
     * (Arrayed instances only count as 1 path node.)
     */
    size_t path_count;
};

/* Type definition */
struct type_def
{
    /* Kind of type (enum/register/block) */
    enum type type;

    /* Type's full name */
    const char *name;

    /* Width of the type (for registers) */
    enum word_width width;

    /* Members */
    struct hashmap members;

    /* Sorted list of members (cached for ease of use) */
    struct array members_sorted;

    /* Flag used to track if the type was visited during code generation */
    bool visited;

    /* File where the type was written during code generation */
    struct output_file *output_file;

    /* Source location where type was defined */
    struct source_loc loc;
};

struct config
{
    enum word_width machine_word_width;
    enum word_width address_word_width;

    /* Prefix used for include guards on generated headers */
    const char *include_guard_prefix;

    /* Header included at the start of every generated header */
    const char *reggen_header_name;

    /* Prefixes used for each possible macro */
    const char *implicit_type_prefix;
    const char *register_iotype_macro_prefix;
    const char *field_pos_macro_prefix;
    const char *field_mask_macro_prefix;
    const char *field_enum_macro_prefix;
    const char *field_value_macro_prefix;
    const char *field_valmask_macro_prefix;
    const char *instance_address_macro_prefix;
    const char *instance_offset_macro_prefix;
    const char *instance_name_address_macro_prefix;
    const char *instance_name_offset_macro_prefix;
    const char *instance_type_address_macro_prefix;
    const char *instance_type_offset_macro_prefix;

    /* Output directory name */
    const char *output_directory;
};

struct context
{
    struct config config;

    /* name -> struct type_def mapping (non-owning) */
    struct hashmap types;

    /* String table for interning (owns strings) */
    struct hashmap strtab;

    /* List of all top-level instances */
    struct array root_instances;

    /* Owns all type_def / type_member objects */
    struct array typelist;
    struct array memblist;

    /* Error counter */
    size_t num_errors;
};

const char *type_to_str(enum type type);
char *vformat_string(const char *fmt, va_list ap_in);
char *format_string(const char *fmt, ...);
char *format_source_loc(const struct source_loc *loc);

void context_verror(struct context *ctx,
                    const struct source_loc *loc,
                    const char *msg, va_list ap_in);
void context_error(struct context *ctx,
                   const struct source_loc *loc,
                   const char *msg, ...);
void context_free(struct context *ctx);

const char *intern_static_string(struct context *ctx, const char *str);
const char *intern_string(struct context *ctx, char *str);
const char *intern_stringf(struct context *ctx, const char *fmt, ...);

struct type_def *type_def_init(struct context *ctx, const char *name,
                               enum type type, enum word_width width);

enum word_width map_word_type(struct context *ctx, enum word_width width);
int get_word_bits(struct context *ctx, enum word_width width);
const char *get_word_literal_suffix(struct context *ctx, enum word_width width);

#endif /* REGGEN_COMMON_H */
