/* n2_99.ch -- implementation of the NRV2[BDE]-99 compression algorithms

   This file is part of the UCL data compression library.

   Copyright (C) 1996-2002 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/ucl/
 */



#include <ucl/uclconf.h>
#include <ucl/ucl.h>
#include "ucl_conf.h"

#if 0
#undef UCL_DEBUG
#define UCL_DEBUG
#endif

#include <stdio.h>

#if 0 && !defined(UCL_DEBUG)
#undef NDEBUG
#include <assert.h>
#endif


/***********************************************************************
//
************************************************************************/

#if 0
#define N       (128*1024ul)        /* size of ring buffer */
#else
#define N       (1024*1024ul)       /* size of ring buffer */
#define SWD_USE_MALLOC
#define SWD_HSIZE   65536ul
#endif
#define THRESHOLD       1           /* lower limit for match length */
#define F            2048           /* upper limit for match length */

#if defined(NRV2B)
#  define UCL_COMPRESS_T                ucl_nrv2b_t
#  define ucl_swd_t                     ucl_nrv2b_swd_t
#  define ucl_nrv_99_compress           ucl_nrv2b_99_compress
#  define M2_MAX_OFFSET                 0xd00
#elif defined(NRV2D)
#  define UCL_COMPRESS_T                ucl_nrv2d_t
#  define ucl_swd_t                     ucl_nrv2d_swd_t
#  define ucl_nrv_99_compress           ucl_nrv2d_99_compress
#  define M2_MAX_OFFSET                 0x500
#elif defined(NRV2E)
#  define UCL_COMPRESS_T                ucl_nrv2e_t
#  define ucl_swd_t                     ucl_nrv2e_swd_t
#  define ucl_nrv_99_compress           ucl_nrv2e_99_compress
#  define M2_MAX_OFFSET                 0x500
#else
#  error
#endif
#define ucl_swd_p       ucl_swd_t * __UCL_MMODEL

#if 0
#  define HEAD3(b,p) \
    ((((((ucl_uint32)b[p]<<3)^b[p+1])<<3)^b[p+2]) & (SWD_HSIZE-1))
#endif
#if 0 && defined(UCL_UNALIGNED_OK_4) && (UCL_BYTE_ORDER == UCL_LITTLE_ENDIAN)
#  define HEAD3(b,p) \
    (((* (ucl_uint32p) &b[p]) ^ ((* (ucl_uint32p) &b[p])>>10)) & (SWD_HSIZE-1))
#endif

#include "ucl_mchw.ch"


/***********************************************************************
//
************************************************************************/

static void code_prefix_ss11(UCL_COMPRESS_T *c, ucl_uint32 i)
{
    if (i >= 2)
    {
        ucl_uint32 t = 4;
        i += 2;
        do {
            t <<= 1;
        } while (i >= t);
        t >>= 1;
        do {
            t >>= 1;
            bbPutBit(c, (i & t) ? 1 : 0);
            bbPutBit(c, 0);
        } while (t > 2);
    }
    bbPutBit(c, (unsigned)i & 1);
    bbPutBit(c, 1);
}


#if defined(NRV2D) || defined(NRV2E)
static void code_prefix_ss12(UCL_COMPRESS_T *c, ucl_uint32 i)
{
    if (i >= 2)
    {
        ucl_uint32 t = 2;
        do {
            i -= t;
            t <<= 2;
        } while (i >= t);
        do {
            t >>= 1;
            bbPutBit(c, (i & t) ? 1 : 0);
            bbPutBit(c, 0);
            t >>= 1;
            bbPutBit(c, (i & t) ? 1 : 0);
        } while (t > 2);
    }
    bbPutBit(c, (unsigned)i & 1);
    bbPutBit(c, 1);
}
#endif


static void
code_match(UCL_COMPRESS_T *c, ucl_uint m_len, const ucl_uint m_off)
{
    unsigned m_low = 0;

    while (m_len > c->conf.max_match)
    {
        code_match(c, c->conf.max_match - 3, m_off);
        m_len -= c->conf.max_match - 3;
    }

    c->match_bytes += m_len;
    if (m_len > c->result[3])
        c->result[3] = m_len;
    if (m_off > c->result[1])
        c->result[1] = m_off;

    bbPutBit(c, 0);

#if defined(NRV2B)
    if (m_off == c->last_m_off)
    {
        bbPutBit(c, 0);
        bbPutBit(c, 1);
    }
    else
    {
        code_prefix_ss11(c, 1 + ((m_off - 1) >> 8));
        bbPutByte(c, (unsigned)m_off - 1);
    }
    m_len = m_len - 1 - (m_off > M2_MAX_OFFSET);
    if (m_len >= 4)
    {
        bbPutBit(c,0);
        bbPutBit(c,0);
        code_prefix_ss11(c, m_len - 4);
    }
    else
    {
        bbPutBit(c, m_len > 1);
        bbPutBit(c, (unsigned)m_len & 1);
    }
#elif defined(NRV2D)
    m_len = m_len - 1 - (m_off > M2_MAX_OFFSET);
    assert(m_len > 0);
    m_low = (m_len >= 4) ? 0u : (unsigned) m_len;
    if (m_off == c->last_m_off)
    {
        bbPutBit(c, 0);
        bbPutBit(c, 1);
        bbPutBit(c, m_low > 1);
        bbPutBit(c, m_low & 1);
    }
    else
    {
        code_prefix_ss12(c, 1 + ((m_off - 1) >> 7));
        bbPutByte(c, ((((unsigned)m_off - 1) & 0x7f) << 1) | ((m_low > 1) ? 0 : 1));
        bbPutBit(c, m_low & 1);
    }
    if (m_len >= 4)
        code_prefix_ss11(c, m_len - 4);
#elif defined(NRV2E)
    m_len = m_len - 1 - (m_off > M2_MAX_OFFSET);
    assert(m_len > 0);
    m_low = (m_len <= 2);
    if (m_off == c->last_m_off)
    {
        bbPutBit(c, 0);
        bbPutBit(c, 1);
        bbPutBit(c, m_low);
    }
    else
    {
        code_prefix_ss12(c, 1 + ((m_off - 1) >> 7));
        bbPutByte(c, ((((unsigned)m_off - 1) & 0x7f) << 1) | (m_low ^ 1));
    }
    if (m_low)
        bbPutBit(c, (unsigned)m_len - 1);
    else if (m_len <= 4)
    {
        bbPutBit(c, 1);
        bbPutBit(c, (unsigned)m_len - 3);
    }
    else
    {
        bbPutBit(c, 0);
        code_prefix_ss11(c, m_len - 5);
    }
#else
#  error
#endif

    c->last_m_off = m_off;
    UCL_UNUSED(m_low);
}


static void
code_run(UCL_COMPRESS_T *c, const ucl_byte *ii, ucl_uint lit)
{
    if (lit == 0)
        return;
    c->lit_bytes += lit;
    if (lit > c->result[5])
        c->result[5] = lit;
    do {
        bbPutBit(c, 1);
        bbPutByte(c, *ii++);
    } while (--lit > 0);
}


/***********************************************************************
//
************************************************************************/

static int
len_of_coded_match(UCL_COMPRESS_T *c, ucl_uint m_len, ucl_uint m_off)
{
    int b;
    if (m_len < 2 || (m_len == 2 && (m_off > M2_MAX_OFFSET))
        || m_off > c->conf.max_offset)
        return -1;
    assert(m_off > 0);

    m_len = m_len - 2 - (m_off > M2_MAX_OFFSET);

    if (m_off == c->last_m_off)
        b = 1 + 2;
    else
    {
#if defined(NRV2B)
        b = 1 + 10;
        m_off = (m_off - 1) >> 8;
        while (m_off > 0)
        {
            b += 2;
            m_off >>= 1;
        }
#elif defined(NRV2D) || defined(NRV2E)
        b = 1 + 9;
        m_off = (m_off - 1) >> 7;
        while (m_off > 0)
        {
            b += 3;
            m_off >>= 2;
        }
#else
#  error
#endif
    }

#if defined(NRV2B) || defined(NRV2D)
    b += 2;
    if (m_len < 3)
        return b;
    m_len -= 3;
#elif defined(NRV2E)
    b += 2;
    if (m_len < 2)
        return b;
    if (m_len < 4)
        return b + 1;
    m_len -= 4;
#else
#  error
#endif
    do {
        b += 2;
        m_len >>= 1;
    } while (m_len > 0);

    return b;
}


/***********************************************************************
//
************************************************************************/

#if !defined(NDEBUG)
static
void assert_match( const ucl_swd_p swd, ucl_uint m_len, ucl_uint m_off )
{
    const UCL_COMPRESS_T *c = swd->c;
    ucl_uint d_off;

    assert(m_len >= 2);
    if (m_off <= (ucl_uint) (c->bp - c->in))
    {
        assert(c->bp - m_off + m_len < c->ip);
        assert(ucl_memcmp(c->bp, c->bp - m_off, m_len) == 0);
    }
    else
    {
        assert(swd->dict != NULL);
        d_off = m_off - (ucl_uint) (c->bp - c->in);
        assert(d_off <= swd->dict_len);
        if (m_len > d_off)
        {
            assert(ucl_memcmp(c->bp, swd->dict_end - d_off, d_off) == 0);
            assert(c->in + m_len - d_off < c->ip);
            assert(ucl_memcmp(c->bp + d_off, c->in, m_len - d_off) == 0);
        }
        else
        {
            assert(ucl_memcmp(c->bp, swd->dict_end - d_off, m_len) == 0);
        }
    }
}
#else
#  define assert_match(a,b,c)   ((void)0)
#endif


#if defined(SWD_BEST_OFF)

static void
better_match ( const ucl_swd_p swd, ucl_uint *m_len, ucl_uint *m_off )
{
}

#endif


/***********************************************************************
//
************************************************************************/

UCL_PUBLIC(int)
ucl_nrv_99_compress        ( const ucl_bytep in, ucl_uint in_len,
                                   ucl_bytep out, ucl_uintp out_len,
                                   ucl_progress_callback_p cb,
                                   int level,
                             const struct ucl_compress_config_p conf,
                                   ucl_uintp result)
{
    const ucl_byte *ii;
    ucl_uint lit;
    ucl_uint m_len, m_off;
    UCL_COMPRESS_T c_buffer;
    UCL_COMPRESS_T * const c = &c_buffer;
#undef swd
#if 1 && defined(SWD_USE_MALLOC)
    ucl_swd_t the_swd;
#   define swd (&the_swd)
#else
    ucl_swd_p swd;
#endif
    ucl_uint result_buffer[16];
    int r;

    struct swd_config_t
    {
        unsigned try_lazy;
        ucl_uint good_length;
        ucl_uint max_lazy;
        ucl_uint nice_length;
        ucl_uint max_chain;
        ucl_uint32 flags;
        ucl_uint32 max_offset;
    };
    const struct swd_config_t *sc;
    static const struct swd_config_t swd_config[10] = {
        /* faster compression */
        {   0,   0,   0,   8,    4,   0,  48*1024L },
        {   0,   0,   0,  16,    8,   0,  48*1024L },
        {   0,   0,   0,  32,   16,   0,  48*1024L },
        {   1,   4,   4,  16,   16,   0,  48*1024L },
        {   1,   8,  16,  32,   32,   0,  48*1024L },
        {   1,   8,  16, 128,  128,   0,  48*1024L },
        {   2,   8,  32, 128,  256,   0, 128*1024L },
        {   2,  32, 128,   F, 2048,   1, 128*1024L },
        {   2,  32, 128,   F, 2048,   1, 256*1024L },
        {   2,   F,   F,   F, 4096,   1, N }
        /* max. compression */
    };

    if (level < 1 || level > 10)
        return UCL_E_INVALID_ARGUMENT;
    sc = &swd_config[level - 1];

    memset(c, 0, sizeof(*c));
    c->ip = c->in = in;
    c->in_end = in + in_len;
    c->out = out;
    if (cb && cb->callback)
        c->cb = cb;
    cb = NULL;
    c->result = result ? result : (ucl_uintp) result_buffer;
    memset(c->result, 0, 16*sizeof(*c->result));
    c->result[0] = c->result[2] = c->result[4] = UCL_UINT_MAX;
    result = NULL;
    memset(&c->conf, 0xff, sizeof(c->conf));
    if (conf)
        memcpy(&c->conf, conf, sizeof(c->conf));
    conf = NULL;
    r = bbConfig(c, 0, 8);
    if (r == 0)
        r = bbConfig(c, c->conf.bb_endian, c->conf.bb_size);
    if (r != 0)
        return UCL_E_INVALID_ARGUMENT;
    c->bb_op = out;

    ii = c->ip;             /* point to start of literal run */
    lit = 0;

#if !defined(swd)
    swd = (ucl_swd_p) ucl_alloc(1, ucl_sizeof(*swd));
    if (!swd)
        return UCL_E_OUT_OF_MEMORY;
#endif
    swd->f = UCL_MIN(F, c->conf.max_match);
    swd->n = UCL_MIN(N, sc->max_offset);
    if (c->conf.max_offset != UCL_UINT_MAX)
        swd->n = UCL_MIN(N, c->conf.max_offset);
    if (in_len >= 256 && in_len < swd->n)
        swd->n = in_len;
    if (swd->f < 8 || swd->n < 256)
        return UCL_E_INVALID_ARGUMENT;
    r = init_match(c,swd,NULL,0,sc->flags);
    if (r != UCL_E_OK)
    {
#if !defined(swd)
        ucl_free(swd);
#endif
        return r;
    }
    if (sc->max_chain > 0)
        swd->max_chain = sc->max_chain;
    if (sc->nice_length > 0)
        swd->nice_length = sc->nice_length;
    if (c->conf.max_match < swd->nice_length)
        swd->nice_length = c->conf.max_match;

    if (c->cb)
        (*c->cb->callback)(0,0,-1,c->cb->user);

    c->last_m_off = 1;
    r = find_match(c,swd,0,0);
    if (r != UCL_E_OK)
        return r;
    while (c->look > 0)
    {
        ucl_uint ahead;
        ucl_uint max_ahead;
        int l1, l2;

        c->codesize = c->bb_op - out;

        m_len = c->m_len;
        m_off = c->m_off;

        assert(c->bp == c->ip - c->look);
        assert(c->bp >= in);
        if (lit == 0)
            ii = c->bp;
        assert(ii + lit == c->bp);
        assert(swd->b_char == *(c->bp));

        if (m_len < 2 || (m_len == 2 && (m_off > M2_MAX_OFFSET))
            || m_off > c->conf.max_offset)
        {
            /* a literal */
            lit++;
            swd->max_chain = sc->max_chain;
            r = find_match(c,swd,1,0);
            assert(r == 0);
            continue;
        }

    /* a match */
#if defined(SWD_BEST_OFF)
        if (swd->use_best_off)
            better_match(swd,&m_len,&m_off);
#endif
        assert_match(swd,m_len,m_off);

        /* shall we try a lazy match ? */
        ahead = 0;
        if (sc->try_lazy <= 0 || m_len >= sc->max_lazy || m_off == c->last_m_off)
        {
            /* no */
            l1 = 0;
            max_ahead = 0;
        }
        else
        {
            /* yes, try a lazy match */
            l1 = len_of_coded_match(c,m_len,m_off);
            assert(l1 > 0);
            max_ahead = UCL_MIN(sc->try_lazy, m_len - 1);
        }

        while (ahead < max_ahead && c->look > m_len)
        {
            if (m_len >= sc->good_length)
                swd->max_chain = sc->max_chain >> 2;
            else
                swd->max_chain = sc->max_chain;
            r = find_match(c,swd,1,0);
            ahead++;

            assert(r == 0);
            assert(c->look > 0);
            assert(ii + lit + ahead == c->bp);

            if (c->m_len < 2)
                continue;
#if defined(SWD_BEST_OFF)
            if (swd->use_best_off)
                better_match(swd,&c->m_len,&c->m_off);
#endif
            l2 = len_of_coded_match(c,c->m_len,c->m_off);
            if (l2 < 0)
                continue;
#if 1
            if (l1 + (int)(ahead + c->m_len - m_len) * 5 > l2 + (int)(ahead) * 9)
#else
            if (l1 > l2)
#endif
            {
                c->lazy++;
                assert_match(swd,c->m_len,c->m_off);

#if 0
                if (l3 > 0)
                {
                    /* code previous run */
                    code_run(c,ii,lit);
                    lit = 0;
                    /* code shortened match */
                    code_match(c,ahead,m_off);
                }
                else
#endif
                {
                    lit += ahead;
                    assert(ii + lit == c->bp);
                }
                goto lazy_match_done;
            }
        }

        assert(ii + lit + ahead == c->bp);

        /* 1 - code run */
        code_run(c,ii,lit);
        lit = 0;

        /* 2 - code match */
        code_match(c,m_len,m_off);
        swd->max_chain = sc->max_chain;
        r = find_match(c,swd,m_len,1+ahead);
        assert(r == 0);

lazy_match_done: ;
    }

    /* store final run */
    code_run(c,ii,lit);

    /* EOF */
    bbPutBit(c, 0);
#if defined(NRV2B)
    code_prefix_ss11(c, UCL_UINT32_C(0x1000000));
    bbPutByte(c, 0xff);
#elif defined(NRV2D) || defined(NRV2E)
    code_prefix_ss12(c, UCL_UINT32_C(0x1000000));
    bbPutByte(c, 0xff);
#else
#  error
#endif
    bbFlushBits(c, 0);

    assert(c->textsize == in_len);
    c->codesize = c->bb_op - out;
    *out_len = c->bb_op - out;
    if (c->cb)
        (*c->cb->callback)(c->textsize,c->codesize,4,c->cb->user);

#if 0
    printf("%7ld %7ld -> %7ld   %7ld %7ld   %ld  (max: %d %d %d)\n",
          (long) c->textsize, (long) in_len, (long) c->codesize,
           c->match_bytes, c->lit_bytes,  c->lazy,
           c->result[1], c->result[3], c->result[5]);
#endif
    assert(c->lit_bytes + c->match_bytes == in_len);

    swd_exit(swd);
#if !defined(swd)
    ucl_free(swd);
#endif
    return UCL_E_OK;
#undef swd
}


/*
vi:ts=4:et
*/

