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

/* A "DSP image" is an array of these, terminated by raw_data_size_half = 0. */
struct dsp_section {
    const unsigned short *raw_data;
    unsigned short physical_addr;
    unsigned short raw_data_size_half;
};

/* Must define struct dsp_section before including the image. */
#include "dsp/dsp-image.h"

#define dsp_message (*(volatile struct ipc_message *)&DSP_(_status))

#ifdef DEBUG
static void dsp_status(void)
{
    unsigned short hpib_ctl = IO_DSPC_HPIB_CONTROL;
    unsigned short hpib_stat = IO_DSPC_HPIB_STATUS;
    char buffer1[80], buffer2[80];
    
    DEBUGF("dsp_status(): clkc_hpib=%u clkc_dsp=%u",
           !!(IO_CLK_MOD0 & (1 << 11)), !!(IO_CLK_MOD0 & (1 << 10)));
    
    DEBUGF("dsp_status(): irq_dsphint=%u 7fff=%04x scratch_status=%04x"
           " acked=%04x",
           (IO_INTC_IRQ0 >> IRQ_DSPHINT) & 1, DSP_(0x7fff), DSP_(_status),
           DSP_(_acked));
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

static void dsp_reset(void)
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

static void dsp_load(const struct dsp_section *im)
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

static signed short *the_rover = (signed short *)0x1900000;
static unsigned int index_rover = 0;
static signed short __attribute__((aligned (16))) pcm_buffer[PCM_SIZE / 2];

void dsp_init(void)
{
    unsigned long sdem_addr;
    int fd;
    int bytes;


    IO_INTC_IRQ0 = 1 << 11;
    IO_INTC_EINT0 |= 1 << 11;
    
    IO_DSPC_HPIB_CONTROL = 1 << 10 | 1 << 9 | 1 << 8 | 1 << 7 | 1 << 3 | 1 << 0;
    
    dsp_reset();
    dsp_load(dsp_image);
    
    /* Initialize codec. */
    sdem_addr = (unsigned long)pcm_buffer - CONFIG_SDRAM_START;
    DEBUGF("pcm_sdram at 0x%08lx, sdem_addr 0x%08lx",
		(unsigned long)pcm_buffer, (unsigned long)sdem_addr);
    DSP_(_sdem_addrl) = sdem_addr & 0xffff;
    DSP_(_sdem_addrh) = sdem_addr >> 16;
	
	fd = open("/test.raw", O_RDONLY);
	bytes = read(fd, the_rover, 4*1024*1024);
	close(fd);
	
	DEBUGF("read %d rover bytes", bytes);
	
	#if 0
	{
    unsigned int i;
    memset(pcm_buffer, 0x80, PCM_SIZE);
    for (i = 0; i < 8192; i++) {
		signed short val = 0;
		/*if (i < PCM_SIZE/4/2) {
			val = 128*(i%256);
		} else {
			val = 2*128*128-128*(i%256);
		}*/
		val = 128*(i%256);
		pcm_buffer[2*i] = pcm_buffer[2*i+1] = val;
    }
    clean_dcache();


    {
    unsigned int i;
    volatile signed short *pdata = &DSP_(_data);
    DEBUGF("dsp__data at %08x", pdata);
    for (i = 0; i < 4096; i++) {
		pdata[2*i]=pdata[2*i+1]=(i&1)*32767;
    }
    for (i = 4096; i < 8192; i++) {
		pdata[2*i]=pdata[2*i+1]=0;
    }
    }
    #endif

#ifdef IPC_SIZES
	DEBUGF("dsp_message at 0x%08x", &dsp_message);
	DEBUGF("sizeof(ipc_message)=%uB offset(ipc_message.payload)=%uB",
		sizeof(struct ipc_message), (int)&((struct ipc_message*)0)->payload);
#endif

#ifdef INIT_MSG
    /* Prepare init message. */
    
    /* DSP accesses MUST be done a word at a time. */
    dsp_message.msg = MSG_INIT;
    
    sdem_addr = (unsigned long)pcm_sdram - CONFIG_SDRAM_START;
    DEBUGF("pcm_sdram at 0x%08x, sdem_addr 0x%08x", pcm_sdram, sdem_addr);
    dsp_message.payload.init.sdem_addrl = sdem_addr & 0xffff;
    dsp_message.payload.init.sdem_addrh = sdem_addr >> 16;
    
    DEBUGF("dsp_message: %04x %04x %04x %04x",
		((unsigned short *)&dsp_message)[0],
		((unsigned short *)&dsp_message)[1],
		((unsigned short *)&dsp_message)[2],
		((unsigned short *)&dsp_message)[3]);
#endif

}

void DSPHINT(void)
{
    unsigned int i;
    char buffer[80];
    static unsigned short level = 2;
     
    unsigned short *pcm_topbottom, *pcm_topbottom_end;

    
    IO_INTC_IRQ0 = 1 << 11;
    

    switch (dsp_message.msg) 
    {
    case MSG_DEBUGF:
        /* DSP stores one character per word. */
        for (i = 0; i < sizeof(buffer); i++) 
        {
            buffer[i] = dsp_message.payload.debugf.buffer[i];
        }
        
        /* Release shared area to DSP. */
        dsp_wake();
        
        DEBUGF("DSP: %s", buffer);
        break;
    case MSG_REFILL:
        DEBUGF("DSP: DMA0 with topbottom=%u (fill at %04lx)",
            dsp_message.payload.refill.topbottom,
            (unsigned long)pcm_buffer +
            dsp_message.payload.refill.topbottom);
        pcm_topbottom = pcm_buffer + dsp_message.payload.refill.topbottom / 2;
		
		/*
		i = 0;
		while (i < PCM_SIZE/4) {
			unsigned int j;
			for (j = 0; j < level; j++) {
				pcm_topbottom[i+j] = -32768;
			}
			for (j = level; j < 2*level; j++) {
				pcm_topbottom[i+j] = 32767;
			}
			i += 2*level;
		}
		
		level += 2;
		if (level > 256) {
			level = 2;
		}*/
		
		memcpy(pcm_topbottom, the_rover + index_rover, PCM_SIZE/2);
		index_rover += PCM_SIZE/4;
		if (index_rover >= 2*1024*1024) {
			index_rover = 0;
		}
		
		/*
		pcm_topbottom = &p_pcm_sdram[dsp_message.payload.refill.topbottom/2];
		DEBUGF("DSP: tofill begins: %04x %04x %04x %04x",
			pcm_topbottom[0],
			pcm_topbottom[1],
			pcm_topbottom[2],
			pcm_topbottom[3]
		);
		pcm_topbottom_end = &p_pcm_sdram[(dsp_message.payload.refill.topbottom+PCM_SIZE/2)/2];
		DEBUGF("DSP: tofill ends: %04x %04x %04x %04x",
			pcm_topbottom_end[-4],
			pcm_topbottom_end[-3],
			pcm_topbottom_end[-2],
			pcm_topbottom_end[-1]
		);
		*/
		
        /*
        DEBUGF("DSP: DMA0: SD %04x:%04x -> DSP %04x:%04x, TRG %04x",
			dsp_message.payload.refill._SDEM_ADDRH,
			dsp_message.payload.refill._SDEM_ADDRL,
			dsp_message.payload.refill._DSP_ADDRH,
			dsp_message.payload.refill._DSP_ADDRL,
			dsp_message.payload.refill._DMA_TRG);
        */
        break;
    default:
        DEBUGF("DSP: unknown msg 0x%04x", dsp_message.msg);
        break;
    }
    
    /* Release shared area to DSP. */
    dsp_wake();
    
    DEBUGF("DSP: %s", buffer);
}
