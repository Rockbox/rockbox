/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Catalin Patulea
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/ 

#include "audio.h"
#include "registers.h"

void audiohw_init(void)
{
    /* Configure McBSP */
    SPCR10 = 0; /* Receiver reset */
    SPCR20 = 3 << 4; /* Rate gen disabled, RINT=XSYNCERR, TX disabled for now */
    PCR0 = 1 << 1; /* Serial port pins, external frame sync, external clock,
                                           frame sync FSX is active-high,
                                           TX data sampled on falling clock */
    XCR10 = 0x00a0; /* 1 word per frame, 32 bits per word */
    XCR20 = 0; /* Single-phase, unexpected frame pulse restarts xfer,
                                        0-bit data delay */
}

void audiohw_postinit(void)
{
    /* Trigger first XEVT0 */
    SPCR20 |= 1;
}
