/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Original source:
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 *
 * Rockbox adaptation:
 * Copyright (c) 2010 by Marcin Bukat
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

/*
 * tinflate  -  tiny inflate
 *
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 *
 * http://www.ibsensoftware.com/
 *
 * This software is provided 'as-is', without any express
 * or implied warranty.  In no event will the authors be
 * held liable for any damages arising from the use of
 * this software.
 *
 * Permission is granted to anyone to use this software
 * for any purpose, including commercial applications,
 * and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The origin of this software must not be
 *    misrepresented; you must not claim that you
 *    wrote the original software. If you use this
 *    software in a product, an acknowledgment in
 *    the product documentation would be appreciated
 *    but is not required.
 *
 * 2. Altered source versions must be plainly marked
 *    as such, and must not be misrepresented as
 *    being the original software.
 *
 * 3. This notice may not be removed or altered from
 *    any source distribution.
 */

#include "tinf.h"
#include <string.h> /* memcpy */

/* ------------------------------ *
 * -- internal data structures -- *
 * ------------------------------ */

typedef struct {
   unsigned short table[16];  /* table of code length counts */
   unsigned short trans[288]; /* code -> symbol translation table */
} TINF_TREE;

typedef struct {
   unsigned short table[16];
   unsigned short trans[32];
} TINF_SDTREE;

typedef struct {
   TINF_TREE sltree;
   TINF_TREE sdtree;
   unsigned char length_bits[30];
   unsigned short length_base[30];
   unsigned char dist_bits[30];
   unsigned short dist_base[30];
   unsigned char clcidx[19];
} TINF_TABLES;

typedef struct {
   const unsigned char *source;
   unsigned int tag;
   unsigned int bitcount;

   unsigned char *dest;
   unsigned int *destLen;

   TINF_TREE ltree; /* dynamic length/symbol tree */
   TINF_TREE dtree; /* dynamic distance tree */
} TINF_DATA;

/* static tables */
static TINF_TABLES tbl = {
    .sltree = {
        .table = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0018,
                  0x0098, 0x0070, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},

        .trans = {0x0100, 0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107,
                  0x0108, 0x0109, 0x010a, 0x010b, 0x010c, 0x010d, 0x010e, 0x010f,
                  0x0110, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116, 0x0117,
                  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
                  0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
                  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
                  0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
                  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
                  0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
                  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
                  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
                  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
                  0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
                  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
                  0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
                  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
                  0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
                  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
                  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
                  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
                  0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
                  0x0118, 0x0119, 0x011a, 0x011b, 0x011c, 0x011d, 0x011e, 0x011f,
                  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
                  0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
                  0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
                  0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
                  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
                  0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
                  0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
                  0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
                  0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
                  0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
                  0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
                  0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
                  0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
                  0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff},
              },

    .sdtree = {
        .table = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000,
                  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},

        .trans = {0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
                  0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
                  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
                  0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f},
              },

    .length_bits = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
                    0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04,
                    0x05, 0x05, 0x05, 0x05, 0x00, 0x06},

    .length_base = {0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a,
                    0x000b, 0x000d, 0x000f, 0x0011, 0x0013, 0x0017, 0x001b, 0x001f,
                    0x0023, 0x002b, 0x0033, 0x003b, 0x0043, 0x0053, 0x0063, 0x0073,
                    0x0083, 0x00a3, 0x00c3, 0x00e3, 0x0102, 0x0143},

    .dist_bits = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02,
                  0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
                  0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0a, 0x0a,
                  0x0b, 0x0b, 0x0c, 0x0c, 0x0d, 0x0d},

    .dist_base = {0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d,
                  0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061, 0x0081, 0x00c1,
                  0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
                  0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001},

    .clcidx = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15},
};


/* ----------------------- *
 * -- utility functions -- *
 * ----------------------- */

/* given an array of code lengths, build a tree */
static void tinf_build_tree(TINF_TREE *t, const unsigned char *lengths, unsigned int num)
{
   unsigned short offs[16];
   unsigned int i, sum;

   /* clear code length count table */
   /* for (i = 0; i < 16; ++i) t->table[i] = 0; */
   memset(t->table, 0, sizeof(short)*16);

   /* scan symbol lengths, and sum code length counts */
   for (i = 0; i < num; ++i) t->table[lengths[i]]++;

   t->table[0] = 0;

   /* compute offset table for distribution sort */
   for (sum = 0, i = 0; i < 16; ++i)
   {
      offs[i] = sum;
      sum += t->table[i];
   }

   /* create code->symbol translation table (symbols sorted by code) */
   for (i = 0; i < num; ++i)
   {
      if (lengths[i]) t->trans[offs[lengths[i]]++] = i;
   }
}

/* ---------------------- *
 * -- decode functions -- *
 * ---------------------- */

/* get one bit from source stream */
static int tinf_getbit(TINF_DATA *d)
{
   unsigned int bit;

   /* check if tag is empty */
   if (!d->bitcount--)
   {
      /* load next tag */
      d->tag = *d->source++;
      d->bitcount = 7;
   }

   /* shift bit out of tag */
   bit = d->tag & 0x01;
   d->tag >>= 1;

   return bit;
}

/* read a num bit value from a stream and add base */
static unsigned int tinf_read_bits(TINF_DATA *d, int num, int base)
{
   unsigned int val = 0;

   /* read num bits */
   if (num)
   {
      unsigned int limit = 1 << (num);
      unsigned int mask;

      for (mask = 1; mask < limit; mask <<= 1)
         if (tinf_getbit(d)) val += mask;
   }

   return val + base;
}

/* given a data stream and a tree, decode a symbol */
static int tinf_decode_symbol(TINF_DATA *d, TINF_TREE *t)
{
   int sum = 0, cur = 0, len = 0;

   /* get more bits while code value is above sum */
   do {

      cur = (cur << 1) + tinf_getbit(d);

      ++len;

      sum += t->table[len];
      cur -= t->table[len];

   } while (cur >= 0);

   return t->trans[sum + cur];
}

/* given a data stream, decode dynamic trees from it */
static void tinf_decode_trees(TINF_DATA *d, TINF_TREE *lt, TINF_TREE *dt, TINF_TABLES *tbl)
{
   TINF_TREE code_tree;
   unsigned char lengths[288+32];
   unsigned int hlit, hdist, hclen;
   unsigned int i, num, length;

   /* get 5 bits HLIT (257-286) */
   hlit = tinf_read_bits(d, 5, 257);

   /* get 5 bits HDIST (1-32) */
   hdist = tinf_read_bits(d, 5, 1);

   /* get 4 bits HCLEN (4-19) */
   hclen = tinf_read_bits(d, 4, 4);

   /* for (i = 0; i < 19; ++i) lengths[i] = 0; */
   memset(lengths, 0, sizeof(unsigned char)*19);

   /* read code lengths for code length alphabet */
   for (i = 0; i < hclen; ++i)
   {
      /* get 3 bits code length (0-7) */
      unsigned int clen = tinf_read_bits(d, 3, 0);

      lengths[tbl->clcidx[i]] = clen;
   }

   /* build code length tree */
   tinf_build_tree(&code_tree, lengths, 19);

   /* decode code lengths for the dynamic trees */
   for (num = 0; num < hlit + hdist; )
   {
      int sym = tinf_decode_symbol(d, &code_tree);

      switch (sym)
      {
      case 16:
         /* copy previous code length 3-6 times (read 2 bits) */
         {
            unsigned char prev = lengths[num - 1];
            for (length = tinf_read_bits(d, 2, 3); length; --length)
            {
               lengths[num++] = prev;
            }
         }
         break;
      case 17:
         /* repeat code length 0 for 3-10 times (read 3 bits) */
         for (length = tinf_read_bits(d, 3, 3); length; --length)
         {
            lengths[num++] = 0;
         }
         break;
      case 18:
         /* repeat code length 0 for 11-138 times (read 7 bits) */
         for (length = tinf_read_bits(d, 7, 11); length; --length)
         {
            lengths[num++] = 0;
         }
         break;
      default:
         /* values 0-15 represent the actual code lengths */
         lengths[num++] = sym;
         break;
      }
   }

   /* build dynamic trees */
   tinf_build_tree(lt, lengths, hlit);
   tinf_build_tree(dt, lengths + hlit, hdist);
}

/* ----------------------------- *
 * -- block inflate functions -- *
 * ----------------------------- */

/* given a stream and two trees, inflate a block of data */
static int tinf_inflate_block_data(TINF_DATA *d, TINF_TREE *lt, TINF_TREE *dt, TINF_TABLES *tbl)
{
   /* remember current output position */
   unsigned char *start = d->dest;

   while (1)
   {
      int sym = tinf_decode_symbol(d, lt);

      /* check for end of block */
      if (sym == 256)
      {
         *d->destLen += d->dest - start;
         return TINF_OK;
      }

      if (sym < 256)
      {
         *d->dest++ = sym;

      } else {

         int length, dist, offs;
         int i;

         sym -= 257;

         /* possibly get more bits from length code */
         length = tinf_read_bits(d, tbl->length_bits[sym], tbl->length_base[sym]);

         dist = tinf_decode_symbol(d, dt);

         /* possibly get more bits from distance code */
         offs = tinf_read_bits(d, tbl->dist_bits[dist], tbl->dist_base[dist]);

         /* copy match */
         for (i = 0; i < length; ++i)
         {
            d->dest[i] = d->dest[i - offs];
         }

         d->dest += length;
      }
   }
}

/* inflate an uncompressed block of data */
static int tinf_inflate_uncompressed_block(TINF_DATA *d)
{
   unsigned int length, invlength;
   /* unsigned int i; */

   /* get length */
   length = d->source[1];
   length = (length << 8) + d->source[0];

   /* get one's complement of length */
   invlength = d->source[3];
   invlength = (invlength << 8) + d->source[2];

   /* check length */
   if (length != (~invlength & 0x0000ffff)) return TINF_DATA_ERROR;

   d->source += 4;

   /* copy block */
   /* for (i = length; i; --i) *d->dest++ = *d->source++; */
   memcpy(d->dest, d->source, sizeof(unsigned char)*length);
   d->dest += sizeof(unsigned char)*length;
   d->source += sizeof(unsigned char)*length;

   /* make sure we start next block on a byte boundary */
   d->bitcount = 0;

   *d->destLen += length;

   return TINF_OK;
}

/* inflate a block of data compressed with fixed huffman trees */
static int tinf_inflate_fixed_block(TINF_DATA *d, TINF_TABLES *tbl)
{
   /* decode block using fixed trees */
   return tinf_inflate_block_data(d, &tbl->sltree, &tbl->sdtree, tbl);
}

/* inflate a block of data compressed with dynamic huffman trees */
static int tinf_inflate_dynamic_block(TINF_DATA *d, TINF_TABLES *tbl)
{
   /* decode trees from stream */
   tinf_decode_trees(d, &d->ltree, &d->dtree, tbl);

   /* decode block using decoded trees */
   return tinf_inflate_block_data(d, &d->ltree, &d->dtree, tbl);
}

/* ---------------------- *
 * -- public functions -- *
 * ---------------------- */

/* inflate stream from source to dest */
int tinf_uncompress(void *dest, unsigned int *destLen,
                    const void *source, unsigned int sourceLen)
{
    TINF_DATA d;
    int bfinal;

    d.source = (const unsigned char *)source;
    d.bitcount = 0;

    d.dest = (unsigned char *)dest;
    d.destLen = destLen;

    *destLen = 0;

    do {

       unsigned int btype;
       int res;

       /* read final block flag */
       bfinal = tinf_getbit(&d);

       /* read block type (2 bits) */
       btype = tinf_read_bits(&d, 2, 0);

       /* decompress block */
       switch (btype)
       {
       case 0:
          /* decompress uncompressed block */
          res = tinf_inflate_uncompressed_block(&d);
          break;
        case 1:
          /* decompress block with fixed huffman trees */
          res = tinf_inflate_fixed_block(&d,&tbl);
          break;
       case 2:
          /* decompress block with dynamic huffman trees */
          res = tinf_inflate_dynamic_block(&d,&tbl);
          break;
       default:
          return TINF_DATA_ERROR;
       }

       if (res != TINF_OK) return TINF_DATA_ERROR;

       if (d.source > (unsigned char *)source + sourceLen)
           return TINF_DATA_ERROR;
    } while (!bfinal);

    return TINF_OK;
}
