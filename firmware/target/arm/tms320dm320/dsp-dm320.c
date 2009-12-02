/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Catalin Patulea <cat@vv.carleton.ca>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "debug.h"
#include "string.h"
#include "file.h"
#include "dsp-target.h"
#include "dsp/ipc.h"

#ifdef DEBUG
static void dsp_status(void)
{
    unsigned short hpib_ctl = IO_DSPC_HPIB_CONTROL;
    unsigned short hpib_stat = IO_DSPC_HPIB_STATUS;
    char buffer1[80], buffer2[80];
    
    DEBUGF("dsp_status(): clkc_hpib=%u clkc_dsp=%u",
           !!(IO_CLK_MOD0 & (1 << 11)), !!(IO_CLK_MOD0 & (1 << 10)));
    
    DEBUGF("dsp_status(): irq_dsphint=%u 7fff=%04x scratch_status=%04x",
           (IO_INTC_IRQ0 >> IRQ_DSPHINT) & 1, DSP_(0x7fff), DSP_(_status));
#define B(f,w,b,m) if ((w & (1 << b)) == 0) \
                       strcat(f, "!"); \
                       strcat(f, #m "|");
    strcpy(buffer1, "");
    B(buffer1, hpib_ctl, 0, EN);
    B(buffer1, hpib_ctl, 3, NMI);
    B(buffer1, hpib_ctl, 5, EXCHG);
    B(buffer1, hpib_ctl, 7, DINT0);
    B(buffer1, hpib_ctl, 8, DRST);
    B(buffer1, hpib_ctl, 9, DHOLD);
    B(buffer1, hpib_ctl, 10, BIO);
    
    strcpy(buffer2, "");
    B(buffer2, hpib_stat, 8, HOLDA);
    B(buffer2, hpib_stat, 12, DXF);
    
    DEBUGF("dsp_status(): hpib: ctl=%s stat=%s", buffer1, buffer2);
#undef B
}
#endif

void dsp_reset(void)
{
    DSP_(0x7fff) = 0xdead;
    
    IO_DSPC_HPIB_CONTROL &= ~(1 << 8);
    /* HPIB bus cycles will lock up the ARM in here. Don't touch DSP RAM. */
    nop; nop;
    IO_DSPC_HPIB_CONTROL |= 1 << 8;
    
    /* TODO: Timeout. */
    while (DSP_(0x7fff) != 0);
}

void dsp_wake(void)
{
    /* If this is called concurrently, we may overlap setting and resetting the
       bit, which causes lost interrupts to the DSP. */
    int old_level = disable_irq_save();
    
    /* The first time you INT0 the DSP, the ROM loader will branch to your RST
       handler. Subsequent times, your INT0 handler will get executed. */
    IO_DSPC_HPIB_CONTROL &= ~(1 << 7);
    nop; nop;
    IO_DSPC_HPIB_CONTROL |= 1 << 7;
    
    restore_irq(old_level);
}

void dsp_load(const struct dsp_section *im)
{
    while (im->raw_data_size_half) {
        volatile unsigned short *data_ptr = &DSP_(im->physical_addr);
        unsigned int i;
        
        /* Use 16-bit writes. */
        if (im->raw_data) {
            DEBUGF("dsp_load(): loading %u words at 0x%04x (0x%08lx)",
                   im->raw_data_size_half, im->physical_addr,
                   (unsigned long)data_ptr);
            
            for (i = 0; i < im->raw_data_size_half; i++) {
                data_ptr[i] = im->raw_data[i];
            }
        } else {
            DEBUGF("dsp_load(): clearing %u words at 0x%04x (0x%08lx)",
                   im->raw_data_size_half, im->physical_addr,
                   (unsigned long)data_ptr);
            
            for (i = 0; i < im->raw_data_size_half; i++) {
                data_ptr[i] = 0;
            }
        }
        
        im++;
    }
}

