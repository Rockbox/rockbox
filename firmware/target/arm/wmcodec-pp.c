/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Portalplayer specific code for Wolfson audio codecs
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "system.h"
#include "audiohw.h"
#include "i2c-pp.h"

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB) || defined(MROBE_100)
/* The H10's audio codec uses an I2C address of 0x1b */
#define I2C_AUDIO_ADDRESS 0x1b
#else
/* The iPod's audio codecs use an I2C address of 0x1a */ 
#define I2C_AUDIO_ADDRESS 0x1a
#endif


/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_init(void) {
    /* reset I2C */
    i2c_init();

#ifdef CPU_PP502x
    /* normal outputs for CDI and I2S pin groups */
    DEV_INIT2 &= ~0x300;

    /*mini2?*/
    DEV_INIT1 &=~0x3000000;
    /*mini2?*/

    /* I2S device reset */
    DEV_RS |= DEV_I2S;
    DEV_RS &=~DEV_I2S;

    /* I2S device enable */
    DEV_EN |= DEV_I2S;

    /* enable external dev clock clocks */
    DEV_EN |= DEV_EXTCLOCKS;

    /* external dev clock to 24MHz */
    outl(inl(0x70000018) & ~0xc, 0x70000018);
#else
    /* device reset */
    outl(inl(0xcf005030) | 0x80, 0xcf005030);
    outl(inl(0xcf005030) & ~0x80, 0xcf005030);

    /* device enable */
    outl(inl(0xcf005000) | 0x80, 0xcf005000);

    /* GPIO D06 enable for output */
    outl(inl(0xcf00000c) | 0x40, 0xcf00000c);
    outl(inl(0xcf00001c) & ~0x40, 0xcf00001c);
#ifdef IPOD_1G2G
    /* bits 11,10 == 10 */
    outl(inl(0xcf004040) & ~0x400, 0xcf004040);
    outl(inl(0xcf004040) | 0x800, 0xcf004040);
#else /* IPOD_3G */
    /* bits 11,10 == 01 */
    outl(inl(0xcf004040) | 0x400, 0xcf004040);
    outl(inl(0xcf004040) & ~0x800, 0xcf004040);

    outl(inl(0xcf004048) & ~0x1, 0xcf004048);

    outl(inl(0xcf000004) & ~0xf, 0xcf000004);
    outl(inl(0xcf004044) & ~0xf, 0xcf004044);

    /* C03 = 0 */
    outl(inl(0xcf000008) | 0x8, 0xcf000008);
    outl(inl(0xcf000018) | 0x8, 0xcf000018);
    outl(inl(0xcf000028) & ~0x8, 0xcf000028);
#endif /* IPOD_1G2G/3G */
#endif
    
#if defined(HAVE_WM8731) || defined(HAVE_WM8751)
    audiohw_preinit();
#endif
   
}

#if !defined(HAVE_WM8731) && !defined(HAVE_WM8751)
void audiohw_postinit(void)
{
}
#endif

void wmcodec_write(int reg, int data)
{
    pp_i2c_send(I2C_AUDIO_ADDRESS, (reg<<1) | ((data&0x100)>>8),data&0xff);
}
