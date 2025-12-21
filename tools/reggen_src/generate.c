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
#include "generate.h"
#include "array.h"
#include "common.h"
#include "output.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

struct output_writer
{
    struct context *ctx;
    struct output_file *file;
    struct array instance_path;
};

static void write_register_macros(struct output_writer *wr, struct type_def *tdef)
{
    output_addcol(wr->file, 0, "/* Register: %s */", tdef->name);
    output_newline(wr->file);
    output_flush(wr->file);

    /* Register I/O type */
    output_addcol(wr->file, 0, "#define %s%s%s",
                  wr->ctx->config.register_iotype_macro_prefix,
                  wr->ctx->config.implicit_type_prefix,
                  tdef->name);
    output_addcol(wr->file, 1, " uint%d_t", get_word_bits(wr->ctx, tdef->width));
    output_newline(wr->file);

    /* Generate macros for each register field */
    struct reg_field *field;
    ARRAY_FOREACH(&tdef->members_sorted, field)
    {
        int width = field->msb - field->lsb + 1;
        uint64_t mask = ~(~0ull << width);
        const char *mask_suffix = get_word_literal_suffix(wr->ctx, tdef->width);

        /* Field mask: constant with all bits in field set to 1 */
        output_addcol(wr->file, 0, "#define %s%s%s_%s",
                      wr->ctx->config.field_mask_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      tdef->name, field->comm.name);
        output_addcol(wr->file, 1, " (0x%" PRIX64 "%s << %d)",
                      mask, mask_suffix, field->lsb);
        output_newline(wr->file);

        /* Field position: index of least significant bit of field */
        output_addcol(wr->file, 0, "#define %s%s%s_%s",
                      wr->ctx->config.field_pos_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      tdef->name, field->comm.name);
        output_addcol(wr->file, 1, " %d", field->lsb);
        output_newline(wr->file);

        /* Field value: function-like macro m(x) which puts value x in field */
        output_addcol(wr->file, 0, "#define %s%s%s_%s(x)",
                      wr->ctx->config.field_value_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      tdef->name, field->comm.name);
        output_addcol(wr->file, 1, " (((x) & 0x%" PRIX64 "%s) << %d)", mask, mask_suffix, field->lsb);
        output_newline(wr->file);

        /* Field mask value: same as field mask but takes an argument which is ignored */
        output_addcol(wr->file, 0, "#define %s%s%s_%s(x)",
                      wr->ctx->config.field_valmask_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      tdef->name, field->comm.name);
        output_addcol(wr->file, 1, " (0x%" PRIX64 "%s << %d)",
                      mask, mask_suffix, field->lsb);
        output_newline(wr->file);

        if (field->comm.type && hashmap_size(&field->comm.type->members) > 0)
        {
            /* Print enum members */
            struct enum_value *value;
            ARRAY_FOREACH(&field->comm.type->members_sorted, value)
            {
                output_addcol(wr->file, 0, "#define %s%s%s_%s_%s",
                              wr->ctx->config.field_enum_macro_prefix,
                              wr->ctx->config.implicit_type_prefix,
                              tdef->name, field->comm.name, value->comm.name);
                output_addcol(wr->file, 1, " 0x%" PRIX64 "%s", value->value, mask_suffix);
                output_newline(wr->file);
            }

            /*
             * Field enum value: function-like macro m(e) which puts the
             * value of the enum member e into the field
             */
            output_addcol(wr->file, 0, "#define %s%s%s_%s_V(e)",
                          wr->ctx->config.field_value_macro_prefix,
                          wr->ctx->config.implicit_type_prefix,
                          tdef->name, field->comm.name);
            output_addcol(wr->file, 1, " (%s%s%s_%s_##e << %d)",
                          wr->ctx->config.field_enum_macro_prefix,
                          wr->ctx->config.implicit_type_prefix,
                          tdef->name, field->comm.name, field->lsb);
            output_newline(wr->file);

            /* Field enum mask value: same as field mask value, ignores its argument */
            output_addcol(wr->file, 0, "#define %s%s%s_%s_V(e)",
                          wr->ctx->config.field_valmask_macro_prefix,
                          wr->ctx->config.implicit_type_prefix,
                          tdef->name, field->comm.name);
            output_addcol(wr->file, 1, " (0x%" PRIX64 "%s << %d)",
                          mask, mask_suffix, field->lsb);
            output_newline(wr->file);
        }
    }

    output_newline(wr->file);
    output_flush(wr->file);
}

static void write_instance_macros(struct output_writer *wr,
                                  struct type_def *enclosing_type,
                                  struct instance *instance)
{
    const char *addr_literal_suffix = get_word_literal_suffix(wr->ctx, WIDTH_ADDRESS);
    const char *addr_macro_prefix = enclosing_type ? wr->ctx->config.instance_offset_macro_prefix
                                                   : wr->ctx->config.instance_address_macro_prefix;
    const char *offset_macro_args = "";
    const char *address_macro_args = "";
    const char *instname = instance->comm.name;
    if (enclosing_type)
        instname = intern_stringf(wr->ctx, "%s_%s", enclosing_type->name, instname);

    /* Generate offset macro if not at top level */
    if (enclosing_type)
    {
        if (instance->count == 1)
        {
            offset_macro_args = "";
            output_addcol(wr->file, 1, " 0x%" PRIX64 "%s",
                          instance->offset, addr_literal_suffix);
        }
        else
        {
            offset_macro_args = "(i)";
            output_addcol(wr->file, 1, " (0x%" PRIX64 "%s + (i) * 0x%" PRIX64 "%s)",
                          instance->offset, addr_literal_suffix,
                          instance->stride, addr_literal_suffix);
        }

        output_addcol(wr->file, 0, "#define %s%s%s%s",
                      addr_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      instname, offset_macro_args);
        output_newline(wr->file);
    }

    /*
     * Generate address macro if the instance can only be accessed
     * through a unique path (which may contain arrayed instances).
     */
    bool has_address_macro = (instance->path_count == 1);
    if (has_address_macro)
    {
        output_addcol(wr->file, 0, "#define %s%s%s",
                      wr->ctx->config.instance_address_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      instname);
        output_addcol(wr->file, 1, " ");

        struct instance *path_inst;
        size_t instindex = 0;
        size_t argindex = 0;
        array_push(&wr->instance_path, instance);
        ARRAY_FOREACH(&wr->instance_path, path_inst)
        {
            if (path_inst->count == 1)
            {
                output_addcol(wr->file, 1, "%s0x%" PRIX64 "%s",
                              instindex == 0 ? "(" : " + ",
                              path_inst->offset, addr_literal_suffix);
            }
            else
            {
                output_addcol(wr->file, 0, "%si%zu",
                              argindex == 0 ? "(" : ", ",
                              argindex + 1);
                output_addcol(wr->file, 1, "%s(0x%" PRIX64 "%s + (i%zu) * 0x%" PRIX64 "%s)",
                              instindex == 0 ? "(" : " + ",
                              path_inst->offset, addr_literal_suffix,
                              argindex + 1,
                              path_inst->stride, addr_literal_suffix);
                argindex++;
            }

            instindex++;
        }

        array_pop(&wr->instance_path);

        if (argindex > 0)
        {
            /*
             * cheat a little bit; the ITTA and ITNA macros don't
             * use their macro arguments, they just need to have a
             * compatible signature with the address macro, which
             * varargs will always be.
             */
            address_macro_args = "(...)";
            output_addcol(wr->file, 0, ")");
        }

        output_addcol(wr->file, 1, ")");
        output_newline(wr->file);
    }

    /* Type name macro: expands to the name of the target register type */
    if (instance->comm.type->type == TYPE_REG &&
        hashmap_size(&instance->comm.type->members) > 0)
    {
        output_addcol(wr->file, 0, "#define %s%s%s%s",
                      wr->ctx->config.instance_name_offset_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      instname, offset_macro_args);
        output_addcol(wr->file, 1, " %s%s",
                      wr->ctx->config.implicit_type_prefix,
                      instance->comm.type->name);
        output_newline(wr->file);

        if (has_address_macro)
        {
            output_addcol(wr->file, 0, "#define %s%s%s%s",
                          wr->ctx->config.instance_name_address_macro_prefix,
                          wr->ctx->config.implicit_type_prefix,
                          instname, address_macro_args);
            output_addcol(wr->file, 1, " %s%s",
                          wr->ctx->config.implicit_type_prefix,
                          instance->comm.type->name);
            output_newline(wr->file);
        }
    }

    /* Access type macro: expands to the uintN_t of the target register type */
    if (instance->comm.type->type == TYPE_REG)
    {
        int num_bits = get_word_bits(wr->ctx, instance->comm.type->width);

        output_addcol(wr->file, 0, "#define %s%s%s%s",
                      wr->ctx->config.instance_type_offset_macro_prefix,
                      wr->ctx->config.implicit_type_prefix,
                      instname, offset_macro_args);
        output_addcol(wr->file, 1, " uint%d_t", num_bits);
        output_newline(wr->file);

        if (has_address_macro)
        {
            output_addcol(wr->file, 0, "#define %s%s%s%s",
                          wr->ctx->config.instance_type_address_macro_prefix,
                          wr->ctx->config.implicit_type_prefix,
                          instname, address_macro_args);
            output_addcol(wr->file, 1, " uint%d_t", num_bits);
            output_newline(wr->file);
        }
    }
}

static void write_block_macros(struct output_writer *wr, struct type_def *tdef)
{
    output_addcol(wr->file, 0, "/* Block: %s */", tdef->name);
    output_newline(wr->file);
    output_flush(wr->file);

    struct instance *instance;
    ARRAY_FOREACH(&tdef->members_sorted, instance)
        write_instance_macros(wr, tdef, instance);

    output_newline(wr->file);
    output_flush(wr->file);
}

static void write_type_def(struct output_writer *wr, struct instance *instance)
{
    struct type_def *tdef = instance->comm.type;
    if (tdef->visited)
    {
        if (wr->file == tdef->output_file)
            return;
        if (hashmap_size(&tdef->members) == 0)
            return;

        context_error(wr->ctx, &tdef->loc,
                      "referencing the same type in multiple output files is not supported");
        context_error(wr->ctx, &tdef->loc,
                      "type was first written to %s",
                      tdef->output_file->filename);
        context_error(wr->ctx, &instance->comm.loc,
                      "type also needed by instance %s in file %s",
                      instance->comm.name, wr->file->filename);
        return;
    }

    switch (tdef->type)
    {
    case TYPE_REG:
        if (hashmap_size(&tdef->members) > 0)
            write_register_macros(wr, tdef);
        break;

    case TYPE_BLOCK:
        write_block_macros(wr, tdef);
        break;

    default:
        context_error(wr->ctx, &tdef->loc, "internal error: unknown type");
        break;
    }

    tdef->visited = true;
    tdef->output_file = wr->file;
}

static void write_instance_types(struct output_writer *wr, struct instance *instance)
{
    array_push(&wr->instance_path, instance);

    /* Generate block member types first */
    struct type_def *tdef = instance->comm.type;
    if (tdef->type == TYPE_BLOCK)
    {
        struct instance *child_instance;
        ARRAY_FOREACH(&tdef->members_sorted, child_instance)
            write_instance_types(wr, child_instance);
    }

    /* Generate the type */
    write_type_def(wr, instance);

    array_pop(&wr->instance_path);
}

static const char *get_filename_for_type(struct context *ctx, struct type_def *tdef)
{
    char *fname = format_string("%s.h", tdef->name);
    for (char *s = fname; *s != 0; ++s)
        *s = tolower(*s);

    return intern_string(ctx, fname);
}

void generate_output(struct context *ctx)
{
    struct output_writer wr = {
        .ctx = ctx,
    };

    struct hashmap files = { 0 };
    struct instance *instance;
    ARRAY_FOREACH(&ctx->root_instances, instance)
    {
        const char *filename = get_filename_for_type(ctx, instance->comm.type);

        wr.file = hashmap_lookup(&files, filename);
        if (!wr.file)
        {
            const char *fpath = intern_stringf(ctx, "%s/%s",
                                               ctx->config.output_directory, filename);
            const char *include_guard_token = intern_stringf(ctx, "REGGEN_%s_%s_H",
                                                             ctx->config.include_guard_prefix,
                                                             instance->comm.type->name);

            wr.file = calloc(1, sizeof(*wr.file));
            wr.file->filename = filename;
            wr.file->include_guard_token = include_guard_token;
            wr.file->fstream = fopen(fpath, "wb");
            if (!wr.file->fstream)
                context_error(ctx, NULL, "failed to open output file for writing: %s", fpath);

            /* Write type information for new file */
            output_addcol(wr.file, 0, "#ifndef %s", include_guard_token);
            output_newline(wr.file);

            output_addcol(wr.file, 0, "#define %s", include_guard_token);
            output_newline(wr.file);
            output_newline(wr.file);

            if (ctx->config.reggen_header_name)
            {
                output_addcol(wr.file, 0, "#include \"%s\"",
                              ctx->config.reggen_header_name);
                output_newline(wr.file);
                output_newline(wr.file);
            }

            output_flush(wr.file);

            write_instance_types(&wr, instance);

            output_addcol(wr.file, 0, "/* Instances */");
            output_newline(wr.file);

            hashmap_insert(&files, filename, wr.file);
        }

        /* All root instances with the same type go into the same file */
        write_instance_macros(&wr, NULL, instance);
    }

    struct output_file *file;
    HASHMAP_FOREACH(&files, file)
    {
        /* Write file trailer */
        output_newline(file);
        output_flush(file);

        output_addcol(file, 0, "#endif /* %s */", file->include_guard_token);
        output_newline(file);
        output_flush(file);

        /* Cleanup */
        output_free(file);
        free(file);
    }

    array_free(&wr.instance_path);
    hashmap_free(&files);
}
