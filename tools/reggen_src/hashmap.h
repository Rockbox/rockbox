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
#ifndef REGGEN_HASHMAP_H
#define REGGEN_HASHMAP_H

#include <stddef.h>

/*
 * Open-addressed hash table with string keys
 */
struct hash_bucket
{
    const char *key;
    void *value;
};

struct hashmap
{
    struct hash_bucket *buckets;
    size_t capacity;
    size_t size;
};

#define HASHMAP_FOREACH(map, _value) \
    for (size_t index##_value = 0; index##_value < (map)->capacity; ++index##_value) \
        if ((_value = (map)->buckets[index##_value].value))

#define HASHERR_EXISTS  (-1)

void hashmap_free(struct hashmap *map);
int hashmap_insert(struct hashmap *map, const char *key, void *value);
void *hashmap_lookup(struct hashmap *map, const char *key);
size_t hashmap_size(struct hashmap *map);

#endif /* REGGEN_HASHMAP_H */
