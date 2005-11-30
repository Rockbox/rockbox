/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jens Arnold
 *
 * Self-extracting firmware loader to work around the 200KB size limit
 * for archos player and recorder v1
 * Decompresses a built-in UCL-compressed image (method 2e) and executes it.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "uclimage.h"

#define ICODE_ATTR __attribute__ ((section (".icode")))

/* Symbols defined in the linker script */
extern char iramcopy[], iramstart[], iramend[];
extern char stackend[];
extern char loadaddress[], dramend[];

/* Prototypes */
extern void start(void);

void main(void) ICODE_ATTR;
int ucl_nrv2e_decompress_8(const unsigned char *src, unsigned char *dst,
                           unsigned long *dst_len) ICODE_ATTR;

/* Vector table */
void (*vbr[]) (void) __attribute__ ((section (".vectors"))) =
{
    start, (void *)stackend,
    start, (void *)stackend,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/** All subsequent functions are executed from IRAM **/

/* Thinned out version of the UCL 2e decompression sourcecode
 * Original (C) Markus F.X.J Oberhumer under GNU GPL license */
#define GETBIT(bb, src, ilen) \
    (((bb = bb & 0x7f ? bb*2 : ((unsigned)src[ilen++]*2+1)) >> 8) & 1)

int ucl_nrv2e_decompress_8(const unsigned char *src, unsigned char *dst,
                           unsigned long *dst_len)
{
    unsigned long bb = 0;
    unsigned ilen = 0, olen = 0, last_m_off = 1;

    for (;;)
    {
        unsigned m_off, m_len;

        while (GETBIT(bb,src,ilen))
            dst[olen++] = src[ilen++];

        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + GETBIT(bb,src,ilen);
            if (GETBIT(bb,src,ilen))
                break;
            m_off = (m_off-1)*2 + GETBIT(bb,src,ilen);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = GETBIT(bb,src,ilen);
        }
        else
        {
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == 0xffffffff)
                break;
            m_len = (m_off ^ 0xffffffff) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        if (m_len)
            m_len = 1 + GETBIT(bb,src,ilen);
        else if (GETBIT(bb,src,ilen))
            m_len = 3 + GETBIT(bb,src,ilen);
        else
        {
            m_len++;
            do {
                m_len = m_len*2 + GETBIT(bb,src,ilen);
            } while (!GETBIT(bb,src,ilen));
            m_len += 3;
        }
        m_len += (m_off > 0x500);
        {
            const unsigned char *m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
    }
    *dst_len = olen;

    return ilen;
}

#define ALIGNED_IMG_SIZE ((sizeof(image) + 3) & ~3)
/* This will never return */
void main(void)
{
    unsigned long dst_len; /* dummy */
    unsigned long *src = (unsigned long *)image;
    unsigned long *dst = (unsigned long *)(dramend - ALIGNED_IMG_SIZE);
    
    do
        *dst++ = *src++;
    while (dst < (unsigned long *)dramend);
    
    ucl_nrv2e_decompress_8(dramend - ALIGNED_IMG_SIZE, loadaddress, &dst_len);

    asm(
        "mov.l   @%0+,r0     \n"
        "jmp     @r0         \n"
        "mov.l   @%0+,r15    \n"
        : : "r"(loadaddress) : "r0"
    );
}
