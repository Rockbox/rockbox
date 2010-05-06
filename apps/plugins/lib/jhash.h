/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 by Andrew Mahone
*
* jhash.c headers
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

#ifndef _LIB_JHASH_H_
#define _LIB_JHASH_H_
#include <inttypes.h>     /* defines uint32_t etc */
#include <string.h>       /* size_t */
#include "plugin.h"

/*
hashw() -- hash an array of uint32_t into a 32-bit value
  k:       pointer to the key, an array of uint32_t
  length:  number of elements in the key
  initval: an initialization value
  returns the 32-bit hash
*/
uint32_t hashw(const uint32_t *k, size_t length, uint32_t  initval);

/*
hashw() -- hash an array of uint32_t into two 32-bit values
(*pc) will be the same as the return value from hashword().
  k:      pointer to the key, an array of uint32_t
  length: number of elements in the key
  pc, pb: pointers to primary and secondary initial values, also used to store
          the hash results.
*/
void hashw2 (const uint32_t *k, size_t length, uint32_t *pc, uint32_t *pb);

/*
hashs() -- hash an array of bytes into a 32-bit value
  k:       pointer to the key, an array of bytes
  length:  number of elements in the key
  initval: an initialization value
  returns the 32-bit hash
*/
uint32_t hashs( const void *key, size_t length, uint32_t initval);

/*
hashs2() -- hash an array of bytes into two 32-bit values
  k:       pointer to the key, an array of bytes
  length: number of elements in the key
  pc, pb: pointers to primary and secondary initial values, also used to store
          the hash results.
*/
void hashs2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);
#endif

