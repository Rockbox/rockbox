/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gigabeat specific code for the Wolfson codec
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
#include "i2c.h"
#include "i2c-meg-fx.h"
/*
 * Reset the I2S BIT.FORMAT I2S, 16bit, FIFO.FORMAT 32bit
 */
void i2s_reset(void)
{
}

/*
 * Initialise the WM8975 for playback via headphone and line out.
 * Note, I'm using the WM8750 datasheet as its apparently close.
 */
int audiohw_init(void) {
    /* reset I2C */
    i2c_init();
    
    /* GPC5 controls headphone output */
    GPCCON &= ~(0x3 << 10);
    GPCCON |= (1 << 10);
    GPCDAT |= (1 << 5);

    return 0;
}

void audiohw_postinit(void)
{
}

void wmcodec_write(int reg, int data)
{
    i2c_send(0x34, (reg<<1) | ((data&0x100)>>8), data&0xff);
}
