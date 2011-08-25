/*
 * gunzip implementation for wikiviewer (c) Frederik M.J.V., 2006.
 * some bug fixes by Adam Gashlin gunzip implementation for busybox
 *
 * Based on GNU gzip v1.2.4 Copyright (C) 1992-1993 Jean-loup Gailly.
 *
 * Originally adjusted for busybox by Sven Rudolph <sr1@inf.tu-dresden.de>
 * based on gzip sources
 *
 * Adjusted further by Erik Andersen <andersen@codepoet.org> to support files as
 *well as stdin/stdout, and to generally behave itself wrt command line
 *handling.
 *
 * General cleanup to better adhere to the style guide and make use of standard
 *busybox functions by Glenn McGrath <bug1@iinet.net.au>
 *
 * read_gz interface + associated hacking by Laurence Anderson
 *
 * Fixed huft_build() so decoding end-of-block code does not grab more bits than
 *necessary (this is required by unzip applet), added inflate_cleanup() to free
 *leaked bytebuffer memory (used in unzip.c), and some minor style guide
 *cleanups by Ed Clark
 *
 * gzip (GNU zip) -- compress files with zip algorithm and 'compress' interface
 *Copyright (C) 1992-1993 Jean-loup Gailly The unzip code was written and put in
 *the public domain by Mark Adler. Portions of the lzw code are derived from the
 *public domain 'compress'
 * written by Spencer Thomas, Joe Orost, James Woods, Jim McKie, Steve Davies,
 *Ken Turkowski, Dave Mack and Peter Jannesen.
 *
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <inttypes.h>
#ifndef NULL
#define NULL 0
#endif
#define ENABLE_DESKTOP 0
#define USE_DESKTOP(...)
#include "mallocer.h"
#include "bbfuncs.h"
#include "inflate.h"

#define TRIM_FILE_ON_ERROR 1

typedef struct huft_s {
    unsigned char e;    /* number of extra bits or operation */
    unsigned char b;    /* number of bits in this code or subcode */
    union {
        unsigned short n;    /* literal, length base, or distance base */
        struct huft_s *t;    /* pointer to next level of table */
    } v;
} huft_t;

/*static void *mainmembuf;*/
static void *huftbuffer1;
static void *huftbuffer2;

#define HUFT_MMP1 8
#define HUFT_MMP2 9

static struct mbreader_t *gunzip_src_md;
static unsigned int gunzip_bytes_out;    /* number of output bytes */
static unsigned int gunzip_outbuf_count;    /* bytes in output buffer */

/* gunzip_window size--must be a power of two, and at least 32K for zip's
   deflate method */
enum {
    gunzip_wsize = 0x8000
};

static unsigned char *gunzip_window;
static uint32_t ifl_total;

static uint32_t gunzip_crc;

/* If BMAX needs to be larger than 16, then h and x[] should be ulg. */
#define BMAX 16    /* maximum bit length of any code (16 for explode) */
#define N_MAX 288    /* maximum number of codes in any set */

/* bitbuffer */
static unsigned int gunzip_bb;    /* bit buffer */
static unsigned char gunzip_bk;    /* bits in bit buffer */

/* These control the size of the bytebuffer */
static unsigned int bytebuffer_max = 0x8000;
static unsigned char *bytebuffer = NULL;
static unsigned int bytebuffer_offset = 0;
static unsigned int bytebuffer_size = 0;

static const unsigned short mask_bits[] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

/* Copy lengths for literal codes 257..285 */
static const unsigned short cplens[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

/* note: see note #13 above about the 258 in this list. */
/* Extra bits for literal codes 257..285 */
static const unsigned char cplext[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5,
    5, 5, 5, 0, 99, 99
};                        /* 99==invalid */

/* Copy offsets for distance codes 0..29 */
static const unsigned short cpdist[] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
    769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/* Extra bits for distance codes */
static const unsigned char cpdext[] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
    11, 11, 12, 12, 13, 13
};

/* Tables for deflate from PKZIP's appnote.txt. */
/* Order of the bit length code lengths */
static const unsigned char border[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static const uint32_t crc_table[256]= {
    0,1996959894,-301047508,-1727442502,124634137,1886057615,
    -379345611,-1637575261,249268274,2044508324,-522852066,
    -1747789432,162941995,2125561021,-407360249,-1866523247,
    498536548,1789927666,-205950648,-2067906082,450548861,
    1843258603,-187386543,-2083289657,325883990,1684777152,
    -43845254,-1973040660,335633487,1661365465,-99664541,
    -1928851979,997073096,1281953886,-715111964,-1570279054,
    1006888145,1258607687,-770865667,-1526024853,901097722,
    1119000684,-608450090,-1396901568,853044451,1172266101,
    -589951537,-1412350631,651767980,1373503546,-925412992,
    -1076862698,565507253,1454621731,-809855591,-1195530993,
    671266974,1594198024,-972236366,-1324619484,795835527,
    1483230225,-1050600021,-1234817731,1994146192,31158534,
    -1731059524,-271249366,1907459465,112637215,-1614814043,
    -390540237,2013776290,251722036,-1777751922,-519137256,
    2137656763,141376813,-1855689577,-429695999,1802195444,
    476864866,-2056965928,-228458418,1812370925,453092731,
    -2113342271,-183516073,1706088902,314042704,-1950435094,
    -54949764,1658658271,366619977,-1932296973,-69972891,
    1303535960,984961486,-1547960204,-725929758,1256170817,
    1037604311,-1529756563,-740887301,1131014506,879679996,
    -1385723834,-631195440,1141124467,855842277,-1442165665,
    -586318647,1342533948,654459306,-1106571248,-921952122,
    1466479909,544179635,-1184443383,-832445281,1591671054,
    702138776,-1328506846,-942167884,1504918807,783551873,
    -1212326853,-1061524307,-306674912,-1698712650,62317068,
    1957810842,-355121351,-1647151185,81470997,1943803523,
    -480048366,-1805370492,225274430,2053790376,-468791541,
    -1828061283,167816743,2097651377,-267414716,-2029476910,
    503444072,1762050814,-144550051,-2140837941,426522225,
    1852507879,-19653770,-1982649376,282753626,1742555852,
    -105259153,-1900089351,397917763,1622183637,-690576408,
    -1580100738,953729732,1340076626,-776247311,-1497606297,
    1068828381,1219638859,-670225446,-1358292148,906185462,
    1090812512,-547295293,-1469587627,829329135,1181335161,
    -882789492,-1134132454,628085408,1382605366,-871598187,
    -1156888829,570562233,1426400815,-977650754,-1296233688,
    733239954,1555261956,-1026031705,-1244606671,752459403,
    1541320221,-1687895376,-328994266,1969922972,40735498,
    -1677130071,-351390145,1913087877,83908371,-1782625662,
    -491226604,2075208622,213261112,-1831694693,-438977011,
    2094854071,198958881,-2032938284,-237706686,1759359992,
    534414190,-2118248755,-155638181,1873836001,414664567,
    -2012718362,-15766928,1711684554,285281116,-1889165569,
    -127750551,1634467795,376229701,-1609899400,-686959890,
    1308918612,956543938,-1486412191,-799009033,1231636301,
    1047427035,-1362007478,-640263460,1088359270,936918000,
    -1447252397,-558129467,1202900863,817233897,-1111625188,
    -893730166,1404277552,615818150,-1160759803,-841546093,
    1423857449,601450431,-1285129682,-1000256840,1567103746,
    711928724,-1274298825,-1022587231,1510334235,755167117
};

static unsigned int fill_bitbuffer(unsigned int bitbuffer, unsigned int *current,
                                   const unsigned int required)
{
    while (*current < required)
    {
        if (bytebuffer_offset >= bytebuffer_size)
        {
            /* Leave the first 4 bytes empty so we can always unwind the
               bitbuffer to the front of the bytebuffer, leave 4 bytes free at
               end of tail so we can easily top up buffer in
               check_trailer_gzip() */
            if (1 > (bytebuffer_size = safe_read(gunzip_src_md, &bytebuffer[4],
                                                 bytebuffer_max - 8)))
                error_die("unexpected end of file");

            bytebuffer_size += 4;
            bytebuffer_offset = 4;
        }

        bitbuffer |= ((unsigned int) bytebuffer[bytebuffer_offset]) << *current;
        bytebuffer_offset++;
        *current += 8;
    }
    return(bitbuffer);
}

/*
 * Free the malloc'ed tables built by huft_build(), which makes a linked list of
 *the tables it made, with the links in a dummy first entry of each table.
 * t: table to free
 */
static int huft_free(huft_t * t,unsigned char bufnum)
{
    wpw_reset_mempool(bufnum);
    if(t==0)
    {
    }

    return 0;
}

/* Given a list of code lengths and a maximum table size, make a set of tables
   to decode that set of codes.  Return zero on success, one if the given code
   set is incomplete (the tables are still built in this case), two if the input
   is invalid (all zero length codes or an oversubscribed set of lengths), and
   three if not enough memory.
 *
 * b:    code lengths in bits (all assumed <= BMAX) n:    number of codes
 *(assumed <= N_MAX) s:    number of simple-valued codes (0..s-1) d:    list of
 *base values for non-simple codes e:    list of extra bits for non-simple codes
 *t:    result: starting table m:    maximum lookup bits, returns actual bufnum:
 *the number of the memory pool to fetch memory from
 */
static
int huft_build(unsigned int *b, const unsigned int n,
               const unsigned int s, const unsigned short *d,
               const unsigned char *e, huft_t ** t, unsigned int *m,
               unsigned char bufnum)
{
    unsigned a=0;                /* counter for codes of length k */
    unsigned c[BMAX + 1];        /* bit length count table */
    unsigned eob_len=0;            /* length of end-of-block code (value 256) */
    unsigned f=0;                /* i repeats in table every f entries */
    int g=0;                    /* maximum code length */
    int htl=0;                    /* table level */
    unsigned i=0;                /* counter, current code */
    unsigned j=0;                /* counter */
    int k=0;                    /* number of bits in current code */
    unsigned *p;                /* pointer into c[], b[], or v[] */
    huft_t *q;                    /* points to current table */
    huft_t r;                    /* table entry for structure assignment */
    huft_t *u[BMAX];            /* table stack */
    unsigned v[N_MAX];            /* values in order of bit length */
    int ws[BMAX+1];                /* bits decoded stack */
    int w=0;                    /* bits decoded */
    unsigned x[BMAX + 1];        /* bit offsets, then code stack */
    unsigned *xp;                /* pointer into x */
    int y=0;                    /* number of dummy codes added */
    unsigned z=0;                /* number of entries in current table */

    /* Length of EOB code, if any */
    eob_len = n > 256 ? b[256] : BMAX;

    /* Generate counts for each bit length */
    memset((void *)c, 0, sizeof(c));
    p = b;
    i = n;
    do {
        c[*p]++;    /* assume all entries <= BMAX */
        p++;        /* Can't combine with above line (Solaris bug) */
    } while (--i);
    if (c[0] == n)   /* null input--all zero length codes */
    {
        *t = (huft_t *) NULL;
        *m = 0;
        return 2;
    }

    /* Find minimum and maximum length, bound *m by those */
    for (j = 1; (c[j] == 0) && (j <= BMAX); j++) ;

    k = j; /* minimum code length */
    for (i = BMAX; (c[i] == 0) && i; i--) ;

    g = i; /* maximum code length */
    *m = (*m < j) ? j : ((*m > i) ? i : *m);

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1)
    {
        if ((y -= c[j]) < 0)
            return 2;  /* bad input: more codes than bits */
    }

    if ((y -= c[i]) < 0)
        return 2;

    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;
    xp = x + 2;
    while (--i)   /* note that i == g from above */
    {
        *xp++ = (j += *p++);
    }

    /* Make a table of values in order of bit lengths */
    p = b;
    i = 0;
    do {
        if ((j = *p++) != 0)
            v[x[j]++] = i;
    } while (++i < n);

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;            /* first Huffman code is zero */
    p = v;                    /* grab values in bit order */
    htl = -1;                /* no tables yet--level -1 */
    w = ws[0] = 0;            /* bits decoded */
    u[0] = (huft_t *) NULL;    /* just to keep compilers happy */
    q = (huft_t *) NULL;    /* ditto */
    z = 0;                    /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++)
    {
        a = c[k];
        while (a--)
        {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > ws[htl + 1])
            {
                w = ws[++htl];

                /* compute minimum size table less than or equal to *m bits */
                z = (z = g - w) > *m ? *m : z; /* upper limit on table size */
                if ((f = 1 << (j = k - w)) > a + 1)   /* try a k-w bit table */
                {   /* too few codes for k-w bit table */
                    f -= a + 1; /* deduct codes from patterns left */
                    xp = c + k;
                    while (++j < z)   /* try smaller tables up to z bits */
                    {
                        if ((f <<= 1) <= *++xp)
                            break;  /* enough codes to use up j bits */

                        f -= *xp; /* else deduct codes from patterns */
                    }
                }

                j = ((unsigned)(w + j) > eob_len && (unsigned)w < eob_len)
                    ? eob_len - w : j;      /* make EOB code end at table */
                z = 1 << j;    /* table entries for j-bit table */
                ws[htl+1] = w + j;    /* set bits decoded in stack */

                /* allocate and link in new table */
                q = (huft_t *) wpw_malloc(bufnum,(z + 1) * sizeof(huft_t));
                if(q==0)
                    return 3;

                *t = q + 1;    /* link to list for huft_free() */
                t = &(q->v.t);
                u[htl] = ++q;    /* table starts after link */

                /* connect to last table, if there is one */
                if (htl)
                {
                    x[htl] = i; /* save pattern for backing up */

                    /* bits to dump before this table */
                    r.b = (unsigned char) (w - ws[htl - 1]);
                    r.e = (unsigned char) (16 + j); /* bits in this table */
                    r.v.t = q; /* pointer to this table */
                    j = (i & ((1 << w) - 1)) >> ws[htl - 1];
                    u[htl - 1][j] = r; /* connect to last table */
                }
            }

            /* set up table entry in r */
            r.b = (unsigned char) (k - w);
            if (p >= v + n)
                r.e = 99;  /* out of values--invalid code */
            else if (*p < s)
            {
                r.e = (unsigned char) (*p < 256 ? 16 : 15);    /* 256 is EOB
                                                                  code */
                r.v.n = (unsigned short) (*p++); /* simple code is just the
                                                    value */
            }
            else
            {
                r.e = (unsigned char) e[*p - s]; /* non-simple--look up in lists
                                                  */
                r.v.n = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f)
            {
                q[j] = r;
            }

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i &j; j >>= 1)
            {
                i ^= j;
            }
            i ^= j;

            /* backup over finished tables */
            while ((i & ((1 << w) - 1)) != x[htl])
            {
                w = ws[--htl];
            }
        }
    }

    /* return actual size of base table */
    *m = ws[1];

    /* Return true (1) if we were given an incomplete table */
    return y != 0 && g != 1;
}

/*
 * inflate (decompress) the codes in a deflated (compressed) block. Return an
 *error code or zero if it all goes ok.
 *
 * tl, td: literal/length and distance decoder tables bl, bd: number of bits
 *decoded by tl[] and td[]
 */
static int inflate_codes_resumeCopy = 0;
static int inflate_codes(huft_t * my_tl, huft_t * my_td,
                         const unsigned int my_bl, const unsigned int my_bd,
                         int setup)
{
    static unsigned int e;        /* table entry flag/number of extra bits */
    static unsigned int n, d;    /* length and index for copy */
    static unsigned int w;        /* current gunzip_window position */
    static huft_t *t;            /* pointer to table entry */
    static unsigned int ml, md;    /* masks for bl and bd bits */
    static unsigned int b;        /* bit buffer */
    static unsigned int k;        /* number of bits in bit buffer */
    static huft_t *tl, *td;
    static unsigned int bl, bd;

    if (setup)   /* 1st time we are called, copy in variables */
    {
        tl = my_tl;
        td = my_td;
        bl = my_bl;
        bd = my_bd;
        /* make local copies of globals */
        b = gunzip_bb;                /* initialize bit buffer */
        k = gunzip_bk;
        w = gunzip_outbuf_count;            /* initialize gunzip_window position
                                             */

        /* inflate the coded data */
        ml = mask_bits[bl];    /* precompute masks for speed */
        md = mask_bits[bd];
        return 0; /* Don't actually do anything the first time */
    }

    if (inflate_codes_resumeCopy) goto do_copy;

    while (1)              /* do until end of block */
    {
        b = fill_bitbuffer(b, &k, bl);
        if ((e = (t = tl + ((unsigned) b & ml))->e) > 16)
            do {
                if (e == 99)
                    error_die("inflate_codes error 1");

                b >>= t->b;
                k -= t->b;
                e -= 16;
                b = fill_bitbuffer(b, &k, e);
            } while ((e =
                          (t = t->v.t + ((unsigned) b & mask_bits[e]))->e) > 16);

        b >>= t->b;
        k -= t->b;
        if (e == 16)      /* then it's a literal */
        {
            gunzip_window[w++] = (unsigned char) t->v.n;
            if (w == gunzip_wsize)
            {
                gunzip_outbuf_count = (w);
                w = 0;
                return 1; /* We have a block to read */
            }
        }
        else            /* it's an EOB or a length */
        {   /* exit if end of block */
            if (e == 15)
                break;

            /* get length of block to copy */
            b = fill_bitbuffer(b, &k, e);
            n = t->v.n + ((unsigned) b & mask_bits[e]);
            b >>= e;
            k -= e;

            /* decode distance of block to copy */
            b = fill_bitbuffer(b, &k, bd);
            if ((e = (t = td + ((unsigned) b & md))->e) > 16)
                do {
                    if (e == 99)
                        error_die("inflate_codes error 2");

                    b >>= t->b;
                    k -= t->b;
                    e -= 16;
                    b = fill_bitbuffer(b, &k, e);
                } while ((e =
                              (t =
                                   t->v.t + ((unsigned) b & mask_bits[e]))->e) > 16);

            b >>= t->b;
            k -= t->b;
            b = fill_bitbuffer(b, &k, e);
            d = w - t->v.n - ((unsigned) b & mask_bits[e]);
            b >>= e;
            k -= e;

            /* do the copy */
do_copy:        do {
                n -= (e =
                          (e =
                               gunzip_wsize - ((d &= gunzip_wsize - 1) > w ? d : w)) > n ? n : e);
                /* copy to new buffer to prevent possible overwrite */
                if (w - d >= e)      /* (this test assumes unsigned comparison)
                                      */
                {
                    memcpy(gunzip_window + w, gunzip_window + d, e);
                    w += e;
                    d += e;
                }
                else
                {
                    /* do it slow to avoid memcpy() overlap */
                    /* !NOMEMCPY */
                    do {
                        gunzip_window[w++] = gunzip_window[d++];
                    } while (--e);
                }

                if (w == gunzip_wsize)
                {
                    gunzip_outbuf_count = (w);
                    if (n) inflate_codes_resumeCopy = 1;
                    else inflate_codes_resumeCopy = 0;

                    w = 0;
                    return 1;
                }
            } while (n);
            inflate_codes_resumeCopy = 0;
        }
    }

    /* restore the globals from the locals */
    gunzip_outbuf_count = w;    /* restore global gunzip_window pointer */
    gunzip_bb = b;                /* restore global bit buffer */
    gunzip_bk = k;

    /* normally just after call to inflate_codes, but save code by putting it
       here */
    /* free the decoding tables, return */
    huft_free(tl,HUFT_MMP1);
    huft_free(td,HUFT_MMP2);

    /* done */
    return 0;
}

static int inflate_stored(int my_n, int my_b_stored, int my_k_stored, int setup)
{
    static unsigned int n, b_stored, k_stored, w;
    if (setup)
    {
        n = my_n;
        b_stored = my_b_stored;
        k_stored = my_k_stored;
        w = gunzip_outbuf_count;    /* initialize gunzip_window position */
        return 0; /* Don't do anything first time */
    }

    /* read and output the compressed data */
    while (n--)
    {
        b_stored = fill_bitbuffer(b_stored, &k_stored, 8);
        gunzip_window[w++] = (unsigned char) b_stored;
        if (w == gunzip_wsize)
        {
            gunzip_outbuf_count = (w);
            w = 0;
            b_stored >>= 8;
            k_stored -= 8;
            return 1; /* We have a block */
        }

        b_stored >>= 8;
        k_stored -= 8;
    }

    /* restore the globals from the locals */
    gunzip_outbuf_count = w;        /* restore global gunzip_window pointer */
    gunzip_bb = b_stored;    /* restore global bit buffer */
    gunzip_bk = k_stored;
    return 0; /* Finished */
}

/*
 * decompress an inflated block e: last block flag
 *
 * GLOBAL VARIABLES: bb, kk,
 */
/* Return values: -1 = inflate_stored, -2 = inflate_codes */
static int inflate_block(int *e)
{
    unsigned t;            /* block type */
    unsigned int b;    /* bit buffer */
    unsigned int k;    /* number of bits in bit buffer */

    /* make local bit buffer */

    b = gunzip_bb;
    k = gunzip_bk;

    /* read in last block bit */
    b = fill_bitbuffer(b, &k, 1);
    *e = (int) b & 1;
    b >>= 1;
    k -= 1;

    /* read in block type */
    b = fill_bitbuffer(b, &k, 2);
    t = (unsigned) b & 3;
    b >>= 2;
    k -= 2;

    /* restore the global bit buffer */
    gunzip_bb = b;
    gunzip_bk = k;

    /* inflate that block type */
    switch (t)
    {
    case 0:            /* Inflate stored */
    {
        unsigned int n=0;    /* number of bytes in block */

        /* make local copies of globals */
        unsigned int b_stored=gunzip_bb;    /* bit buffer */
        unsigned int k_stored=gunzip_bk;    /* number of bits in bit buffer */

        /* go to byte boundary */
        n = k_stored & 7;
        b_stored >>= n;
        k_stored -= n;

        /* get the length and its complement */
        b_stored = fill_bitbuffer(b_stored, &k_stored, 16);
        n = ((unsigned) b_stored & 0xffff);
        b_stored >>= 16;
        k_stored -= 16;

        b_stored = fill_bitbuffer(b_stored, &k_stored, 16);
        if (n != (unsigned) ((~b_stored) & 0xffff))
            return 1;    /* error in compressed data */

        b_stored >>= 16;
        k_stored -= 16;

        inflate_stored(n, b_stored, k_stored, 1); /* Setup inflate_stored */
        return -1;
    }
    case 1:            /* Inflate fixed decompress an inflated type 1 (fixed
                          Huffman codes) block.  We should either replace this
                          with a custom decoder, or at least precompute the
                          Huffman tables.
                        */
    {
        int i;            /* temporary variable */
        huft_t *tl;        /* literal/length code table */
        huft_t *td;        /* distance code table */
        unsigned int bl;            /* lookup bits for tl */
        unsigned int bd;            /* lookup bits for td */
        unsigned int l[288];    /* length list for huft_build */

        /* set up literal table */
        for (i = 0; i < 144; i++)
        {
            l[i] = 8;
        }
        for (; i < 256; i++)
        {
            l[i] = 9;
        }
        for (; i < 280; i++)
        {
            l[i] = 7;
        }
        for (; i < 288; i++)      /* make a complete, but wrong code set */
        {
            l[i] = 8;
        }
        bl = 7;
        if ((i = huft_build(l, 288, 257, cplens, cplext, &tl, &bl,HUFT_MMP1)) != 0)
            return i;

        /* set up distance table */
        for (i = 0; i < 30; i++)      /* make an incomplete code set */
        {
            l[i] = 5;
        }
        bd = 5;
        if ((i = huft_build(l, 30, 0, cpdist, cpdext, &td, &bd,HUFT_MMP2)) > 1)
        {
            huft_free(tl,HUFT_MMP1);
            return i;
        }

        /* decompress until an end-of-block code */
        inflate_codes(tl, td, bl, bd, 1); /* Setup inflate_codes */

        /* huft_free code moved into inflate_codes */

        return -2;
    }
    case 2:            /* Inflate dynamic */
    {
        const int dbits = 6;    /* bits in base distance lookup table */
        const int lbits = 9;    /* bits in base literal/length lookup table */

        huft_t *tl;        /* literal/length code table */
        huft_t *td;        /* distance code table */
        unsigned int i;            /* temporary variables */
        unsigned int j;
        unsigned int l;        /* last length */
        unsigned int m;        /* mask for bit lengths table */
        unsigned int n;        /* number of lengths to get */
        unsigned int bl;            /* lookup bits for tl */
        unsigned int bd;            /* lookup bits for td */
        unsigned int nb;    /* number of bit length codes */
        unsigned int nl;    /* number of literal/length codes */
        unsigned int nd;    /* number of distance codes */

        unsigned int ll[286 + 30];    /* literal/length and distance code
                                         lengths */
        unsigned int b_dynamic;    /* bit buffer */
        unsigned int k_dynamic;    /* number of bits in bit buffer */

        /* make local bit buffer */
        b_dynamic = gunzip_bb;
        k_dynamic = gunzip_bk;

        /* read in table lengths */
        b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 5);
        nl = 257 + ((unsigned int) b_dynamic & 0x1f);    /* number of
                                                            literal/length codes
                                                          */

        b_dynamic >>= 5;
        k_dynamic -= 5;
        b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 5);
        nd = 1 + ((unsigned int) b_dynamic & 0x1f);    /* number of distance
                                                          codes */

        b_dynamic >>= 5;
        k_dynamic -= 5;
        b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 4);
        nb = 4 + ((unsigned int) b_dynamic & 0xf);    /* number of bit length
                                                         codes */

        b_dynamic >>= 4;
        k_dynamic -= 4;
        if (nl > 286 || nd > 30)
            return 1;    /* bad lengths */

        /* read in bit-length-code lengths */
        for (j = 0; j < nb; j++)
        {
            b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 3);
            ll[border[j]] = (unsigned int) b_dynamic & 7;
            b_dynamic >>= 3;
            k_dynamic -= 3;
        }
        for (; j < 19; j++)
        {
            ll[border[j]] = 0;
        }

        /* build decoding table for trees--single level, 7 bit lookup */
        bl = 7;
        i = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl,HUFT_MMP1);
        if (i != 0)
        {
            if (i == 1)
                huft_free(tl,HUFT_MMP1);

            return i;    /* incomplete code set */
        }

        /* read in literal and distance code lengths */
        n = nl + nd;
        m = mask_bits[bl];
        i = l = 0;
        while ((unsigned int) i < n)
        {
            b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, (unsigned int)bl);
            j = (td = tl + ((unsigned int) b_dynamic & m))->b;
            b_dynamic >>= j;
            k_dynamic -= j;
            j = td->v.n;
            if (j < 16)      /* length of code in bits (0..15) */
                ll[i++] = l = j;    /* save last length in l */
            else if (j == 16)        /* repeat last length 3 to 6 times */
            {
                b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 2);
                j = 3 + ((unsigned int) b_dynamic & 3);
                b_dynamic >>= 2;
                k_dynamic -= 2;
                if ((unsigned int) i + j > n)
                    return 1;

                while (j--)
                {
                    ll[i++] = l;
                }
            }
            else if (j == 17)        /* 3 to 10 zero length codes */
            {
                b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 3);
                j = 3 + ((unsigned int) b_dynamic & 7);
                b_dynamic >>= 3;
                k_dynamic -= 3;
                if ((unsigned int) i + j > n)
                    return 1;

                while (j--)
                {
                    ll[i++] = 0;
                }
                l = 0;
            }
            else        /* j == 18: 11 to 138 zero length codes */
            {
                b_dynamic = fill_bitbuffer(b_dynamic, &k_dynamic, 7);
                j = 11 + ((unsigned int) b_dynamic & 0x7f);
                b_dynamic >>= 7;
                k_dynamic -= 7;
                if ((unsigned int) i + j > n)
                    return 1;

                while (j--)
                {
                    ll[i++] = 0;
                }
                l = 0;
            }
        }

        /* free decoding table for trees */
        huft_free(tl,HUFT_MMP1);

        /* restore the global bit buffer */
        gunzip_bb = b_dynamic;
        gunzip_bk = k_dynamic;

        /* build the decoding tables for literal/length and distance codes */
        bl = lbits;

        if ((i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl,HUFT_MMP1)) != 0)
        {
            if (i == 1)
            {
                error_die("Incomplete literal tree");
                huft_free(tl,HUFT_MMP1);
            }

            return i;    /* incomplete code set */
        }

        bd = dbits;
        if ((i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd,HUFT_MMP2)) != 0)
        {
            if (i == 1)
            {
                error_die("incomplete distance tree");
                huft_free(td,HUFT_MMP2);
            }

            huft_free(tl,HUFT_MMP1);
            return i;    /* incomplete code set */
        }

        /* decompress until an end-of-block code */
        inflate_codes(tl, td, bl, bd, 1); /* Setup inflate_codes */

        /* huft_free code moved into inflate_codes */

        return -2;
    }
    default:
        /* bad block type */
        error_die("bad block type");
    }
    return 0;
}

static void calculate_gunzip_crc(void)
{
    unsigned int n;
    for (n = 0; n < gunzip_outbuf_count; n++)
    {
        gunzip_crc = crc_table[((int) gunzip_crc ^ (gunzip_window[n])) & 0xff]
                     ^ (gunzip_crc >> 8);
    }
    gunzip_bytes_out += gunzip_outbuf_count;
}

static int inflate_get_next_window_method = -1; /* Method == -1 for stored, -2
                                                   for codes */
static int inflate_get_next_window_e = 0;
static int inflate_get_next_window_needAnotherBlock = 1;

static int inflate_get_next_window(void)
{
    gunzip_outbuf_count = 0;

    while(1)
    {
        int ret=0;
        if (inflate_get_next_window_needAnotherBlock)
        {
            if(inflate_get_next_window_e)
            {
                calculate_gunzip_crc();
                inflate_get_next_window_e = 0;
                inflate_get_next_window_needAnotherBlock = 1;
                return 0;
            } /* Last block */

            inflate_get_next_window_method = inflate_block(&inflate_get_next_window_e);
            inflate_get_next_window_needAnotherBlock = 0;
        }

        switch (inflate_get_next_window_method)
        {
        case -1:    ret = inflate_stored(0,0,0,0);
            break;
        case -2:    ret = inflate_codes(0,0,0,0,0);
            break;
        default:
            error_die("inflate error");
        }

        if (ret == 1)
        {
            calculate_gunzip_crc();
            return 1; /* More data left */
        }
        else inflate_get_next_window_needAnotherBlock = 1;    /* End of that
                                                                 block */
    }
    /* Doesnt get here */
}

/* Initialise bytebuffer, be careful not to overfill the buffer */
static void inflate_init(unsigned int bufsize)
{
    /* Set the bytebuffer size, default is same as gunzip_wsize */
    bytebuffer_max = bufsize + 8;
    bytebuffer_offset = 4;
    bytebuffer_size = 0;
}

static void inflate_cleanup(void)
{
    /* free(bytebuffer); */
}

USE_DESKTOP(long long) static int
inflate_unzip(struct mbreader_t *in,char* outbuffer,uint32_t outbuflen)
{
    USE_DESKTOP(long long total = 0; )
    typedef void (*sig_type)(int);

    /* Allocate all global buffers (for DYN_ALLOC option) */
    gunzip_outbuf_count = 0;
    gunzip_bytes_out = 0;
    gunzip_src_md = in;

    /* initialize gunzip_window, bit buffer */
    gunzip_bk = 0;
    gunzip_bb = 0;

    /* Create the crc table */
    gunzip_crc = ~0;

    /* Allocate space for buffer */
    while(1)
    {
        int ret = inflate_get_next_window();
        if((signed int)outbuflen-(signed int)gunzip_outbuf_count<0)
        {
            error_msg("write_error");
		#ifdef TRIM_FILE_ON_ERROR
            return USE_DESKTOP(total) + 0;
        #else
        	return -1;
        #endif
        }

        memcpy(outbuffer,gunzip_window,gunzip_outbuf_count);
        outbuffer+=sizeof(char)*gunzip_outbuf_count;
        ifl_total+=sizeof(char)*gunzip_outbuf_count;
        outbuflen-=gunzip_outbuf_count;
        USE_DESKTOP(total += gunzip_outbuf_count; )
        if (ret == 0) break;
    }

    /* Store unused bytes in a global buffer so calling applets can access it */
    if (gunzip_bk >= 8)
    {
        /* Undo too much lookahead. The next read will be byte aligned so we can
           discard unused bits in the last meaningful byte. */
        bytebuffer_offset--;
        bytebuffer[bytebuffer_offset] = gunzip_bb & 0xff;
        gunzip_bb >>= 8;
        gunzip_bk -= 8;
    }

    return USE_DESKTOP(total) + 0;
}

USE_DESKTOP(long long) static int
inflate_gunzip(struct mbreader_t *in,char* outbuffer,uint32_t outbuflen)
{
    uint32_t stored_crc = 0;
    unsigned int count;
    USE_DESKTOP(long long total = ) inflate_unzip(in, outbuffer,outbuflen);

    USE_DESKTOP(if (total < 0) return total;

                )

    /* top up the input buffer with the rest of the trailer */
    count = bytebuffer_size - bytebuffer_offset;
    if (count < 8)
    {
        xread(in, &bytebuffer[bytebuffer_size], 8 - count);
        bytebuffer_size += 8 - count;
    }

    for (count = 0; count != 4; count++)
    {
        stored_crc |= (bytebuffer[bytebuffer_offset] << (count * 8));
        bytebuffer_offset++;
    }

    /* Validate decompression - crc */
    if (stored_crc != (~gunzip_crc))
    {
        error_msg("crc error");
    
	#ifdef TRIM_FILE_ON_ERROR
	    return USE_DESKTOP(total) + 0;
	#else
		return -1;
	#endif
    }

    /* Validate decompression - size */
    if ((signed int)gunzip_bytes_out !=
        (bytebuffer[bytebuffer_offset] | (bytebuffer[bytebuffer_offset+1] << 8) |
         (bytebuffer[bytebuffer_offset+2] << 16) | (bytebuffer[bytebuffer_offset+3] << 24)))
    {
        error_msg("incorrect length");
        return -1;
    }

    return USE_DESKTOP(total) + 0;
}

/*An allocated memory buffer at least 0x13100 (72448) bytes long*/
uint32_t decompress(const char *inbuffer,uint32_t inbuflen,char* outbuffer,uint32_t outbuflen,uint32_t offset,char* membuf)
{
    signed char status=0;
    int exitcode=0;
    struct mbreader_t src_md;
    ifl_total=0;
    /* reset statics */
    inflate_codes_resumeCopy = 0;
    inflate_get_next_window_method = -1; /* Method == -1 for stored, -2 for
                                            codes */
    inflate_get_next_window_e = 0;
    inflate_get_next_window_needAnotherBlock = 1;
    /* init */
    inflate_init(0x8000-8);
    /*Memory init*/
    huftbuffer1=membuf;
    huftbuffer2=membuf+0x2A00;
    gunzip_window=membuf+0x2A00+0xA00;
    bytebuffer=membuf+0x2A00+0xA00+0x8000;
    wpw_init_mempool_pdm(HUFT_MMP1,(unsigned char*)huftbuffer1,0x2A00);
    wpw_init_mempool_pdm(HUFT_MMP2,(unsigned char*)huftbuffer2,0xA00);

	/* Initialize memory buffer reader */
    src_md.ptr = inbuffer;
    src_md.size = inbuflen;
    src_md.offset = offset;
    
    if ((exitcode=xread_char(&src_md)) == 0x1f)
    {
        unsigned char magic2;
        magic2 = xread_char(&src_md);
        if (magic2 == 0x8b)
        {
            check_header_gzip(&src_md); /* FIXME: xfunc? _or_die? */
            status = inflate_gunzip(&src_md, outbuffer,outbuflen);
        }
        else
        {
            error_msg("invalid magic");
            exitcode = -1;
        }

        if (status < 0)
        {
            error_msg("error inflating");
            exitcode = -1;
        }
    }
    else
    {
        error_msg("invalid magic");
        exitcode = -1;
    }

    inflate_cleanup();
    wpw_destroy_mempool(HUFT_MMP1);
    wpw_destroy_mempool(HUFT_MMP2);

    if(exitcode==-1)
        return 0;

    return ifl_total;
}
