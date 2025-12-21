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
#include "validate.h"
#include "common.h"
#include <stdlib.h>

static int sort_enum_members_by_value(const void *e1, const void *e2)
{
    const struct enum_value *eval1 = *(const void **)e1;
    const struct enum_value *eval2 = *(const void **)e2;

    if (eval1->value < eval2->value)
        return -1;
    if (eval1->value > eval2->value)
        return 1;

    return 0;
}

static int sort_register_fields_by_lsb(const void *f1, const void *f2)
{
    const struct reg_field *field1 = *(const void **)f1;
    const struct reg_field *field2 = *(const void **)f2;

    return field1->lsb - field2->lsb;
}

static int sort_instances_by_offset(const void *i1, const void *i2)
{
    const struct instance *inst1 = *(const void **)i1;
    const struct instance *inst2 = *(const void **)i2;

    if (inst1->offset < inst2->offset)
        return -1;
    if (inst1->offset > inst2->offset)
        return 1;

    return 0;
}

static void sort_type_members(struct type_def *tdef)
{
    struct type_member *member;
    HASHMAP_FOREACH(&tdef->members, member)
        array_push(&tdef->members_sorted, member);

    int (*sort_fn) (const void *, const void *);
    switch (tdef->type)
    {
    case TYPE_ENUM:  sort_fn = sort_enum_members_by_value;  break;
    case TYPE_REG:   sort_fn = sort_register_fields_by_lsb; break;
    case TYPE_BLOCK: sort_fn = sort_instances_by_offset;    break;
    default:         return;
    }

    qsort(tdef->members_sorted.data, tdef->members_sorted.size,
          sizeof(*tdef->members_sorted.data), sort_fn);
}

static void check_register_fields(struct context *ctx, struct type_def *tdef)
{
    /* Check overlap between fields */
    struct reg_field *field;
    const char *last_field_name = NULL;
    int cur_bit = 0;
    int register_bits = get_word_bits(ctx, tdef->width);
    ARRAY_FOREACH(&tdef->members_sorted, field)
    {
        if (field->lsb < cur_bit)
        {
            context_error(ctx, &field->comm.loc, "in %s: field %s overlaps %s",
                          tdef->name, field->comm.name, last_field_name);
        }

        if (field->msb >= register_bits ||
            field->lsb >= register_bits)
        {
            context_error(ctx, &field->comm.loc, "in %s: field %s doesn't fit in %d-bit register",
                          tdef->name, field->comm.name, register_bits);
        }

        last_field_name = field->comm.name;
        cur_bit = field->msb;
    }
}

static void count_instance_paths(struct instance *instance)
{
    instance->path_count += 1;

    struct type_def *tdef = instance->comm.type;
    if (tdef->type == TYPE_BLOCK)
    {
        struct instance *child_instance;
        ARRAY_FOREACH(&tdef->members_sorted, child_instance)
            count_instance_paths(child_instance);
    }
}

void validate(struct context *ctx)
{
    struct type_def *tdef;
    HASHMAP_FOREACH(&ctx->types, tdef)
    {
        sort_type_members(tdef);

        if (tdef->type == TYPE_REG)
            check_register_fields(ctx, tdef);
    }

    struct instance *instance;
    ARRAY_FOREACH(&ctx->root_instances, instance)
        count_instance_paths(instance);
}
