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
#include "array.h"
#include <stdlib.h>
#include <string.h>

void array_free(struct array *array)
{
    free(array->data);
    memset(array, 0, sizeof(*array));
}

void array_push(struct array *array, void *data)
{
    if (array->size == array->capacity)
    {
        array->capacity += 128;
        array->data = realloc(array->data, array->capacity * sizeof(*array->data));
    }

    array->data[array->size++] = data;
}

void *array_pop(struct array *array)
{
    array->size--;
    return array->data[array->size];
}

size_t array_size(struct array *array)
{
    return array->size;
}

void *array_get(struct array *array, size_t index)
{
    return array->data[index];
}
