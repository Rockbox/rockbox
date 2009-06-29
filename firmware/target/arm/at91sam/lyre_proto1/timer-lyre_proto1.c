/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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
#include "timer.h"
#include "logf.h"
#include "at91sam9260.h"

/*-----------------------------------------------------------------------------
 * Function Name       : pitc_handler
 * Object              : Handler for PITC interrupt
 *---------------------------------------------------------------------------*/
void pitc_handler(void)
{
    unsigned long pivr = 0;
    unsigned long pisr = 0;

    /* Read the PISR */
    pisr = AT91C_PITC_PISR & AT91C_PITC_PITS;

    if (pisr != 0)
    {
        /* Read the PIVR. It acknowledges the IT */
        pivr = AT91C_PITC_PIVR;

        if (pfn_timer != NULL)
            pfn_timer();
    }
}

bool timer_set(long cycles, bool start)
{
    if (cycles < 1000) /* Max value on PITC?? */
    {
        if (start && pfn_unregister != NULL)
        {
            pfn_unregister();
            pfn_unregister = NULL;
        }

        /* Configure a resolution of <cycles> ms */
        AT91C_PITC_PIMR = MCK_FREQ / ( 16 * cycles) - 1;

        return true;
    }

    return false;
}

bool timer_start(void)
{
    bool retval = true;
    volatile unsigned long pimr = 0;

    /* Configure a resolution of 1 ms */
    AT91C_PITC_PIMR = MCK_FREQ / (16 * 1000) - 1;

    /* Enable interrupts */
    /* Disable the interrupt on the interrupt controller */
    AT91C_AIC_IDCR = (1 << AT91C_ID_SYS);

    /* Save the interrupt handler routine pointer and the interrupt priority */
    AT91C_AIC_SVR(AT91C_ID_SYS) = (unsigned long) pitc_handler;
    /* Store the Source Mode Register */
    AT91C_AIC_SMR(AT91C_ID_SYS) = AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE | \
    AT91C_AIC_PRIOR_LOWEST;
    /* Clear the interrupt on the interrupt controller */
    AT91C_AIC_ICCR = (1 << AT91C_ID_SYS);

    /* Enable the interrupt on the interrupt controller */
    AT91C_AIC_IECR = (1 << AT91C_ID_SYS);

    /* Enable the interrupt on the pit */
    pimr = AT91C_PITC_PIMR;
    AT91C_PITC_PIMR = pimr | AT91C_PITC_PITIEN;

    /* Enable the pit */
    pimr = AT91C_PITC_PIMR;
    AT91C_PITC_PIMR = pimr | AT91C_PITC_PITEN;

    return retval;
}

void timer_stop(void)
{
    volatile unsigned long pimr = 0;

    /* Disable the interrupt on the interrupt controller */
    AT91C_AIC_IDCR = (1 << AT91C_ID_SYS);

    /* Clear the interrupt on the interrupt controller */
    AT91C_AIC_ICCR = (1 << AT91C_ID_SYS);

    /* Disable the interrupt on the pit */
    pimr = AT91C_PITC_PIMR;
    pimr &= ~AT91C_PITC_PITIEN;
    AT91C_PITC_PIMR = pimr;

    /* Disable the pit */
    pimr = AT91C_PITC_PIMR;
    pimr &= ~AT91C_PITC_PITEN;
    AT91C_PITC_PIMR = pimr;
}
