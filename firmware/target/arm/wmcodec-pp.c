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
#include "lcd.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "power.h"
#include "debug.h"
#include "system.h"
#include "sprintf.h"
#include "button.h"
#include "string.h"
#include "file.h"
#include "buffer.h"
#include "audio.h"

#if CONFIG_CPU == PP5020
#include "i2c-pp5020.h"
#elif CONFIG_CPU == PP5002
#include "i2c-pp5002.h"
#endif

/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
void i2s_reset(void)
{
#if CONFIG_CPU == PP5020
    /* I2S soft reset */
    outl(inl(0x70002800) | 0x80000000, 0x70002800);
    outl(inl(0x70002800) & ~0x80000000, 0x70002800);

    /* BIT.FORMAT [11:10] = I2S (default) */
    outl(inl(0x70002800) & ~0xc00, 0x70002800);
    /* BIT.SIZE [9:8] = 16bit (default) */
    outl(inl(0x70002800) & ~0x300, 0x70002800);

    /* FIFO.FORMAT [6:4] = 32 bit LSB */
    /* since BIT.SIZ < FIFO.FORMAT low 16 bits will be 0 */
    outl(inl(0x70002800) | 0x30, 0x70002800);

    /* RX_ATN_LVL=1 == when 12 slots full */
    /* TX_ATN_LVL=1 == when 12 slots empty */
    outl(inl(0x7000280c) | 0x33, 0x7000280c);

    /* Rx.CLR = 1, TX.CLR = 1 */
    outl(inl(0x7000280c) | 0x1100, 0x7000280c);
#elif CONFIG_CPU == PP5002
    /* I2S device reset */
    outl(inl(0xcf005030) | 0x80, 0xcf005030);
    outl(inl(0xcf005030) & ~0x80, 0xcf005030);

    /* I2S controller enable */
    outl(inl(0xc0002500) | 0x1, 0xc0002500);

    /* BIT.FORMAT [11:10] = I2S (default) */
    /* BIT.SIZE [9:8] = 24bit */
    /* FIFO.FORMAT = 24 bit LSB */

    /* reset DAC and ADC fifo */
    outl(inl(0xc000251c) | 0x30000, 0xc000251c);
#endif
}

/*
 * Initialise the WM8975 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int wmcodec_init(void) {
    /* reset I2C */
    i2c_init();

#if CONFIG_CPU == PP5020
    /* normal outputs for CDI and I2S pin groups */
    outl(inl(0x70000020) & ~0x300, 0x70000020);

    /*mini2?*/
    outl(inl(0x70000010) & ~0x3000000, 0x70000010);
    /*mini2?*/

    /* device reset */
    outl(inl(0x60006004) | 0x800, 0x60006004);
    outl(inl(0x60006004) & ~0x800, 0x60006004);

    /* device enable */
    outl(inl(0x6000600C) | 0x807, 0x6000600C);

    /* enable external dev clock clocks */
    outl(inl(0x6000600c) | 0x2, 0x6000600c);

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
#endif
    
    return 0;
}

void wmcodec_write(int reg, int data)
{
/* Todo: Since the ipod_i2c_* functions also work on H10 and possibly other PP
   targets, these functions should probably be renamed */
#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
    /* The H10's audio codec uses an I2C address of 0x1b */
    ipod_i2c_send(0x1b, (reg<<1) | ((data&0x100)>>8),data&0xff);
#else
    /* The iPod's audio codecs use an I2C address of 0x1a */ 
    ipod_i2c_send(0x1a, (reg<<1) | ((data&0x100)>>8),data&0xff);
#endif
}
