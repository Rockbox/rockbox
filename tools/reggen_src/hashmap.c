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
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

static size_t hash_string(const char *str)
{
    /* DJB hash */
    size_t hash = 5381;
    while (*str)
        hash = (33 * hash) + *str++;

    return hash;
}

static void hashmap_grow(struct hashmap *map)
{
    struct hash_bucket *oldbuckets = map->buckets;
    size_t oldcap = map->capacity;

    map->capacity = oldcap ? oldcap*2 : 32;
    map->buckets = calloc(map->capacity, sizeof(*map->buckets));
    map->size = 0;

    for (size_t i = 0; i < oldcap; ++i)
    {
        struct hash_bucket *bucket = &oldbuckets[i];
        if (bucket->key == NULL)
            continue;

        hashmap_insert(map, bucket->key, bucket->value);
    }

    free(oldbuckets);
}

void hashmap_free(struct hashmap *map)
{
    free(map->buckets);
    memset(map, 0, sizeof(*map));
}

int hashmap_insert(struct hashmap *map, const char *key, void *value)
{
    if (map->capacity == 0)
        hashmap_grow(map);

    size_t hash = hash_string(key);
    size_t startpos = hash % map->capacity;
    size_t pos = startpos;
    for (;;)
    {
        struct hash_bucket *bucket = &map->buckets[pos];
        if (bucket->key == NULL)
        {
            bucket->key = key;
            bucket->value = value;
            map->size++;
            return 0;
        }
        else if (!strcmp(key, bucket->key))
        {
            return HASHERR_EXISTS;
        }

        if (++pos == map->capacity)
            pos = 0;

        /* no free space; grow and try again */
        if (pos == startpos)
        {
            hashmap_grow(map);
            startpos = hash % map->capacity;
            pos = startpos;
        }
    }
}

void *hashmap_lookup(struct hashmap *map, const char *key)
{
    if (map->capacity == 0)
        return NULL;

    size_t hash = hash_string(key);
    size_t startpos = hash % map->capacity;
    size_t pos = startpos;
    for (;;)
    {
        struct hash_bucket *bucket = &map->buckets[pos];
        if (bucket->key == NULL)
            return NULL;

        if (!strcmp(key, bucket->key))
            return bucket->value;

        if (++pos == map->capacity)
            pos = 0;

        if (pos == startpos)
            return NULL;
    }
}

size_t hashmap_size(struct hashmap *map)
{
    return map->size;
}
