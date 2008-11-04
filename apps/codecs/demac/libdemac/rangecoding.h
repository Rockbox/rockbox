/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

/*

Range decoder adapted from rangecod.c included in:

  http://www.compressconsult.com/rangecoder/rngcod13.zip

  rangecod.c     range encoding

  (c) Michael Schindler
  1997, 1998, 1999, 2000
  http://www.compressconsult.com/
  michael@compressconsult.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.


The encoding functions were removed, and functions turned into "static
inline" functions and moved to a .h file.  Some minor cosmetic changes
were made (e.g. turning pre-processor symbols into upper-case,
removing the rc parameter from each function (and the RNGC macro)).


*/


/* BITSTREAM READING FUNCTIONS */

/* We deal with the input data one byte at a time - to ensure
   functionality on CPUs of any endianness regardless of any requirements
   for aligned reads. 
*/

static unsigned char* bytebuffer IBSS_ATTR;
static int bytebufferoffset IBSS_ATTR;

static inline void skip_byte(void)
{
    bytebufferoffset--;
    bytebuffer += bytebufferoffset & 4;
    bytebufferoffset &= 3;
}

static inline int read_byte(void)
{
    int ch = bytebuffer[bytebufferoffset];

    skip_byte();

    return ch;
}

/* RANGE DECODING FUNCTIONS */

/* SIZE OF RANGE ENCODING CODE VALUES. */

#define CODE_BITS 32
#define TOP_VALUE ((unsigned int)1 << (CODE_BITS-1))
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

struct rangecoder_t
{
    uint32_t low;        /* low end of interval */
    uint32_t range;      /* length of interval */
    uint32_t help;       /* bytes_to_follow resp. intermediate value */
    unsigned int buffer; /* buffer for input/output */
};

static struct rangecoder_t rc IBSS_ATTR;

/* Start the decoder */
static inline void range_start_decoding(void)
{
    rc.buffer = read_byte();
    rc.low = rc.buffer >> (8 - EXTRA_BITS);
    rc.range = (uint32_t) 1 << EXTRA_BITS;
}

static inline void range_dec_normalize(void)
{
    while (rc.range <= BOTTOM_VALUE)
    {   
        rc.buffer = (rc.buffer << 8) | read_byte();
        rc.low = (rc.low << 8) | ((rc.buffer >> 1) & 0xff);
        rc.range <<= 8;
    }
}

/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* tot_f is the total frequency                              */
/* or: totf is (code_value)1<<shift                                      */
/* returns the culmulative frequency                         */
static inline int range_decode_culfreq(int tot_f)
{
    range_dec_normalize();
    rc.help = rc.range / tot_f;
    return rc.low / rc.help;
}

static inline int range_decode_culshift(int shift)
{
    range_dec_normalize();
    rc.help = rc.range >> shift;
    return rc.low / rc.help;
}


/* Update decoding state                                     */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
static inline void range_decode_update(int sy_f, int lt_f)
{
    rc.low -= rc.help * lt_f;
    rc.range = rc.help * sy_f;
}


/* Decode a byte/short without modelling                     */
static inline unsigned char decode_byte(void)
{   int tmp = range_decode_culshift(8);
    range_decode_update( 1,tmp);
    return tmp;
}

static inline int short range_decode_short(void)
{   int tmp = range_decode_culshift(16);
    range_decode_update( 1,tmp);
    return tmp;
}

/* Decode n bits (n <= 16) without modelling - based on range_decode_short */
static inline int range_decode_bits(int n)
{   int tmp = range_decode_culshift(n);
    range_decode_update( 1,tmp);
    return tmp;
}


/* Finish decoding                                           */
static inline void range_done_decoding(void)
{   range_dec_normalize();      /* normalize to use up all bytes */
}
