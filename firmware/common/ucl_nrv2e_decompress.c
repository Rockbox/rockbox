/* Standalone version of ucl_nrv2e_decompress_8 from UCL library
 * Original copyright notice:
 */
/* n2e_d.c -- implementation of the NRV2E decompression algorithm

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

#include "ucl_nrv2e_decompress.h"

#define UCL_UINT32_C(c) c ## U
#define fail(x, r) do if(x) { *dst_len = olen; return r; } while(0)

#define getbit(bb) \
    (((bb = bb & 0x7f ? bb*2 : ((unsigned)src[ilen++]*2+1)) >> 8) & 1)

int ucl_nrv2e_decompress_8(const uint8_t* src, uint32_t src_len,
                           uint8_t* dst, uint32_t* dst_len)
{
    uint32_t bb = 0;
    uint32_t ilen = 0, olen = 0, last_m_off = 1;
    uint32_t oend = *dst_len;

    for (;;)
    {
        uint32_t m_off, m_len;

        while (getbit(bb))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
            dst[olen++] = src[ilen++];
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > UCL_UINT32_C(0xffffff) + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb)) break;
            m_off = (m_off-1)*2 + getbit(bb);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == UCL_UINT32_C(0xffffffff))
                break;
            m_len = (m_off ^ UCL_UINT32_C(0xffffffff)) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + getbit(bb);
        else if (getbit(bb))
            m_len = 3 + getbit(bb);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        fail(olen + m_len > oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
        {
            const uint8_t *m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}
