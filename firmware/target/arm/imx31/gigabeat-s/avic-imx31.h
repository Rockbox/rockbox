/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by James Espinoza
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef AVIC_IMX31_H
#define AVIC_IMX31_H

enum INT_TYPE
{
    IRQ = 0,
    FIQ
};

enum IMX31_INT_LIST
{
    __IMX31_INT_FIRST = -1,
   RESERVED0, RESERVED1,     RESERVED2, I2C3,
   I2C2,      MPEG4_ENCODER, RTIC,      FIR,
   MMC_SDHC2, MMC_SDHC1,     I2C1,      SSI2,
   SSI1,      CSPI2,         CSPI1,     ATA,
   MBX,       CSPI3,         UART3,     IIM,
   SIM1,      SIM2,          RNGA,      EVTMON,
   KPP,       RTC,           PWN,       EPIT2,
   EPIT1,     GPT,           PWR_FAIL,  CCM_DVFS,
   UART2,     NANDFC,        SDMA,      USB_HOST1,
   USB_HOST2, USB_OTG,       RESERVED3, MSHC1,
   MSHC2,     IPU_ERR,       IPU,       RESERVED4,
   RESERVED5, UART1,         UART4,     UART5,
   ETC_IRQ,   SCC_SCM,       SCC_SMN,   GPIO2,
   GPIO1,     CCM_CLK,       PCMCIA,    WDOG,
   GPIO3,     RESERVED6,     EXT_PWMG,  EXT_TEMP,
   EXT_SENS1, EXT_SENS2,     EXT_WDOG,  EXT_TV,
   ALL
};

void avic_init(void);
void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
                     void (*handler)(void));
void avic_disable_int(enum IMX31_INT_LIST ints);
void avic_set_int_type(enum IMX31_INT_LIST ints, enum INT_TYPE intstype);
#endif
