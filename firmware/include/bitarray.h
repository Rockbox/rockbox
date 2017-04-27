/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#ifndef BITARRAY_H
#define BITARRAY_H

/* Type-checked bit array definitions */

/* All this stuff gets optimized into very simple object code */

#define BITARRAY_WORD_BITS  \
     (sizeof (unsigned int) * 8)
#define BITARRAY_NWORDS(bits) \
    (((bits) + BITARRAY_WORD_BITS - 1) / BITARRAY_WORD_BITS)
#define BITARRAY_BITWORD(bitnum) \
    ((bitnum) / BITARRAY_WORD_BITS)
#define BITARRAY_WORDBIT(bitnum) \
    ((bitnum) % BITARRAY_WORD_BITS)
#define BITARRAY_NBIT(word, bit) \
    ((word)*BITARRAY_WORD_BITS + (bit))
#define BITARRAY_BITS(bits) \
    (BITARRAY_NWORDS(bits)*BITARRAY_WORD_BITS)
#define BITARRAY_BITN(bitnum) \
    (1u << BITARRAY_WORDBIT(bitnum))


/** Iterators **/
#include "config.h"
#include <stdint.h>
#include <limits.h>

#if (defined(CPU_ARM) && ARM_ARCH >= 5) || UINT32_MAX < UINT_MAX
#define __BITARRAY_CTZ(wval)    __builtin_ctz(wval)
#else
#include "system.h"
#define __BITARRAY_CTZ(wval)    find_first_set_bit(wval)
#endif
#define __BITARRAY_POPCNT(wval) __builtin_popcount(wval)

#ifndef BIT_N
#define BIT_N(n) (1u << (n))
#endif

/* Enumerate each word index */
#define FOR_EACH_BITARRAY_WORD_INDEX(nwords, index) \
    for (unsigned int index = 0, _nwords = (nwords); \
         index < _nwords; index++)

/* Enumerate each word value */
#define FOR_EACH_BITARRAY_WORD(a, wval) \
    FOR_EACH_BITARRAY_WORD_INDEX(ARRAYLEN((a)->words), _w) \
        for (unsigned int wval = (a)->words[_w], _ = 1; _; _--)

/* Enumerate the bit number of each set bit of a word in sequence */
#define FOR_EACH_BITARRAY_SET_WORD_BIT(wval, bit) \
    for (unsigned int _wval = (wval), bit;                 \
         _wval ? (((bit) = __BITARRAY_CTZ(_wval)), 1) : 0; \
         _wval &= ~BIT_N(bit))

/* Enumerate the bit number of each set bit in the bit array in sequence */
#define FOR_EACH_BITARRAY_SET_BIT_ARR(nwords, words, nbit) \
    FOR_EACH_BITARRAY_WORD_INDEX(nwords, _w)               \
        FOR_EACH_BITARRAY_SET_WORD_BIT(words[_w], _bit)    \
            for (unsigned int nbit = BITARRAY_NBIT(_w, _bit), _ = 1; _; _--)

/* As above but takes an array type for an argument */
#define FOR_EACH_BITARRAY_SET_BIT(a, nbit) \
    FOR_EACH_BITARRAY_SET_BIT_ARR(ARRAYLEN((a)->words), (a)->words, nbit)


/** Base functions (called by typed functions) **/

/* Return the word associated with the bit */
static inline unsigned int
__bitarray_get_word(unsigned int words[], unsigned int bitnum)
{
    return words[BITARRAY_BITWORD(bitnum)];
}

/* Set the word associated with the bit */
static inline void
__bitarray_set_word(unsigned int words[], unsigned int bitnum,
                    unsigned int wordval)
{
    words[BITARRAY_BITWORD(bitnum)] = wordval;
}

/* Set the bit at index 'bitnum' to '1' */
static inline void
__bitarray_set_bit(unsigned int words[], unsigned int bitnum)
{
    unsigned int word = BITARRAY_BITWORD(bitnum);
    unsigned int bit  = BITARRAY_BITN(bitnum);
    words[word] |= bit;
}

/* Set the bit at index 'bitnum' to '0' */
static inline void
__bitarray_clear_bit(unsigned int words[], unsigned int bitnum)
{
    unsigned int word = BITARRAY_BITWORD(bitnum);
    unsigned int bit  = BITARRAY_BITN(bitnum);
    words[word] &= ~bit;
}

/* Return the value of the specified bit ('0' or '1') */
static inline unsigned int
__bitarray_test_bit(const unsigned int words[], unsigned int bitnum)
{
    unsigned int word = BITARRAY_BITWORD(bitnum);
    unsigned int nbit = BITARRAY_WORDBIT(bitnum);
    return (words[word] >> nbit) & 1u;
}

/* Check if all bits in the bit array are '0' */
static inline bool
__bitarray_is_clear(const unsigned int words[], unsigned int nbits)
{
    FOR_EACH_BITARRAY_WORD_INDEX(BITARRAY_NWORDS(nbits), word)
    {
        if (words[word] != 0)
            return false;
    }

    return true;
}

/* Set every bit in the array to '0' */
static inline void
__bitarray_clear(unsigned int words[], unsigned int nbits)
{
    FOR_EACH_BITARRAY_WORD_INDEX(BITARRAY_NWORDS(nbits), word)
        words[word] = 0;
}

/* Set every bit in the array to '1' */
static inline void
__bitarray_set(unsigned int words[], unsigned int nbits)
{
    FOR_EACH_BITARRAY_WORD_INDEX(BITARRAY_NWORDS(nbits), word)
        words[word] = ~0u;
}

/* Find the lowest-indexed '1' bit in the bit array, returning the size of
   the array if none are set */
static inline unsigned int
__bitarray_ffs(const unsigned int words[], unsigned int nbits)
{
    FOR_EACH_BITARRAY_SET_BIT_ARR(BITARRAY_NWORDS(nbits), words, nbit)
        return nbit;

    return BITARRAY_BITS(nbits);
}

/* Return the number of bits currently set to '1' in the bit array */
static inline unsigned int
__bitarray_popcount(const unsigned int words[], unsigned int nbits)
{
    unsigned int count = 0;

    FOR_EACH_BITARRAY_WORD_INDEX(BITARRAY_NWORDS(nbits), word)
        count += __BITARRAY_POPCNT(words[word]);

    return count;
}

/**
 * Giant macro to define all the typed functions
 *   typename: The name of the type (e.g. myarr_t myarr;)
 *   fnprefix: The prefix all functions get (e.g. myarr_set_bit)
 *   nbits   : The minimum number of bits the array is meant to hold
 *             (the implementation rounds this up to the word size
 *              and all words may be fully utilized)
 *
 * uses 'typedef' to freely change from, e.g., struct to union without
 * changing source code
 */
#define BITARRAY_TYPE_DECLARE(typename, fnprefix, nbits) \
typedef struct                                                  \
{                                                               \
    unsigned int words[BITARRAY_NWORDS(nbits)];                 \
} typename;                                                     \
static inline unsigned int                                      \
fnprefix##_get_word(typename *array, unsigned int bitnum)       \
    { return __bitarray_get_word(array->words, bitnum); }       \
static inline void                                              \
fnprefix##_set_word(typename *array, unsigned int bitnum,       \
                    unsigned int wordval)                       \
    { __bitarray_set_word(array->words, bitnum, wordval); }     \
static inline void                                              \
fnprefix##_set_bit(typename *array, unsigned int bitnum)        \
    { __bitarray_set_bit(array->words, bitnum); }               \
static inline void                                              \
fnprefix##_clear_bit(typename *array, unsigned int bitnum)      \
    { __bitarray_clear_bit(array->words, bitnum); }             \
static inline unsigned int                                      \
fnprefix##_test_bit(const typename *array, unsigned int bitnum) \
    { return __bitarray_test_bit(array->words, bitnum); }       \
static inline bool                                              \
fnprefix##_is_clear(const typename *array)                      \
    { return __bitarray_is_clear(array->words, nbits); }        \
static inline void                                              \
fnprefix##_clear(typename *array)                               \
    { __bitarray_clear(array->words, nbits); }                  \
static inline void                                              \
fnprefix##_set(typename *array)                                 \
    { __bitarray_set(array->words, nbits); }                    \
static inline unsigned int                                      \
fnprefix##_ffs(const typename *array)                           \
    { return __bitarray_ffs(array->words, nbits); }             \
static inline unsigned int                                      \
fnprefix##_popcount(const typename *array)                      \
    { return __bitarray_popcount(array->words, nbits); }

#endif /* BITARRAY_H */
