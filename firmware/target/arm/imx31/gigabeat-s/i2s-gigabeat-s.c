/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#include "system.h"
#include "ccm-imx31.h"
#include "sdma-imx31.h"
#include "i2s.h"

void i2s_reset(void)
{
    /* How SYSCLK for codec is derived (USBPLL=338.688MHz).
     *
     * SSI post dividers (SSI2 PODF=4, SSI2 PRE PODF=0):
     * 338688000Hz / 5 = 67737600Hz = ssi1_clk
     * 
     * SSI bit clock dividers (DIV2=1, PSR=0, PM=0):
     * ssi1_clk / 4 = 16934400Hz = INT_BIT_CLK (MCLK)
     *
     * WM Codec post divider (MCLKDIV=1.5):
     * INT_BIT_CLK (MCLK) / 1.5 = 11289600Hz = 256*fs = SYSCLK
     */
    bitmod32(&CCM_PDR1,
             ((1-1) << CCM_PDR1_SSI1_PRE_PODF_POS) |
             ((5-1) << CCM_PDR1_SSI1_PODF_POS) |
             ((8-1) << CCM_PDR1_SSI2_PRE_PODF_POS) |
             ((64-1) << CCM_PDR1_SSI2_PODF_POS),
             CCM_PDR1_SSI1_PODF | CCM_PDR1_SSI2_PODF |
             CCM_PDR1_SSI1_PRE_PODF | CCM_PDR1_SSI2_PRE_PODF);

    ccm_module_clock_gating(CG_SSI1, CGM_ON_RUN_WAIT);
    ccm_module_clock_gating(CG_SSI2, CGM_ON_RUN_WAIT);

    /* Reset & disable SSIs */
    SSI_SCR1 &= ~SSI_SCR_SSIEN;
    SSI_SCR2 &= ~SSI_SCR_SSIEN;

    SSI_SIER1 = 0;
    SSI_SIER2 = 0;

    /* Set up audio mux */

    /* Port 2 (internally connected to SSI2)
     * All clocking is output sourced from port 4 */
    AUDMUX_PTCR2 = AUDMUX_PTCR_TFS_DIR | AUDMUX_PTCR_TFSEL_PORT4 |
                   AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT4 |
                   AUDMUX_PTCR_SYN;
 
    /* Receive data from port 4 */
    AUDMUX_PDCR2 = AUDMUX_PDCR_RXDSEL_PORT4;
    /* All clock lines are inputs sourced from the master mode codec and
     * sent back to SSI2 through port 2 */
    AUDMUX_PTCR4 = AUDMUX_PTCR_SYN;

    /* Receive data from port 2 */
    AUDMUX_PDCR4 = AUDMUX_PDCR_RXDSEL_PORT2;

    /* PORT1 (internally connected to SSI1) routes clocking to PORT5 to
     * provide MCLK to the codec */
    /* TX clocks are inputs taken from SSI2 */
    /* RX clocks are outputs taken from PORT4 */
    AUDMUX_PTCR1 = AUDMUX_PTCR_RFS_DIR | AUDMUX_PTCR_RFSSEL_PORT4 |
                   AUDMUX_PTCR_RCLKDIR | AUDMUX_PTCR_RCSEL_PORT4;
    /* RX data taken from PORT4 */
    AUDMUX_PDCR1 = AUDMUX_PDCR_RXDSEL_PORT4;

    /* PORT5 outputs TCLK sourced from PORT1 (SSI1) */
    AUDMUX_PTCR5 = AUDMUX_PTCR_TCLKDIR | AUDMUX_PTCR_TCSEL_PORT1;
    AUDMUX_PDCR5 = 0;

    /* Setup SSIs */

    /* SSI2 - SoC software interface for all I2S data out */
    SSI_SCR2 = SSI_SCR_SYN | SSI_SCR_I2S_MODE_SLAVE;
    SSI_STCR2 = SSI_STCR_TXBIT0 | SSI_STCR_TSCKP | SSI_STCR_TFSI |
                SSI_STCR_TEFS | SSI_STCR_TFEN0;

    /* 16 bits per word, 2 words per frame */
    SSI_STCCR2 = SSI_STRCCR_WL16 | ((2-1) << SSI_STRCCR_DC_POS) |
                 ((4-1) << SSI_STRCCR_PM_POS);

    /* Transmit low watermark */
    SSI_SFCSR2 = (SSI_SFCSR2 & ~SSI_SFCSR_TFWM0) |
                 ((8-SDMA_SSI_TXFIFO_WML) << SSI_SFCSR_TFWM0_POS);
    SSI_STMSK2 = 0;

    /* SSI1 - provides MCLK to codec. Receives data from codec. */
    SSI_STCR1 = SSI_STCR_TXDIR;

    /* f(INT_BIT_CLK) =
     * f(SYS_CLK) / [(DIV2 + 1)*(7*PSR + 1)*(PM + 1)*2] =
     * 677737600 / [(1 + 1)*(7*0 + 1)*(0 + 1)*2] =
     * 677737600 / 4 = 169344000 Hz
     *
     * 45.4.2.2 DIV2, PSR, and PM Bit Description states:
     * Bits DIV2, PSR, and PM should not be all set to zero at the same
     * time.
     *
     * The hardware seems to force a divide by 4 even if all bits are
     * zero but comply by setting DIV2 and the others to zero.
     */
    SSI_STCCR1 = SSI_STRCCR_DIV2 | ((1-1) << SSI_STRCCR_PM_POS);

    /* SSI1 - receive - asynchronous clocks */
    SSI_SCR1 = SSI_SCR_I2S_MODE_SLAVE;

    SSI_SRCR1 = SSI_SRCR_RXBIT0 | SSI_SRCR_RSCKP | SSI_SRCR_RFSI |
                SSI_SRCR_REFS;

    /* 16 bits per word, 2 words per frame */
    SSI_SRCCR1 = SSI_STRCCR_WL16 | ((2-1) << SSI_STRCCR_DC_POS) |
                 ((4-1) << SSI_STRCCR_PM_POS);

    /* Receive high watermark */
    SSI_SFCSR1 = (SSI_SFCSR1 & ~SSI_SFCSR_RFWM0) |
                 (SDMA_SSI_RXFIFO_WML << SSI_SFCSR_RFWM0_POS);
    SSI_SRMSK1 = 0;

    /* Enable SSI1 (codec clock) */
    SSI_SCR1 |= SSI_SCR_SSIEN;
}
