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
 * Copyright (C) 2008 by Maurus Cuelenaere
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "registers.h"
#include "arm.h"
#include "dma.h"

/* This is placed at the right (aligned) address using linker.cmd. */
#pragma DATA_SECTION (data, ".dma")
signed short data[PCM_SIZE / 2];

/* Filled in by loader. */
unsigned short sdem_addrh;
unsigned short sdem_addrl;

interrupt void handle_dma0(void) {
    unsigned long sdem_addr;
    /* Byte offset to half-buffer locked by DMA0.
            0 for top, PCM_SIZE/2(0x4000) for bottom */
    unsigned short dma0_locked;
    unsigned short dma0_unlocked;
    
    IFR = 1 << 6;

    /* DMSRC0 is the beginning of the DMA0-locked SARAM half-buffer. */
    DMSA = 0x00 /* DMSRC0 */;
    dma0_locked = (DMSDN << 1) & (PCM_SIZE / 2);
    dma0_unlocked = dma0_locked ^ (PCM_SIZE / 2);
    
    /* ARM, decode into same half, in SDRAM. */
    status.msg = MSG_REFILL;
    status.payload.refill.topbottom = dma0_locked;
    int_arm();

    /* DMAC, copy opposite halves from SDRAM to SARAM. */
    sdem_addr = ((unsigned long)sdem_addrh << 16 | sdem_addrl) + dma0_unlocked;
    SDEM_ADDRL = sdem_addr & 0xffff;
    SDEM_ADDRH = sdem_addr >> 16;
    DSP_ADDRL = (unsigned short)data + (dma0_unlocked >> 1);
    DSP_ADDRH = 0;
    DMA_SIZE = PCM_SIZE / 2;
    DMA_CTRL = 0;

    status.payload.refill._DMA_TRG = DMA_TRG;
    status.payload.refill._SDEM_ADDRH = SDEM_ADDRH;
    status.payload.refill._SDEM_ADDRL = SDEM_ADDRL;
    status.payload.refill._DSP_ADDRH = DSP_ADDRH;
    status.payload.refill._DSP_ADDRL = DSP_ADDRL;

    DMA_TRG = 1;
}

interrupt void handle_dmac(void) {
    IFR = 1 << 11;
}

void dma_init(void) {
    /* Configure DMA */
    DMSFC0 = 2 << 12 | 1 << 11; /* Event XEVT0, 32-bit transfers, 0 frame count */
    DMMCR0 = 1 << 14 | 1 << 13 | /* Interrupts generated, Half and full buffer */
             1 << 12 | 1 << 8 | 1 << 6 | 1; /* ABU mode,
                                               From data space with postincrement,
                                               To data space with no mod */
    DMSRC0 = (unsigned short)&data;
    DMDST0 = (unsigned short)&DXR20; /* First of two-word register pair */
    DMCTR0 = sizeof(data);

    /* Run, Rudolf, run! (with DMA0 interrupts) */
    DMPREC = 2 << 6 | 1;
}
