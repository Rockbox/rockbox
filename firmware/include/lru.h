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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

