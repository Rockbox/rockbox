/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Thom Johansen 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PP5020_H__
#define __PP5020_H__

/* All info gleaned and/or copied from the iPodLinux project. */

/* DRAM starts at 0x10000000, but in Rockbox we remap it to 0x00000000 */
#define DRAM_START       0x10000000

/* Processor ID */
#define PROCESSOR_ID     (*(volatile unsigned long *)(0x60000000))

#define PROC_ID_CPU      0x55
#define PROC_ID_COP      0xaa

/* Mailboxes */
/* Each processor has two mailboxes it can write to and two which
   it can read from.  We define the first to be for sending messages
   and the second for replying to messages */
#define CPU_MESSAGE      (*(volatile unsigned long *)(0x60001000))
#define COP_MESSAGE      (*(volatile unsigned long *)(0x60001004))
#define CPU_REPLY        (*(volatile unsigned long *)(0x60001008))
#define COP_REPLY        (*(volatile unsigned long *)(0x6000100c))

/* Interrupts */
#define CPU_INT_STAT        (*(volatile unsigned long*)(0x60004000))
#define COP_INT_STAT        (*(volatile unsigned long*)(0x60004004))
#define CPU_FIQ_STAT        (*(volatile unsigned long*)(0x60004008))
#define COP_FIQ_STAT        (*(volatile unsigned long*)(0x6000400c))

#define INT_STAT            (*(volatile unsigned long*)(0x60004010))
#define INT_FORCED_STAT     (*(volatile unsigned long*)(0x60004014))
#define INT_FORCED_SET      (*(volatile unsigned long*)(0x60004018))
#define INT_FORCED_CLR      (*(volatile unsigned long*)(0x6000401c))

#define CPU_INT_EN_STAT     (*(volatile unsigned long*)(0x60004020))
#define CPU_INT_EN          (*(volatile unsigned long*)(0x60004024))
#define CPU_INT_CLR         (*(volatile unsigned long*)(0x60004028))
#define CPU_INT_PRIORITY    (*(volatile unsigned long*)(0x6000402c))

#define COP_INT_EN_STAT     (*(volatile unsigned long*)(0x60004030))
#define COP_INT_EN          (*(volatile unsigned long*)(0x60004034))
#define COP_INT_CLR         (*(volatile unsigned long*)(0x60004038))
#define COP_INT_PRIORITY    (*(volatile unsigned long*)(0x6000403c))

#define CPU_HI_INT_STAT     (*(volatile unsigned long*)(0x60004100))
#define COP_HI_INT_STAT     (*(volatile unsigned long*)(0x60004104))
#define CPU_HI_FIQ_STAT     (*(volatile unsigned long*)(0x60004108))
#define COP_HI_FIQ_STAT     (*(volatile unsigned long*)(0x6000410c))

#define HI_INT_STAT         (*(volatile unsigned long*)(0x60004110))
#define HI_INT_FORCED_STAT  (*(volatile unsigned long*)(0x60004114))
#define HI_INT_FORCED_SET   (*(volatile unsigned long*)(0x60004118))
#define HI_INT_FORCED_CLR   (*(volatile unsigned long*)(0x6000411c))

#define CPU_HI_INT_EN_STAT  (*(volatile unsigned long*)(0x60004120))
#define CPU_HI_INT_EN       (*(volatile unsigned long*)(0x60004124))
#define CPU_HI_INT_CLR      (*(volatile unsigned long*)(0x60004128))
#define CPU_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000412c))
 
#define COP_HI_INT_EN_STAT  (*(volatile unsigned long*)(0x60004130))
#define COP_HI_INT_EN       (*(volatile unsigned long*)(0x60004134))
#define COP_HI_INT_CLR      (*(volatile unsigned long*)(0x60004138))
#define COP_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000413c))

#define TIMER1_IRQ   0
#define TIMER2_IRQ   1
#define MAILBOX_IRQ  4
#define I2S_IRQ      10
#define IDE_IRQ      23
#define USB_IRQ      24
#define FIREWIRE_IRQ 25
#define HI_IRQ       30
#define GPIO_IRQ     (32+0)
#define SER0_IRQ     (32+4)
#define SER1_IRQ     (32+5)
#define I2C_IRQ      (32+8)

#define TIMER1_MASK   (1 << TIMER1_IRQ)
#define TIMER2_MASK   (1 << TIMER2_IRQ)
#define MAILBOX_MASK  (1 << MAILBOX_IRQ)
#define I2S_MASK      (1 << I2S_IRQ)
#define IDE_MASK      (1 << IDE_IRQ)
#define USB_MASK      (1 << USB_IRQ)
#define FIREWIRE_MASK (1 << FIREWIRE_IRQ)
#define HI_MASK       (1 << HI_IRQ)
#define GPIO_MASK     (1 << (GPIO_IRQ-32))
#define SER0_MASK     (1 << (SER0_IRQ-32))
#define SER1_MASK     (1 << (SER1_IRQ-32))
#define I2C_MASK      (1 << (I2C_IRQ-32))

/* Timers */
#define TIMER1_CFG   (*(volatile unsigned long *)(0x60005000))
#define TIMER1_VAL   (*(volatile unsigned long *)(0x60005004))
#define TIMER2_CFG   (*(volatile unsigned long *)(0x60005008))
#define TIMER2_VAL   (*(volatile unsigned long *)(0x6000500c))
#define USEC_TIMER   (*(volatile unsigned long *)(0x60005010))
#define RTC          (*(volatile unsigned long *)(0x60005014))

/* Device Controller */
#define DEV_RS (*(volatile unsigned long *)(0x60006004))
#define DEV_EN (*(volatile unsigned long *)(0x6000600c))

#define DEV_SYSTEM      0x4
#define DEV_SER0        0x40
#define DEV_SER1        0x80
#define DEV_I2S         0x800
#define DEV_I2C         0x1000
#define DEV_OPTO        0x10000
#define DEV_PIEZO       0x10000
#define DEV_USB         0x400000
#define DEV_FIREWIRE    0x800000
#define DEV_IDE0        0x2000000

/* Processors Control */
#define CPU_CTL          (*(volatile unsigned long *)(0x60007000))
#define COP_CTL          (*(volatile unsigned long *)(0x60007004))

#define PROC_SLEEP   0x80000000
#define PROC_WAKE    0x0

/* Cache Control */
#define CACHE_CTL        (*(volatile unsigned long *)(0x6000c000))

#define CACHE_DISABLE    0
#define CACHE_ENABLE     1
#define CACHE_INIT       4

/* GPIO Ports */
#define GPIOA_ENABLE     (*(volatile unsigned long *)(0x6000d000))
#define GPIOB_ENABLE     (*(volatile unsigned long *)(0x6000d004))
#define GPIOC_ENABLE     (*(volatile unsigned long *)(0x6000d008))
#define GPIOD_ENABLE     (*(volatile unsigned long *)(0x6000d00c))
#define GPIOA_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d010))
#define GPIOB_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d014))
#define GPIOC_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d018))
#define GPIOD_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d01c))
#define GPIOA_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d020))
#define GPIOB_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d024))
#define GPIOC_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d028))
#define GPIOD_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d02c))
#define GPIOA_INPUT_VAL  (*(volatile unsigned long *)(0x6000d030))
#define GPIOB_INPUT_VAL  (*(volatile unsigned long *)(0x6000d034))
#define GPIOC_INPUT_VAL  (*(volatile unsigned long *)(0x6000d038))
#define GPIOD_INPUT_VAL  (*(volatile unsigned long *)(0x6000d03c))
#define GPIOA_INT_STAT   (*(volatile unsigned long *)(0x6000d040))
#define GPIOB_INT_STAT   (*(volatile unsigned long *)(0x6000d044))
#define GPIOC_INT_STAT   (*(volatile unsigned long *)(0x6000d048))
#define GPIOD_INT_STAT   (*(volatile unsigned long *)(0x6000d04c))
#define GPIOA_INT_EN     (*(volatile unsigned long *)(0x6000d050))
#define GPIOB_INT_EN     (*(volatile unsigned long *)(0x6000d054))
#define GPIOC_INT_EN     (*(volatile unsigned long *)(0x6000d058))
#define GPIOD_INT_EN     (*(volatile unsigned long *)(0x6000d05c))
#define GPIOA_INT_LEV    (*(volatile unsigned long *)(0x6000d060))
#define GPIOB_INT_LEV    (*(volatile unsigned long *)(0x6000d064))
#define GPIOC_INT_LEV    (*(volatile unsigned long *)(0x6000d068))
#define GPIOD_INT_LEV    (*(volatile unsigned long *)(0x6000d06c))
#define GPIOA_INT_CLR    (*(volatile unsigned long *)(0x6000d070))
#define GPIOB_INT_CLR    (*(volatile unsigned long *)(0x6000d074))
#define GPIOC_INT_CLR    (*(volatile unsigned long *)(0x6000d078))
#define GPIOD_INT_CLR    (*(volatile unsigned long *)(0x6000d07c))

#define GPIOE_ENABLE     (*(volatile unsigned long *)(0x6000d080))
#define GPIOF_ENABLE     (*(volatile unsigned long *)(0x6000d084))
#define GPIOG_ENABLE     (*(volatile unsigned long *)(0x6000d088))
#define GPIOH_ENABLE     (*(volatile unsigned long *)(0x6000d08c))
#define GPIOE_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d090))
#define GPIOF_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d094))
#define GPIOG_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d098))
#define GPIOH_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d09c))
#define GPIOE_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a0))
#define GPIOF_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a4))
#define GPIOG_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0a8))
#define GPIOH_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d0ac))
#define GPIOE_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b0))
#define GPIOF_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b4))
#define GPIOG_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0b8))
#define GPIOH_INPUT_VAL  (*(volatile unsigned long *)(0x6000d0bc))
#define GPIOE_INT_STAT   (*(volatile unsigned long *)(0x6000d0c0))
#define GPIOF_INT_STAT   (*(volatile unsigned long *)(0x6000d0c4))
#define GPIOG_INT_STAT   (*(volatile unsigned long *)(0x6000d0c8))
#define GPIOH_INT_STAT   (*(volatile unsigned long *)(0x6000d0cc))
#define GPIOE_INT_EN     (*(volatile unsigned long *)(0x6000d0d0))
#define GPIOF_INT_EN     (*(volatile unsigned long *)(0x6000d0d4))
#define GPIOG_INT_EN     (*(volatile unsigned long *)(0x6000d0d8))
#define GPIOH_INT_EN     (*(volatile unsigned long *)(0x6000d0dc))
#define GPIOE_INT_LEV    (*(volatile unsigned long *)(0x6000d0e0))
#define GPIOF_INT_LEV    (*(volatile unsigned long *)(0x6000d0e4))
#define GPIOG_INT_LEV    (*(volatile unsigned long *)(0x6000d0e8))
#define GPIOH_INT_LEV    (*(volatile unsigned long *)(0x6000d0ec))
#define GPIOE_INT_CLR    (*(volatile unsigned long *)(0x6000d0f0))
#define GPIOF_INT_CLR    (*(volatile unsigned long *)(0x6000d0f4))
#define GPIOG_INT_CLR    (*(volatile unsigned long *)(0x6000d0f8))
#define GPIOH_INT_CLR    (*(volatile unsigned long *)(0x6000d0fc))

#define GPIOI_ENABLE     (*(volatile unsigned long *)(0x6000d100))
#define GPIOJ_ENABLE     (*(volatile unsigned long *)(0x6000d104))
#define GPIOK_ENABLE     (*(volatile unsigned long *)(0x6000d108))
#define GPIOL_ENABLE     (*(volatile unsigned long *)(0x6000d10c))
#define GPIOI_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d110))
#define GPIOJ_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d114))
#define GPIOK_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d118))
#define GPIOL_OUTPUT_EN  (*(volatile unsigned long *)(0x6000d11c))
#define GPIOI_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d120))
#define GPIOJ_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d124))
#define GPIOK_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d128))
#define GPIOL_OUTPUT_VAL (*(volatile unsigned long *)(0x6000d12c))
#define GPIOI_INPUT_VAL  (*(volatile unsigned long *)(0x6000d130))
#define GPIOJ_INPUT_VAL  (*(volatile unsigned long *)(0x6000d134))
#define GPIOK_INPUT_VAL  (*(volatile unsigned long *)(0x6000d138))
#define GPIOL_INPUT_VAL  (*(volatile unsigned long *)(0x6000d13c))
#define GPIOI_INT_STAT   (*(volatile unsigned long *)(0x6000d140))
#define GPIOJ_INT_STAT   (*(volatile unsigned long *)(0x6000d144))
#define GPIOK_INT_STAT   (*(volatile unsigned long *)(0x6000d148))
#define GPIOL_INT_STAT   (*(volatile unsigned long *)(0x6000d14c))
#define GPIOI_INT_EN     (*(volatile unsigned long *)(0x6000d150))
#define GPIOJ_INT_EN     (*(volatile unsigned long *)(0x6000d154))
#define GPIOK_INT_EN     (*(volatile unsigned long *)(0x6000d158))
#define GPIOL_INT_EN     (*(volatile unsigned long *)(0x6000d15c))
#define GPIOI_INT_LEV    (*(volatile unsigned long *)(0x6000d160))
#define GPIOJ_INT_LEV    (*(volatile unsigned long *)(0x6000d164))
#define GPIOK_INT_LEV    (*(volatile unsigned long *)(0x6000d168))
#define GPIOL_INT_LEV    (*(volatile unsigned long *)(0x6000d16c))
#define GPIOI_INT_CLR    (*(volatile unsigned long *)(0x6000d170))
#define GPIOJ_INT_CLR    (*(volatile unsigned long *)(0x6000d174))
#define GPIOK_INT_CLR    (*(volatile unsigned long *)(0x6000d178))
#define GPIOL_INT_CLR    (*(volatile unsigned long *)(0x6000d17c))

/* Device initialization */
#define PP_VER1          (*(volatile unsigned long *)(0x70000000))
#define PP_VER2          (*(volatile unsigned long *)(0x70000004))
#define DEV_INIT         (*(volatile unsigned long *)(0x70000020))

#define INIT_USB         0x80000000

/* I2S */
#define IISCONFIG           (*(volatile unsigned long*)(0x70002800))
#define IISFIFO_CFG         (*(volatile unsigned long*)(0x7000280c))
#define IISFIFO_WR          (*(volatile unsigned long*)(0x70002840))
#define IISFIFO_RD          (*(volatile unsigned long*)(0x70002880))

/* Serial Controller */
#define SERIAL0             (*(volatile unsigned long*)(0x70006000))
#define SERIAL1             (*(volatile unsigned long*)(0x70006040))

/* I2C */
#define I2C_BASE            0x7000c000

/* EIDE Controller */
#define IDE0_PRI_TIMING0    (*(volatile unsigned long*)(0xc3000000))
#define IDE0_PRI_TIMING1    (*(volatile unsigned long*)(0xc3000004))
#define IDE0_SEC_TIMING0    (*(volatile unsigned long*)(0xc3000008))
#define IDE0_SEC_TIMING1    (*(volatile unsigned long*)(0xc300000c))

#define IDE1_PRI_TIMING0    (*(volatile unsigned long*)(0xc3000010))
#define IDE1_PRI_TIMING1    (*(volatile unsigned long*)(0xc3000014))
#define IDE1_SEC_TIMING0    (*(volatile unsigned long*)(0xc3000018))
#define IDE1_SEC_TIMING1    (*(volatile unsigned long*)(0xc300001c))

#define IDE0_CFG            (*(volatile unsigned long*)(0xc3000028))
#define IDE1_CFG            (*(volatile unsigned long*)(0xc300002c))

#define IDE0_CNTRLR_STAT    (*(volatile unsigned long*)(0xc30001e0))

/* USB controller */
#define USB_BASE            0xc5000000

/* Firewire Controller */
#define FIREWIRE_BASE       0xc6000000

/* Memory controller */
#define CACHE_BASE          (*(volatile unsigned long*)(0xf0000000))
#define CACHE_INIT_BASE     (*(volatile unsigned long*)(0xf0004000))
#define CACHE_FLUSH_BASE    (*(volatile unsigned long*)(0xf0008000))
#define CACHE_INVALID_BASE  (*(volatile unsigned long*)(0xf000c000))
#define MMAP0_LOGICAL       (*(volatile unsigned long*)(0xf000f000))
#define MMAP0_PHYSICAL      (*(volatile unsigned long*)(0xf000f004))
#define MMAP1_LOGICAL       (*(volatile unsigned long*)(0xf000f008))
#define MMAP1_PHYSICAL      (*(volatile unsigned long*)(0xf000f00c))
#define MMAP2_LOGICAL       (*(volatile unsigned long*)(0xf000f010))
#define MMAP2_PHYSICAL      (*(volatile unsigned long*)(0xf000f014))
#define MMAP3_LOGICAL       (*(volatile unsigned long*)(0xf000f018))
#define MMAP3_PHYSICAL      (*(volatile unsigned long*)(0xf000f01c))
#define CACHE_CTRL1         (*(volatile unsigned long*)(0xf000f020))
#define CACHE_CTRL2         (*(volatile unsigned long*)(0xf000f024))

#endif
