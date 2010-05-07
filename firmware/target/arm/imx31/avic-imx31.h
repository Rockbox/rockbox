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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef AVIC_IMX31_H
#define AVIC_IMX31_H

/* #define IRQ priorities for different modules (0-15) */
#define INT_PRIO_DEFAULT    7
#define INT_PRIO_DVFS       (INT_PRIO_DEFAULT+1)
#define INT_PRIO_DPTC       (INT_PRIO_DEFAULT+1)
#define INT_PRIO_SDMA       (INT_PRIO_DEFAULT+2)

enum INT_TYPE
{
    INT_TYPE_IRQ = 0,
    INT_TYPE_FIQ
};

enum IMX31_INT_LIST
{
    __IMX31_INT_FIRST = -1,
   INT_RESERVED0, INT_RESERVED1,     INT_RESERVED2, INT_I2C3,
   INT_I2C2,      INT_MPEG4_ENCODER, INT_RTIC,      INT_FIR,
   INT_MMC_SDHC2, INT_MMC_SDHC1,     INT_I2C1,      INT_SSI2,
   INT_SSI1,      INT_CSPI2,         INT_CSPI1,     INT_ATA,
   INT_MBX,       INT_CSPI3,         INT_UART3,     INT_IIM,
   INT_SIM1,      INT_SIM2,          INT_RNGA,      INT_EVTMON,
   INT_KPP,       INT_RTC,           INT_PWN,       INT_EPIT2,
   INT_EPIT1,     INT_GPT,           INT_PWR_FAIL,  INT_CCM_DVFS,
   INT_UART2,     INT_NANDFC,        INT_SDMA,      INT_USB_HOST1,
   INT_USB_HOST2, INT_USB_OTG,       INT_RESERVED3, INT_MSHC1,
   INT_MSHC2,     INT_IPU_ERR,       INT_IPU,       INT_RESERVED4,
   INT_RESERVED5, INT_UART1,         INT_UART4,     INT_UART5,
   INT_ETC_IRQ,   INT_SCC_SCM,       INT_SCC_SMN,   INT_GPIO2,
   INT_GPIO1,     INT_CCM_CLK,       INT_PCMCIA,    INT_WDOG,
   INT_GPIO3,     INT_RESERVED6,     INT_EXT_PWMG,  INT_EXT_TEMP,
   INT_EXT_SENS1, INT_EXT_SENS2,     INT_EXT_WDOG,  INT_EXT_TV,
   INT_ALL
};

void avic_init(void);
void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
                     unsigned long ni_priority, void (*handler)(void));
void avic_set_int_priority(enum IMX31_INT_LIST ints,
                           unsigned long ni_priority);
void avic_disable_int(enum IMX31_INT_LIST ints);
void avic_set_int_type(enum IMX31_INT_LIST ints, enum INT_TYPE intstype);

#define AVIC_NIL_DISABLE 15
#define AVIC_NIL_ENABLE  (-1)
void avic_set_ni_level(int level);


/* Call a service routine while allowing preemption by interrupts of higher
 * priority. Avoid using any app or other SVC stack by doing it with a mini
 * "stack on irq stack". Avoid actually enabling IRQ until the routine
 * decides to do so; epilogue code will always disable them again. */
#define AVIC_NESTED_NI_CALL_PROLOGUE(prio, stacksize) \
({ asm volatile ( \
        "sub    lr, lr, #4               \n" /* prepare return address */ \
        "srsdb  #0x12!                   \n" /* save LR_irq and SPSR_irq */ \
        "stmfd  sp!, { r0-r3, r12 }      \n" /* preserve context */ \
        "mov    r0, #0x68000000          \n" /* AVIC_BASE_ADDR */ \
        "mov    r1, %0                   \n" /* load interrupt level */ \
        "ldr    r2, [r0, #0x04]          \n" /* save NIMASK */ \
        "str    r1, [r0, #0x04]          \n" /* set interrupt level */ \
        "mov    r0, sp                   \n" /* grab IRQ stack */ \
        "sub    sp, sp, %1               \n" /* allocate space for routine to SP_irq */ \
        "cps    #0x13                    \n" /* change to SVC mode */ \
        "mov    r1, sp                   \n" /* save SP_svc */ \
        "mov    sp, r0                   \n" /* switch to SP_irq *copy* */ \
        "stmfd  sp!, { r1, r2, lr }      \n" /* push SP_svc, NIMASK and LR_svc */ \
        : : "i"(prio), "i"(stacksize)); })

#define AVIC_NESTED_NI_CALL_EPILOGUE(stacksize) \
({ asm volatile ( \
        "cpsid  i                        \n" /* disable IRQ */ \
        "ldmfd  sp!, { r1, r2, lr }      \n" /* pop SP_svc, NIMASK and LR_svc */ \
        "mov    sp, r1                   \n" /* restore SP_svc */ \
        "cps    #0x12                    \n" /* return to IRQ mode */ \
        "add    sp, sp, %0               \n" /* deallocate routine space */ \
        "mov    r0, #0x68000000          \n" /* AVIC BASE ADDR */ \
        "str    r2, [r0, #0x04]          \n" /* restore NIMASK */ \
        "ldmfd  sp!, { r0-r3, r12 }      \n" /* reload context */ \
        "rfefd  sp!                      \n" /* move stacked SPSR to CPSR, return */ \
        : : "i"(stacksize)); })

#endif /* AVIC_IMX31_H */
