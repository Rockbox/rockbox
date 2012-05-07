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

/* Include Standard files */
#include "at91sam9260.h"
#include "config.h"

/*-----------------------------------------------------------------------------
 * Function Name       : default_spurious_handler
 * Object              : default handler for spurious interrupt
 *---------------------------------------------------------------------------*/
void default_spurious_handler(void)
{
    while (1);
}

/*-----------------------------------------------------------------------------
 * Function Name       : default_fiq_handler
 * Object              : default handler for fast interrupt
 *---------------------------------------------------------------------------*/
void default_fiq_handler(void)
{
    while (1);
}

/*-----------------------------------------------------------------------------
 * Function Name       : default_irq_handler
 * Object              : default handler for irq
 *---------------------------------------------------------------------------*/
void default_irq_handler(void)
{
#if defined(BOOTLOADER)
    while (1);
#endif
}

/*-----------------------------------------------------------------------------
 * Function Name     : lowlevel_init
 * Object            : This function performs very low level HW initialization
 *                     this function can use a Stack, depending the compilation
 *                     optimization mode
 *---------------------------------------------------------------------------*/
void lowlevel_init(void)
{
    unsigned char i = 0;

    /* void default_fiq_handler(void)
     * Init PMC Step 1. Enable Main Oscillator
     * Main Oscillator startup time is board specific:
     * Main Oscillator Startup Time worst case (3MHz) corresponds to 15ms
     * (0x40 for AT91C_CKGR_OSCOUNT field)
     */
    AT91C_PMC_MOR = (((AT91C_CKGR_OSCOUNT & (0x40 << 8)) | AT91C_CKGR_MOSCEN));
    /* Wait Main Oscillator stabilization */
    while (!(AT91C_PMC_SR & AT91C_PMC_MOSCS));

    /* Init PMC Step 2.
     * Set PLLA to 198,608MHz
     * PLL Startup time depends on PLL RC filter: worst case is choosen.
     *
     * Crystal frequency = 18.432MHz; PLLA = (18.432 * 96) / 9 = 198,608MHz.
     */

    AT91C_PMC_PLLAR = (1 << 29)       |
                      (0x60 << 16)    | /* MULA = 96 */
                      (0x2 << 14)     |
                      (0x3f << 8)     |
                      (0x09); /* DIVA = 9 */

    /* Wait for PLLA stabilization */
    while (!(AT91C_PMC_SR & AT91C_PMC_LOCKA));
    /* Wait until the master clock is established for the case we already */
    /* turn on the PLLA */
    while (!(AT91C_PMC_SR & AT91C_PMC_MCKRDY));

    /* Init PMC Step 3.
     * Processor Clock = 198,608MHz (PLLA); Master clock =
     * (198,608MHz (PLLA))/2 = 98,304MHz.
     * The PMC_MCKR register must not be programmed in a single write operation
     * (see. Product Errata Sheet)
     */
    AT91C_PMC_MCKR = AT91C_PMC_PRES_CLK | AT91C_PMC_MDIV_2;
    /* Wait until the master clock is established */
    while (!(AT91C_PMC_SR & AT91C_PMC_MCKRDY));

    AT91C_PMC_MCKR |= AT91C_PMC_CSS_PLLA_CLK;
    /* Wait until the master clock is established */
    while (!(AT91C_PMC_SR & AT91C_PMC_MCKRDY));

    /* Reset AIC: assign default handler for each interrupt source
     */

    /* Disable the interrupt on the interrupt controller */
    AT91C_AIC_IDCR = (1 << AT91C_ID_SYS);

    /* Assign default handler for each IRQ source */
    AT91C_AIC_SVR(AT91C_ID_FIQ) = (int) default_fiq_handler;
    for (i = 1; i < 31; i++)
    {
        AT91C_AIC_SVR(i) = (int) default_irq_handler;
    }
    AT91C_AIC_SPU = (unsigned int) default_spurious_handler;

    /* Perform 8 IT acknoledge (write any value in EOICR) */

/* The End of Interrupt Command Register (AIC_EOICR) must be written in order
to indicate to the AIC that the current interrupt is finished. This causes the
current level to be popped from the stack, restoring the previous current level
if one exists on the stack. If another interrupt is pending, with lower or
equal priority than the old current level but with higher priority than the new
current level, the nIRQ line is re-asserted, but the interrupt sequence does
not immediately start because the “I” bit is set in the core.
SPSR_irq is restored. Finally, the saved value of the link register is restored
directly into the PC. This has the effect of returning from the interrupt to
whatever was being executed before, and of loading the CPSR with the stored
SPSR, masking or unmasking the interrupts depending on the state saved in
SPSR_irq. */
    for (i = 0; i < 8 ; i++)
    {
        AT91C_AIC_EOICR = 0;
    }

    /* Enable the interrupt on the interrupt controller */
    AT91C_AIC_IECR = (1 << AT91C_ID_SYS);

    /* Disable Watchdog */
    AT91C_WDTC_WDMR = AT91C_WDTC_WDDIS;

    /* Remap */
    AT91C_MATRIX_MRCR = AT91C_MATRIX_RCA926I | AT91C_MATRIX_RCA926D;
}
