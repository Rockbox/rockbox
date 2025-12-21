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
#ifndef REGGEN_ARRAY_H
#define REGGEN_ARRAY_H

#include <stddef.h>

/*
 * Dynamically sized array
 */
struct array
{
    void **data;
    size_t size;
    size_t capacity;
};

#define ARRAY_FOREACH(arr, value) \
    for (size_t index##value = 0; index##value < (arr)->size; ++index##value) \
        if (value = (arr)->data[index##value], 1)

void array_free(struct array *array);
void array_push(struct array *array, void *data);
void *array_pop(struct array *array);
size_t array_size(struct array *array);
void *array_get(struct array *array, size_t index);

#endif /* REGGEN_ARRAY_H */
