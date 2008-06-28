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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __PP5020_H__
#define __PP5020_H__

/* All info gleaned and/or copied from the iPodLinux project. */

#define QHARRAY_ATTR   __attribute__((section(".qharray"),nocommon))

/* DRAM starts at 0x10000000, but in Rockbox we remap it to 0x00000000 */
#define DRAM_START       0x10000000

/* Processor ID */
#define PROCESSOR_ID     (*(volatile unsigned long *)(0x60000000))

#define PROC_ID_CPU      0x55
#define PROC_ID_COP      0xaa

/* Mailboxes */
#define MBX_BASE            (0x60001000)
/* Read bits in the mailbox */
#define MBX_MSG_STAT        (*(volatile unsigned long *)(0x60001000))
/* Set bits in the mailbox */
#define MBX_MSG_SET         (*(volatile unsigned long *)(0x60001004))
/* Clear bits in the mailbox */
#define MBX_MSG_CLR         (*(volatile unsigned long *)(0x60001008))
/* Doesn't seem to be COP_REPLY at all :) */
#define MBX_UNKNOWN1        (*(volatile unsigned long *)(0x6000100c))
/* COP can set bit 29 - only CPU read clears it */
#define CPU_QUEUE           (*(volatile unsigned long *)(0x60001010))
/* CPU can set bit 29 - only COP read clears it */
#define COP_QUEUE           (*(volatile unsigned long *)(0x60001020))

#define PROC_QUEUE(core)    ((&CPU_QUEUE)[(core)*4])

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
#define CPU_INT_DIS         (*(volatile unsigned long*)(0x60004028))
#define CPU_INT_PRIORITY    (*(volatile unsigned long*)(0x6000402c))

#define COP_INT_EN_STAT     (*(volatile unsigned long*)(0x60004030))
#define COP_INT_EN          (*(volatile unsigned long*)(0x60004034))
#define COP_INT_DIS         (*(volatile unsigned long*)(0x60004038))
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
#define CPU_HI_INT_DIS      (*(volatile unsigned long*)(0x60004128))
#define CPU_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000412c))
 
#define COP_HI_INT_EN_STAT  (*(volatile unsigned long*)(0x60004130))
#define COP_HI_INT_EN       (*(volatile unsigned long*)(0x60004134))
#define COP_HI_INT_DIS      (*(volatile unsigned long*)(0x60004138))
#define COP_HI_INT_PRIORITY (*(volatile unsigned long*)(0x6000413c))

#define TIMER1_IRQ   0
#define TIMER2_IRQ   1
#define MAILBOX_IRQ  4
#define IIS_IRQ      10
#define USB_IRQ      20
#define IDE_IRQ      23
#define FIREWIRE_IRQ 25
#define HI_IRQ       30
#define GPIO0_IRQ    (32+0) /* Ports A..D */
#define GPIO1_IRQ    (32+1) /* Ports E..H */
#define GPIO2_IRQ    (32+2) /* Ports I..L */
#define SER0_IRQ     (32+4)
#define SER1_IRQ     (32+5)
#define I2C_IRQ      (32+8)

#define TIMER1_MASK   (1 << TIMER1_IRQ)
#define TIMER2_MASK   (1 << TIMER2_IRQ)
#define MAILBOX_MASK  (1 << MAILBOX_IRQ)
#define IIS_MASK      (1 << IIS_IRQ)
#define IDE_MASK      (1 << IDE_IRQ)
#define USB_MASK      (1 << USB_IRQ)
#define FIREWIRE_MASK (1 << FIREWIRE_IRQ)
#define HI_MASK       (1 << HI_IRQ)
#define GPIO0_MASK    (1 << (GPIO0_IRQ-32))
#define GPIO1_MASK    (1 << (GPIO1_IRQ-32))
#define GPIO2_MASK    (1 << (GPIO2_IRQ-32))
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
#define DEV_RS       (*(volatile unsigned long *)(0x60006004))
#define DEV_RS2      (*(volatile unsigned long *)(0x60006008))
#define DEV_EN       (*(volatile unsigned long *)(0x6000600c))
#define DEV_EN2      (*(volatile unsigned long *)(0x60006010))

#define DEV_EXTCLOCKS   0x00000002
#define DEV_SYSTEM      0x00000004
#define DEV_USB0        0x00000008
#define DEV_SER0        0x00000040
#define DEV_SER1        0x00000080
#define DEV_I2S         0x00000800
#define DEV_I2C         0x00001000
#define DEV_ATA         0x00004000
#define DEV_OPTO        0x00010000
#define DEV_PIEZO       0x00010000
#define DEV_PWM         0x00020000
#define DEV_USB1        0x00400000
#define DEV_FIREWIRE    0x00800000
#define DEV_IDE0        0x02000000
#define DEV_LCD         0x04000000

/* clock control */
#define CLOCK_SOURCE    (*(volatile unsigned long *)(0x60006020))
#define MLCD_SCLK_DIV   (*(volatile unsigned long *)(0x6000602c))
        /* bits 0..1: Mono LCD bridge serial clock divider: 1 / (n+1) */
#define PLL_CONTROL     (*(volatile unsigned long *)(0x60006034))
#define PLL_STATUS      (*(volatile unsigned long *)(0x6000603c))
#define ADC_CLOCK_SRC   (*(volatile unsigned long *)(0x60006094))
#define CLCD_CLOCK_SRC  (*(volatile unsigned long *)(0x600060a0))

/* Processors Control */
#define CPU_CTL          (*(volatile unsigned long *)(0x60007000))
#define COP_CTL          (*(volatile unsigned long *)(0x60007004))
#define PROC_CTL(core)   ((&CPU_CTL)[core])

/* Control flags, can be ORed together */
#define PROC_SLEEP     0x80000000  /* Sleep until an interrupt occurs */
#define PROC_WAIT_CNT  0x40000000  /* Sleep until end of countdown */
#define PROC_WAKE_INT  0x20000000  /* Fire interrupt on wake-up. Auto-clears. */

/* Counter source, select one */
#define PROC_CNT_CLKS  0x08000000  /* Clock cycles */
#define PROC_CNT_USEC  0x02000000  /* Microseconds */
#define PROC_CNT_MSEC  0x01000000  /* Milliseconds */
#define PROC_CNT_SEC   0x00800000  /* Seconds. Works on PP5022+ only! */

#define PROC_WAKE      0x00000000

/**
 * [22:8] - Semaphore flags for core communication? No execution effect observed
 *          [11:8] seem to often be set to the core's own ID
 *                 nybble when sleeping - 0x5 or 0xa.
 *  [7:0] - W: number of cycles to skip on next instruction
 *          R: cycles remaining
 * Executing on CPU
 *   CPU_CTL = 0x68000080
 *   nop
 * stalls the nop for 128 cycles
 * Reading CPU_CTL after the nop will return 0x48000000
 */


/* Cache Control */
#define CACHE_PRIORITY   (*(volatile unsigned long *)(0x60006044))
#define CACHE_CTL        (*(volatile unsigned long *)(0x6000c000))
#define CACHE_MASK       (*(volatile unsigned long *)(0xf000f040))
#define CACHE_OPERATION  (*(volatile unsigned long *)(0xf000f044))
#define CACHE_FLUSH_MASK (*(volatile unsigned long *)(0xf000f048))

/* CACHE_CTL bits */
#define CACHE_CTL_DISABLE    0x0000
#define CACHE_CTL_ENABLE     0x0001
#define CACHE_CTL_RUN        0x0002
#define CACHE_CTL_INIT       0x0004
#define CACHE_CTL_VECT_REMAP 0x0010
#define CACHE_CTL_READY      0x4000
#define CACHE_CTL_BUSY       0x8000
/* CACHE_OPERATION bits */
#define CACHE_OP_FLUSH       0x0002
#define CACHE_OP_INVALIDATE  0x0004

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

/* Standard GPIO addresses + 0x800 allow atomic port manipulation on PP502x.
 * Bits 8..15 of the written word define which bits are changed, bits 0..7
 * define the value of those bits. */

#define GPIO_SET_BITWISE(port, mask) \
        do { *(&port + (0x800/sizeof(long))) = (mask << 8) | mask; } while(0)

#define GPIO_CLEAR_BITWISE(port, mask) \
        do { *(&port + (0x800/sizeof(long))) = mask << 8; } while(0)

/* Device initialization */
#define PP_VER1          (*(volatile unsigned long *)(0x70000000))
#define PP_VER2          (*(volatile unsigned long *)(0x70000004))
#define STRAP_OPT_A      (*(volatile unsigned long *)(0x70000008))
#define STRAP_OPT_B      (*(volatile unsigned long *)(0x7000000c))
#define BUS_WIDTH_MASK   0x00000010
#define RAM_TYPE_MASK    0x000000c0
#define ROM_TYPE_MASK    0x00000008

#define DEV_INIT1        (*(volatile unsigned long *)(0x70000010))
#define DEV_INIT2        (*(volatile unsigned long *)(0x70000020))
/* some timing that needs to be handled during clock setup */
#define DEV_TIMING1      (*(volatile unsigned long *)(0x70000034))
#define XMB_NOR_CFG      (*(volatile unsigned long *)(0x70000038))
#define XMB_RAM_CFG      (*(volatile unsigned long *)(0x7000003c))

#define INIT_BUTTONS     0x00040000
#define INIT_PLL         0x40000000
#define INIT_USB         0x80000000

/* 32 bit GPO port */
#define GPO32_VAL        (*(volatile unsigned long *)(0x70000080))
#define GPO32_ENABLE     (*(volatile unsigned long *)(0x70000084))

/* IIS */
#define IISDIV              (*(volatile unsigned long*)(0x60006080))
#define IISCONFIG           (*(volatile unsigned long*)(0x70002800))
#define IISCLK              (*(volatile unsigned long*)(0x70002808))
#define IISFIFO_CFG         (*(volatile unsigned long*)(0x7000280c))
#define IISFIFO_WR          (*(volatile unsigned long*)(0x70002840))
#define IISFIFO_WRH         (*(volatile unsigned short*)(0x70002840))
#define IISFIFO_RD          (*(volatile unsigned long*)(0x70002880))
#define IISFIFO_RDH         (*(volatile unsigned short*)(0x70002880))

/**
 * IISCONFIG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * | RESET  |        |TXFIFOEN|RXFIFOEN|        |  ????  |   MS   |  ????  |
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * |        |        |        |        |        |        |        |        |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |        |        |        |        | Bus Format[1:0] |     Size[1:0]   |
 * |    7   |    6   |    5   |    4   |    3   |    2   |    1   |    0   |
 * |        |     Size Format[2:0]     |  ????  |  ????  | IRQTX  | IRQRX  |
 */

/* All IIS formats send MSB first */
#define IIS_RESET    (1 << 31)
#define IIS_TXFIFOEN (1 << 29)
#define IIS_RXFIFOEN (1 << 28)
#define IIS_MASTER   (1 << 25)
#define IIS_IRQTX    (1 << 1)
#define IIS_IRQRX    (1 << 0)

#define IIS_IRQTX_REG  IISCONFIG
#define IIS_IRQRX_REG  IISCONFIG

/* Data format on the IIS bus */
#define IIS_FORMAT_MASK  (0x3 << 10)
#define IIS_FORMAT_IIS   (0x0 << 10) /* Standard IIS - leading dummy bit */
#define IIS_FORMAT_1     (0x1 << 10)
#define IIS_FORMAT_LJUST (0x2 << 10) /* Left justified - no dummy bit */
#define IIS_FORMAT_3     (0x3 << 10)
/* Other formats not yet known  */

/* Data size on IIS bus */
#define IIS_SIZE_MASK   (0x3 << 8)
#define IIS_SIZE_16BIT  (0x0 << 8)
/* Other sizes not yet known */

/* Data size/format on IIS FIFO */
#define IIS_FIFO_FORMAT_MASK        (0x7 << 4)
#define IIS_FIFO_FORMAT_LE_HALFWORD (0x0 << 4)
/* Big-endian formats - data sent to the FIFO must be big endian.
 * I forgot which is which size but did test them. */
#define IIS_FIFO_FORMAT_1           (0x1 << 4)
#define IIS_FIFO_FORMAT_2           (0x2 << 4)
/* 32bit-MSB-little endian */
#define IIS_FIFO_FORMAT_LE32        (0x3 << 4)
/* 16bit-MSB-little endian */
#define IIS_FIFO_FORMAT_LE16        (0x4 << 4)
#define IIS_FIFO_FORMAT_5           (0x5 << 4)
#define IIS_FIFO_FORMAT_6           (0x6 << 4)
/* A second one like IIS_FIFO_FORMAT_LE16? PP5020 only? */
#define IIS_FIFO_FORMAT_LE16_2      (0x7 << 4)

/* FIFO formats 0x5 and above seem equivalent to 0x4 ?? */

/**
 * IISFIFO_CFG bits:
 * |   31   |   30   |   29   |   28   |   27   |   26   |   25   |   24   |
 * |        |        |                      RXFull[5:0]                    |
 * |   23   |   22   |   21   |   20   |   19   |   18   |   17   |   16   |
 * |        |        |                      TXFree[5:0]                    |
 * |   15   |   14   |   13   |   12   |   11   |   10   |    9   |    8   |
 * |        |        |        | RXCLR  |        |        |        | TXCLR  |
 * |    7   |    6   |    5   |    4   |    3   |    2   |    1   |    0   |
 * |        |        |   RX_FULL_LVL   |        |        |  TX_EMPTY_LVL   |
 */

/* handy macros to extract the FIFO counts */
#define IIS_RX_FULL_MASK (0x3f << 24)
#define IIS_RX_FULL_COUNT \
    ((IISFIFO_CFG & IIS_RX_FULL_MASK) >> 24)

#define IIS_TX_FREE_MASK (0x3f << 16)
#define IIS_TX_FREE_COUNT \
    ((IISFIFO_CFG & IIS_TX_FREE_MASK) >> 16)

#define IIS_RXCLR (1 << 12)
#define IIS_TXCLR (1 << 8)
/* Number of slots */
#define IIS_RX_FULL_LVL_4  (0x1 << 4)
#define IIS_RX_FULL_LVL_8  (0x2 << 4)
#define IIS_RX_FULL_LVL_12 (0x3 << 4)

#define IIS_TX_EMPTY_LVL_4  (0x1 << 0)
#define IIS_TX_EMPTY_LVL_8  (0x2 << 0)
#define IIS_TX_EMPTY_LVL_12 (0x3 << 0)

/* Note: didn't bother to see of levels 0 and 16 actually work */

/* First ("mono") LCD bridge */
#define LCD1_BASE           0x70003000

#define LCD1_CONTROL        (*(volatile unsigned long *)(0x70003000))
#define LCD1_CMD            (*(volatile unsigned long *)(0x70003008))
#define LCD1_DATA           (*(volatile unsigned long *)(0x70003010))

#define LCD1_BUSY_MASK      0x8000

/* Serial Controller */
#define SER0_BASE           (*(volatile unsigned long*)(0x70006000))

#define SER0_RBR            (*(volatile unsigned long*)(0x70006000))
#define SER0_THR            (*(volatile unsigned long*)(0x70006000))
#define SER0_IER            (*(volatile unsigned long*)(0x70006004))
#define SER0_FCR            (*(volatile unsigned long*)(0x70006008))
#define SER0_IIR            (*(volatile unsigned long*)(0x70006008))
#define SER0_LCR            (*(volatile unsigned long*)(0x7000600c))
#define SER0_MCR            (*(volatile unsigned long*)(0x70006010))
#define SER0_LSR            (*(volatile unsigned long*)(0x70006014))
#define SER0_MSR            (*(volatile unsigned long*)(0x70006018))
#define SER0_SPR            (*(volatile unsigned long*)(0x7000601c))

#define SER0_DLL            (*(volatile unsigned long*)(0x70006000))
#define SER0_DLM            (*(volatile unsigned long*)(0x70006004))

#define SER1_BASE           (*(volatile unsigned long*)(0x70006040))

#define SER1_RBR            (*(volatile unsigned long*)(0x70006040))
#define SER1_THR            (*(volatile unsigned long*)(0x70006040))
#define SER1_IER            (*(volatile unsigned long*)(0x70006044))
#define SER1_FCR            (*(volatile unsigned long*)(0x70006048))
#define SER1_IIR            (*(volatile unsigned long*)(0x70006048))
#define SER1_LCR            (*(volatile unsigned long*)(0x7000604c))
#define SER1_MCR            (*(volatile unsigned long*)(0x70006050))
#define SER1_LSR            (*(volatile unsigned long*)(0x70006054))
#define SER1_MSR            (*(volatile unsigned long*)(0x70006058))
#define SER1_SPR            (*(volatile unsigned long*)(0x7000605c))

#define SER1_DLL            (*(volatile unsigned long*)(0x70006040))
#define SER1_DLM            (*(volatile unsigned long*)(0x70006044))

/* Second ("color") LCD bridge */
#define LCD2_BASE           0x70008a00

#define LCD2_PORT           (*(volatile unsigned long*)(0x70008a0c))
#define LCD2_BLOCK_CTRL     (*(volatile unsigned long*)(0x70008a20))
#define LCD2_BLOCK_CONFIG   (*(volatile unsigned long*)(0x70008a24))
#define LCD2_BLOCK_DATA     (*(volatile unsigned long*)(0x70008b00))

#define LCD2_BUSY_MASK      0x80000000
#define LCD2_CMD_MASK       0x80000000
#define LCD2_DATA_MASK      0x81000000

#define LCD2_BLOCK_READY    0x04000000
#define LCD2_BLOCK_TXOK     0x01000000

/* I2C */
#define I2C_BASE            0x7000c000

/* EIDE Controller */
#define IDE_BASE            0xc3000000

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
#define CACHE_BASE              (*(volatile unsigned long*)(0xf0000000))
/* 0xf0000000-0xf0001fff */
#define CACHE_DATA_BASE         (*(volatile unsigned long*)(0xf0000000))
/* 0xf0002000-0xf0003fff */
#define CACHE_DATA_MIRROR_BASE  (*(volatile unsigned long*)(0xf0002000))
/* 0xf0004000-0xf0007fff */
#define CACHE_STATUS_BASE       (*(volatile unsigned long*)(0xf0004000))
#define CACHE_FLUSH_BASE        (*(volatile unsigned long*)(0xf0008000))
#define CACHE_INVALID_BASE      (*(volatile unsigned long*)(0xf000c000))
#define MMAP_PHYS_READ_MASK     0x0100
#define MMAP_PHYS_WRITE_MASK    0x0200
#define MMAP_PHYS_DATA_MASK     0x0400
#define MMAP_PHYS_CODE_MASK     0x0800
#define MMAP_FIRST              (*(volatile unsigned long*)(0xf000f000))
#define MMAP_LAST               (*(volatile unsigned long*)(0xf000f03c))
#define MMAP0_LOGICAL           (*(volatile unsigned long*)(0xf000f000))
#define MMAP0_PHYSICAL          (*(volatile unsigned long*)(0xf000f004))
#define MMAP1_LOGICAL           (*(volatile unsigned long*)(0xf000f008))
#define MMAP1_PHYSICAL          (*(volatile unsigned long*)(0xf000f00c))
#define MMAP2_LOGICAL           (*(volatile unsigned long*)(0xf000f010))
#define MMAP2_PHYSICAL          (*(volatile unsigned long*)(0xf000f014))
#define MMAP3_LOGICAL           (*(volatile unsigned long*)(0xf000f018))
#define MMAP3_PHYSICAL          (*(volatile unsigned long*)(0xf000f01c))
#define MMAP4_LOGICAL           (*(volatile unsigned long*)(0xf000f020))
#define MMAP4_PHYSICAL          (*(volatile unsigned long*)(0xf000f024))
#define MMAP5_LOGICAL           (*(volatile unsigned long*)(0xf000f028))
#define MMAP5_PHYSICAL          (*(volatile unsigned long*)(0xf000f02c))
#define MMAP6_LOGICAL           (*(volatile unsigned long*)(0xf000f030))
#define MMAP6_PHYSICAL          (*(volatile unsigned long*)(0xf000f034))
#define MMAP7_LOGICAL           (*(volatile unsigned long*)(0xf000f038))
#define MMAP7_PHYSICAL          (*(volatile unsigned long*)(0xf000f03c))

#endif /* __PP5020_H__ */
