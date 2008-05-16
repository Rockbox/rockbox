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

#include "registers.h"
#include "arm.h"
#include "ipc.h"
#include "dma.h"
#include "audio.h"
#include <math.h>

void main(void) {
    register int i;
    register signed short *p;

    TCR = 1 << 4; /* Stop the timer. */
    IMR = 0xffff; /* Unmask all interrupts. */
    IFR = IFR; /* Clear all pending interrupts. */
    asm("   rsbx INTM"); /* Globally enable interrupts. */
    
    audiohw_init();
    
    dma_init();

    audiohw_postinit();

#if 0
    for (i = 0; i < 32; i++)
    {
        double ratio = ((double)i)/32.0;
        double rad = 3.0*3.141592*ratio;
        double normal = sin(rad);
        double scaled = 32767.0*(normal);
        data[2*i + 0] = -(signed short)scaled;
        data[2*i + 1] = (signed short)scaled;
    }

    debugf("starting write");

    i = 0;
    p = data;
    SPSA0 = 0x01;
    for (;;) {
        while ((SPSD0 & (1 << 1)) == 0);
        DXR20 = *p++; // left channel
        DXR10 = *p++; // right channel
        if (++i == 32)
        {
            p = data;
            i = 0;
        }
    }
#endif
    debugf("DSP inited...");

    for (;;) {
        asm("        IDLE 1");
        asm("        NOP");
    }
}

/* Obsoleted/testing snippets: */
#ifdef REMAP_VECTORS
    /* Remap vectors to 0x3F80 (set in linker.cmd). */
    PMST = (PMST & 0x7f) | 0x3F80;
    
    /* Make sure working interrupts aren't a fluke. */
    memset((unsigned short *)0x7f80, 0, 0x80);
#endif

#ifdef DATA_32_SINE
    for (i = 0; i < 32; i++) {
        double ratio = ((double)i)/32.0;
        double rad = 3.0*3.141592*ratio;
        double normal = sin(rad);
        double scaled = 32767.0*(normal);
        data[2*i + 0] = -(signed short)scaled;
        data[2*i + 1] = (signed short)scaled;
    }
#endif

#ifdef MANUAL_TRANSFER
    register signed short *p;

    debugf("starting write");

    i = 0;
    p = data;
    SPSA0 = 0x01;
    for (;;) {
        while ((SPSD0 & (1 << 1)) == 0);
        DXR20 = *p++; // left channel
        DXR10 = *p++; // right channel
        if (++i == 32) {
            p = data;
            i = 0;
        }
    }
#endif

#ifdef INIT_MSG
    /* Copy codec init data (before debugf, which clobbers status). */
    if (status.msg != MSG_INIT) {
        debugf("No init message (%04x: %04x %04x %04x %04x instead)",
            (unsigned short)&status,
            ((unsigned short *)&status)[0],
            ((unsigned short *)&status)[1],
            ((unsigned short *)&status)[2],
            ((unsigned short *)&status)[3]);
        return;
    }
    
    memcpy(&init, (void *)&status.payload.init, sizeof(init));
#endif
    
#ifdef IPC_SIZES
    debugf("sizeof(ipc_message)=%uw offset(ipc_message.payload)=%uw",
        sizeof(struct ipc_message), (int)&((struct ipc_message*)0)->payload);
#endif

#ifdef VERBOSE_INIT
    debugf("codec started with PCM at SDRAM offset %04x:%04x",
        sdem_addrh, sdem_addrl);
#endif

#ifdef SPIKE_DATA
    for (i = 0; i < 0x2000; i++) {
        data[2*i  ] = data[2*i+1] = ((i % 32) == 0) * 32767;
    }
#endif
