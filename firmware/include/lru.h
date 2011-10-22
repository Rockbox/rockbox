/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 Tat Tang
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

#ifndef LRU_H
#define LRU_H

/*******************************************************************************
 * LRU manager
 ******************************************************************************/
struct lru
{
    short _head;
    short _tail;
    short _size;
    short _slot_size;
    void *_base;
};

#define LRU_SLOT_OVERHEAD (2 * sizeof(short))

/* Create LRU list with specified size from buf. */
void lru_create(struct lru* pl, void *buf, short size, short data_size);
/* Touch an entry. Moves handle to back of LRU list */
void lru_touch(struct lru* pl, short handle);
/* Data */
void *lru_data(struct lru* pl, short handle);
/* Traverse lru-wise */
void lru_traverse(struct lru* pl, void (*callback)(void* data));

#endif /* LRU_H */

