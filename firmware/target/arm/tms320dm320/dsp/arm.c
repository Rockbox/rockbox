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
 * Copyright (C) 2009 by Karl Kurbjun
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

#include <stdio.h>
#include "arm.h"
#include "registers.h"
#include "ipc.h"
#include "dma.h"

volatile struct ipc_message status;

#if defined(HAVE_DEBUG)
static int acked;
#endif

interrupt void handle_int0(void) {
    IFR = 1;
    
#if defined(HAVE_DEBUG)
    acked = 1;
#endif

    waiting = 0;
    
    if(dma0_stopped==0)
   	{
   		if(!(DMPREC&0x01))
   		{
   		    /* Give the HPIB access to refill first */
   		    rebuffer();
   		    
   		    /* Start the MCBSP DMA */
   			DMPREC |= 1;
   			audiohw_start();
		}
		else
		{
			rebuffer();
		}
	}
	else
	{
	    rebuffer();
    }
}

#if defined(HAVE_DEBUG)
static void startack(void)
{
    acked = 0;
    int_arm();
}

static void waitack(void)
{
    /* Wait until ARM has picked up data. */
    while (!acked) 
    {
        /* IDLE alone never seems to wake up :( */
        asm("        IDLE 1");
        asm("        NOP");
    }
}

void debugf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    status.msg = MSG_DEBUGF;
    vsnprintf((char *)status.payload.debugf.buffer, sizeof(status), fmt, args);
    va_end(args);

	startack();
	waitack();

    acked = 2;
}
#endif
