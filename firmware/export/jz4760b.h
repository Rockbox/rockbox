/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Vortex
 * Copyright (C) 2008 Ingenic Semiconductor Inc.
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

/*
 *  jz4760b.h
 *
 *  JZ4760B common definition.
 *
 *  Copyright (C) 2008 Ingenic Semiconductor Inc.
 *
 *  Author: <cwjia@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ4760B_H__
#define __JZ4760B_H__

#if defined(__ASSEMBLY__) || defined(__LANGUAGE_ASSEMBLY)
        #ifndef __MIPS_ASSEMBLER
                #define __MIPS_ASSEMBLER
        #endif
        #define REG8(addr)	(addr)
        #define REG16(addr)	(addr)
        #define REG32(addr)	(addr)
#else
	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned int u32;

        #define REG8(addr)	*((volatile unsigned char *)(addr))
        #define REG16(addr)	*((volatile unsigned short *)(addr))
        #define REG32(addr)	*((volatile unsigned int *)(addr))

        #define INREG8(x)               ((unsigned char)(*(volatile unsigned char *)(x)))
        #define OUTREG8(x, y)           *(volatile unsigned char *)(x) = (y)
        #define SETREG8(x, y)           OUTREG8(x, INREG8(x)|(y))
        #define CLRREG8(x, y)           OUTREG8(x, INREG8(x)&~(y))
        #define CMSREG8(x, y, m)        OUTREG8(x, (INREG8(x)&~(m))|(y))

        #define INREG16(x)              ((unsigned short)(*(volatile unsigned short *)(x)))
        #define OUTREG16(x, y)          *(volatile unsigned short *)(x) = (y)
        #define SETREG16(x, y)          OUTREG16(x, INREG16(x)|(y))
        #define CLRREG16(x, y)          OUTREG16(x, INREG16(x)&~(y))
        #define CMSREG16(x, y, m)       OUTREG16(x, (INREG16(x)&~(m))|(y))

        #define INREG32(x)              ((unsigned int)(*(volatile unsigned int *)(x)))
        #define OUTREG32(x, y)          *(volatile unsigned int *)(x) = (y)
        #define SETREG32(x, y)          OUTREG32(x, INREG32(x)|(y))
        #define CLRREG32(x, y)          OUTREG32(x, INREG32(x)&~(y))
        #define CMSREG32(x, y, m)       OUTREG32(x, (INREG32(x)&~(m))|(y))
#endif

/*
 * Define the bit field macro to avoid the bit mistake
 */
#define BIT0            (1 << 0)
#define BIT1            (1 << 1)
#define BIT2            (1 << 2)
#define BIT3            (1 << 3)
#define BIT4            (1 << 4)
#define BIT5            (1 << 5)
#define BIT6            (1 << 6)
#define BIT7            (1 << 7)
#define BIT8            (1 << 8)
#define BIT9            (1 << 9)
#define BIT10           (1 << 10)
#define BIT11           (1 << 11)
#define BIT12 	        (1 << 12)
#define BIT13 	        (1 << 13)
#define BIT14 	        (1 << 14)
#define BIT15 	        (1 << 15)
#define BIT16 	        (1 << 16)
#define BIT17 	        (1 << 17)
#define BIT18 	        (1 << 18)
#define BIT19 	        (1 << 19)
#define BIT20 	        (1 << 20)
#define BIT21 	        (1 << 21)
#define BIT22 	        (1 << 22)
#define BIT23 	        (1 << 23)
#define BIT24 	        (1 << 24)
#define BIT25 	        (1 << 25)
#define BIT26 	        (1 << 26)
#define BIT27 	        (1 << 27)
#define BIT28 	        (1 << 28)
#define BIT29 	        (1 << 29)
#define BIT30 	        (1 << 30)
#define BIT31 	        (1 << 31)

/* Generate the bit field mask from msb to lsb */
#define BITS_H2L(msb, lsb)  ((0xFFFFFFFF >> (32-((msb)-(lsb)+1))) << (lsb))

/* Get the bit field value from the data which is read from the register */
#define get_bf_value(data, lsb, mask)  (((data) & (mask)) >> (lsb))

/*
 * Define the module base addresses
 */
/* AHB0 BUS Devices Base */
#define HARB0_BASE	0xB3000000
/* AHB1 BUS Devices Base */
#define HARB1_BASE	0xB3200000
#define	DMAGP0_BASE	0xB3210000
#define	DMAGP1_BASE	0xB3220000
#define	DMAGP2_BASE	0xB3230000
#define	DEBLK_BASE	0xB3270000
#define	IDCT_BASE	0xB3280000
#define	CABAC_BASE	0xB3290000
#define	TCSM0_BASE	0xB32B0000
#define	TCSM1_BASE	0xB32C0000
#define	SRAM_BASE	0xB32D0000
/* AHB2 BUS Devices Base */
#define HARB2_BASE	0xB3400000
#define UHC_BASE	0xB3430000
#define GPS_BASE	0xB3480000
#define ETHC_BASE	0xB34B0000
/* APB BUS Devices Base */
#define	PS2_BASE	0xB0060000

/*
 * General purpose I/O port module(GPIO) address definition
 */
#define	GPIO_BASE	0xb0010000

/* GPIO group offset */
#define GPIO_GOS	0x100

/* Each group address */
#define GPIO_BASEA	(GPIO_BASE + (0) * GPIO_GOS)
#define GPIO_BASEB	(GPIO_BASE + (1) * GPIO_GOS)
#define GPIO_BASEC	(GPIO_BASE + (2) * GPIO_GOS)
#define GPIO_BASED	(GPIO_BASE + (3) * GPIO_GOS)
#define GPIO_BASEE	(GPIO_BASE + (4) * GPIO_GOS)
#define GPIO_BASEF	(GPIO_BASE + (5) * GPIO_GOS)

/*
 * GPIO registers offset address definition
 */
#define GPIO_PXPIN_OFFSET	(0x00)	/*  r, 32, 0x00000000 */
#define GPIO_PXDAT_OFFSET	(0x10)	/*  r, 32, 0x00000000 */
#define GPIO_PXDATS_OFFSET	(0x14)  /*  w, 32, 0x???????? */
#define GPIO_PXDATC_OFFSET	(0x18)  /*  w, 32, 0x???????? */
#define GPIO_PXIM_OFFSET	(0x20)  /*  r, 32, 0xffffffff */
#define GPIO_PXIMS_OFFSET	(0x24)  /*  w, 32, 0x???????? */
#define GPIO_PXIMC_OFFSET	(0x28)  /*  w, 32, 0x???????? */
#define GPIO_PXPE_OFFSET	(0x30)  /*  r, 32, 0x00000000 */
#define GPIO_PXPES_OFFSET	(0x34)  /*  w, 32, 0x???????? */
#define GPIO_PXPEC_OFFSET	(0x38)  /*  w, 32, 0x???????? */
#define GPIO_PXFUN_OFFSET	(0x40)  /*  r, 32, 0x00000000 */
#define GPIO_PXFUNS_OFFSET	(0x44)  /*  w, 32, 0x???????? */
#define GPIO_PXFUNC_OFFSET	(0x48)  /*  w, 32, 0x???????? */
#define GPIO_PXSEL_OFFSET	(0x50)  /*  r, 32, 0x00000000 */
#define GPIO_PXSELS_OFFSET	(0x54)  /*  w, 32, 0x???????? */
#define GPIO_PXSELC_OFFSET	(0x58)  /*  w, 32, 0x???????? */
#define GPIO_PXDIR_OFFSET	(0x60)  /*  r, 32, 0x00000000 */
#define GPIO_PXDIRS_OFFSET	(0x64)  /*  w, 32, 0x???????? */
#define GPIO_PXDIRC_OFFSET	(0x68)  /*  w, 32, 0x???????? */
#define GPIO_PXTRG_OFFSET	(0x70)  /*  r, 32, 0x00000000 */
#define GPIO_PXTRGS_OFFSET	(0x74)  /*  w, 32, 0x???????? */
#define GPIO_PXTRGC_OFFSET	(0x78)  /*  w, 32, 0x???????? */
#define GPIO_PXFLG_OFFSET	(0x80)  /*  r, 32, 0x00000000 */
#define GPIO_PXFLGC_OFFSET	(GPIO_PXDATS_OFFSET)  /*  w, 32, 0x???????? */
#define GPIO_PXDS0_OFFSET	(0xC0)  /*  r, 32, 0x00000000 */
#define GPIO_PXDS0S_OFFSET	(0xC4)  /*  w, 32, 0x00000000 */
#define GPIO_PXDS0C_OFFSET	(0xC8)  /*  w, 32, 0x00000000 */
#define GPIO_PXDS1_OFFSET	(0xD0)  /*  r, 32, 0x00000000 */
#define GPIO_PXDS1S_OFFSET	(0xD4)  /*  w, 32, 0x00000000 */
#define GPIO_PXDS1C_OFFSET	(0xD8)  /*  w, 32, 0x00000000 */
#define GPIO_PXDS2_OFFSET	(0xE0)  /*  r, 32, 0x00000000 */
#define GPIO_PXDS2S_OFFSET	(0xE4)  /*  w, 32, 0x00000000 */
#define GPIO_PXDS2C_OFFSET	(0xE8)  /*  w, 32, 0x00000000 */
#define GPIO_PXSL_OFFSET	(0xF0)  /*  r, 32, 0x00000000 */
#define GPIO_PXSLS_OFFSET	(0xF4)  /*  w, 32, 0x00000000 */
#define GPIO_PXSLC_OFFSET	(0xF8)  /*  w, 32, 0x00000000 */

/*
 * GPIO registers address definition
 */
#define GPIO_PXPIN(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXPIN_OFFSET)
#define GPIO_PXDAT(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDAT_OFFSET)
#define GPIO_PXDATS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDATS_OFFSET)
#define GPIO_PXDATC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDATC_OFFSET)
#define GPIO_PXIM(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXIM_OFFSET)
#define GPIO_PXIMS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXIMS_OFFSET)
#define GPIO_PXIMC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXIMC_OFFSET)
#define GPIO_PXPE(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXPE_OFFSET)
#define GPIO_PXPES(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXPES_OFFSET)
#define GPIO_PXPEC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXPEC_OFFSET)
#define GPIO_PXFUN(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXFUN_OFFSET)
#define GPIO_PXFUNS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXFUNS_OFFSET)
#define GPIO_PXFUNC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXFUNC_OFFSET)
#define GPIO_PXSEL(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSEL_OFFSET)
#define GPIO_PXSELS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSELS_OFFSET)
#define GPIO_PXSELC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSELC_OFFSET)
#define GPIO_PXDIR(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDIR_OFFSET)
#define GPIO_PXDIRS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDIRS_OFFSET)
#define GPIO_PXDIRC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDIRC_OFFSET)
#define GPIO_PXTRG(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXTRG_OFFSET)
#define GPIO_PXTRGS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXTRGS_OFFSET)
#define GPIO_PXTRGC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXTRGC_OFFSET)
#define GPIO_PXFLG(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXFLG_OFFSET)
#define GPIO_PXFLGC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXFLGC_OFFSET)
#define GPIO_PXDS0(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS0_OFFSET)
#define GPIO_PXDS0S(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS0S_OFFSET)
#define GPIO_PXDS0C(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS0C_OFFSET)
#define GPIO_PXDS1(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS1_OFFSET)
#define GPIO_PXDS1S(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS1S_OFFSET)
#define GPIO_PXDS1C(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS1C_OFFSET)
#define GPIO_PXDS2(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS2_OFFSET)
#define GPIO_PXDS2S(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS2S_OFFSET)
#define GPIO_PXDS2C(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXDS2C_OFFSET)
#define GPIO_PXSL(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSL_OFFSET)
#define GPIO_PXSLS(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSLS_OFFSET)
#define GPIO_PXSLC(n)	(GPIO_BASE + (n)*GPIO_GOS + GPIO_PXSLC_OFFSET)

/*  */
#define GPIO_PORT_NUM   6
#define MAX_GPIO_NUM	192
#define GPIO_WAKEUP     (30)

#ifndef __MIPS_ASSEMBLER

//n = 0,1,2,3,4,5 (PORTA, PORTB, PORTC, PORTD, PORTE, PORTF)
#define REG_GPIO_PXPIN(n)	REG32(GPIO_PXPIN(n))
#define REG_GPIO_PXDAT(n)	REG32(GPIO_PXDAT(n))
#define REG_GPIO_PXDATS(n)	REG32(GPIO_PXDATS(n))
#define REG_GPIO_PXDATC(n)	REG32(GPIO_PXDATC(n))
#define REG_GPIO_PXIM(n)	REG32(GPIO_PXIM(n))
#define REG_GPIO_PXIMS(n)	REG32(GPIO_PXIMS(n))
#define REG_GPIO_PXIMC(n)	REG32(GPIO_PXIMC(n))
#define REG_GPIO_PXPE(n)	REG32(GPIO_PXPE(n))
#define REG_GPIO_PXPES(n)	REG32(GPIO_PXPES(n))
#define REG_GPIO_PXPEC(n)	REG32(GPIO_PXPEC(n))
#define REG_GPIO_PXFUN(n)	REG32(GPIO_PXFUN(n))
#define REG_GPIO_PXFUNS(n)	REG32(GPIO_PXFUNS(n))
#define REG_GPIO_PXFUNC(n)	REG32(GPIO_PXFUNC(n))
#define REG_GPIO_PXSEL(n)	REG32(GPIO_PXSEL(n))
#define REG_GPIO_PXSELS(n)	REG32(GPIO_PXSELS(n))
#define REG_GPIO_PXSELC(n)	REG32(GPIO_PXSELC(n))
#define REG_GPIO_PXDIR(n)	REG32(GPIO_PXDIR(n))
#define REG_GPIO_PXDIRS(n)	REG32(GPIO_PXDIRS(n))
#define REG_GPIO_PXDIRC(n)	REG32(GPIO_PXDIRC(n))
#define REG_GPIO_PXTRG(n)	REG32(GPIO_PXTRG(n))
#define REG_GPIO_PXTRGS(n)	REG32(GPIO_PXTRGS(n))
#define REG_GPIO_PXTRGC(n)	REG32(GPIO_PXTRGC(n))
#define REG_GPIO_PXFLG(n)	REG32(GPIO_PXFLG(n))
#define REG_GPIO_PXFLGC(n)	REG32(GPIO_PXFLGC(n))
#define REG_GPIO_PXDS0(n) 	REG32(GPIO_PXDS0(n))
#define REG_GPIO_PXDS0S(n) 	REG32(GPIO_PXDS0S(n))
#define REG_GPIO_PXDS0C(n) 	REG32(GPIO_PXDS0C(n))
#define REG_GPIO_PXDS1(n) 	REG32(GPIO_PXDS1(n))
#define REG_GPIO_PXDS1S(n) 	REG32(GPIO_PXDS1S(n))
#define REG_GPIO_PXDS1C(n) 	REG32(GPIO_PXDS1C(n))
#define REG_GPIO_PXDS2(n) 	REG32(GPIO_PXDS2(n))
#define REG_GPIO_PXDS2S(n) 	REG32(GPIO_PXDS2S(n))
#define REG_GPIO_PXDS2C(n) 	REG32(GPIO_PXDS2C(n))
#define REG_GPIO_PXSL(n) 	REG32(GPIO_PXSL(n))
#define REG_GPIO_PXSLS(n) 	REG32(GPIO_PXSLS(n))
#define REG_GPIO_PXSLC(n) 	REG32(GPIO_PXSLC(n))

/*----------------------------------------------------------------
 * p is the port number (0,1,2,3,4,5)
 * o is the pin offset (0-31) inside the port
 * n is the absolute number of a pin (0-127), regardless of the port
 */

//----------------------------------------------------------------
// Function Pins Mode

#define __gpio_as_func0(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXFUNS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXSELC(p) = (1 << o);		\
} while (0)

#define __gpio_as_func1(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXFUNS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
} while (0)

#define __gpio_as_func2(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXFUNS(p) = (1 << o);		\
	REG_GPIO_PXTRGS(p) = (1 << o);		\
	REG_GPIO_PXSELC(p) = (1 << o);		\
} while (0)

#define __gpio_as_func3(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXFUNS(p) = (1 << o);		\
	REG_GPIO_PXTRGS(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
} while (0)

/*
 * UART0_TxD, UART0_RxD
 */
#define __gpio_as_uart0()			\
do {						\
	unsigned int bits = BIT3 | BIT0;	\
	REG_GPIO_PXFUNS(5) = bits;		\
	REG_GPIO_PXTRGC(5) = bits;		\
	REG_GPIO_PXSELC(5) = bits;		\
	REG_GPIO_PXPES(5)  = bits;		\
} while (0)

/*
 * UART0_TxD, UART0_RxD, UART0_CTS, UART0_RTS
 */
#define __gpio_as_uart0_ctsrts()		\
do {						\
	unsigned int bits = BITS_H2L(3, 0);	\
	REG_GPIO_PXFUNS(5) = bits;		\
	REG_GPIO_PXTRGC(5) = bits;		\
	REG_GPIO_PXSELC(5) = bits;		\
	REG_GPIO_PXPES(5)  = bits;		\
} while (0)

/*
 * UART1_TxD, UART1_RxD
 */
#define __gpio_as_uart1()			\
do {						\
	unsigned int bits = BIT28 | BIT26;	\
	REG_GPIO_PXFUNS(3) = bits;		\
	REG_GPIO_PXTRGC(3) = bits;		\
	REG_GPIO_PXSELC(3) = bits;		\
	REG_GPIO_PXPES(3)  = bits;		\
} while (0)

/*
 * UART1_TxD, UART1_RxD, UART1_CTS, UART1_RTS
 */
#define __gpio_as_uart1_ctsrts()		\
do {						\
	unsigned int bits = BITS_H2L(29, 26);	\
	REG_GPIO_PXFUNS(3) = bits;		\
	REG_GPIO_PXTRGC(3) = bits;		\
	REG_GPIO_PXSELC(3) = bits;		\
	REG_GPIO_PXPES(3)  = bits;		\
} while (0)

/*
 * UART2_TxD, UART2_RxD
 */
#define __gpio_as_uart2()			\
do {						\
	unsigned int bits = BIT30 | BIT28;	\
	REG_GPIO_PXFUNS(2) = bits;		\
	REG_GPIO_PXTRGC(2) = bits;		\
	REG_GPIO_PXSELC(2) = bits;		\
	REG_GPIO_PXPES(2)  = bits;		\
} while (0)

/*
 * UART2_TxD, UART2_RxD, UART2_CTS, UART2_RTS
 */
#define __gpio_as_uart2_ctsrts()		\
do {						\
	unsigned int bits = BITS_H2L(31, 28);	\
	REG_GPIO_PXFUNS(2) = bits;		\
	REG_GPIO_PXTRGC(2) = bits;		\
	REG_GPIO_PXSELC(2) = bits;		\
	REG_GPIO_PXPES(2)  = bits;		\
} while (0)

/* WARNING: the folloing macro do NOT check */
/*
 * UART3_TxD, UART3_RxD
 */
#define __gpio_as_uart3()			\
do {						\
	unsigned int bits = BIT12;	\
	REG_GPIO_PXFUNS(3) = bits;		\
	REG_GPIO_PXTRGC(3) = bits;		\
	REG_GPIO_PXSELS(3) = bits;		\
	REG_GPIO_PXPES(3)  = bits;	\
	bits = BIT5;	\
	REG_GPIO_PXFUNS(4) = bits;		\
	REG_GPIO_PXTRGC(4) = bits;		\
	REG_GPIO_PXSELS(4) = bits;		\
	REG_GPIO_PXPES(4)  = bits;	\
} while (0)
/*
 * UART3_TxD, UART3_RxD, UART3_CTS, UART3_RTS
 */
#define __gpio_as_uart3_ctsrts()		\
do {						\
	REG_GPIO_PXFUNS(3) = (1 << 12);		\
	REG_GPIO_PXTRGC(3) = (1 << 12);		\
	REG_GPIO_PXSELC(3) = (1 << 12);		\
	REG_GPIO_PXPES(3)  = (1 << 12);		\
	REG_GPIO_PXFUNS(4) = 0x00000320;	\
	REG_GPIO_PXTRGC(4) = 0x00000320;	\
	REG_GPIO_PXSELS(4) = 0x00000020;	\
	REG_GPIO_PXSELC(4) = 0x00000300;	\
	REG_GPIO_PXPES(4)  = 0x00000320;	\
} while (0)

/*
 * SD0 ~ SD7, CS1#, CLE, ALE, FRE#, FWE#, FRB#
 * @n: chip select number(1 ~ 6)
 */
#define __gpio_as_nand_8bit(n)						\
do {		              						\
									\
	REG_GPIO_PXFUNS(0) = 0x000c00ff; /* SD0 ~ SD7, FRE#, FWE# */ \
	REG_GPIO_PXSELC(0) = 0x000c00ff;				\
	REG_GPIO_PXTRGC(0) = 0x000c00ff;				\
	REG_GPIO_PXPES(0) = 0x000c00ff;					\
	REG_GPIO_PXFUNS(1) = 0x00000003; /* CLE(SA0_CL), ALE(SA1_AL) */	\
	REG_GPIO_PXSELC(1) = 0x00000003;				\
	REG_GPIO_PXTRGC(1) = 0x00000003;				\
	REG_GPIO_PXPES(1) = 0x00000003;					\
									\
	REG_GPIO_PXFUNS(0) = 0x00200000 << ((n)-1); /* CSn */		\
	REG_GPIO_PXSELC(0) = 0x00200000 << ((n)-1);			\
	REG_GPIO_PXTRGC(0) = 0x00200000 << ((n)-1);			\
	REG_GPIO_PXPES(0) = 0x00200000 << ((n)-1);			\
									\
 	REG_GPIO_PXFUNC(0) = 0x00100000; /* FRB#(input) */		\
	REG_GPIO_PXSELC(0) = 0x00100000;				\
	REG_GPIO_PXDIRC(0) = 0x00100000;				\
	REG_GPIO_PXPES(0) = 0x00100000;					\
} while (0)

#define __gpio_as_nand_16bit(n)						\
do {		              						\
									\
	REG_GPIO_PXFUNS(0) = 0x000cffff; /* SD0 ~ SD15, CS1#, FRE#, FWE# */ \
	REG_GPIO_PXSELC(0) = 0x000cffff;				\
	REG_GPIO_PXTRGC(0) = 0x000cffff;				\
	REG_GPIO_PXPES(0) = 0x000cffff;					\
	REG_GPIO_PXFUNS(1) = 0x00000003; /* CLE(SA2), ALE(SA3) */	\
	REG_GPIO_PXSELC(1) = 0x00000003;				\
	REG_GPIO_PXTRGC(1) = 0x00000003;				\
	REG_GPIO_PXPES(1) = 0x00000003;					\
									\
	REG_GPIO_PXFUNS(0) = 0x00200000 << ((n)-1); /* CSn */		\
	REG_GPIO_PXSELC(0) = 0x00200000 << ((n)-1);			\
	REG_GPIO_PXTRGC(0) = 0x00200000 << ((n)-1);			\
	REG_GPIO_PXPES(0) = 0x00200000 << ((n)-1);			\
									\
 	REG_GPIO_PXFUNC(0) = 0x00100000; /* FRB#(input) */		\
	REG_GPIO_PXSELC(0) = 0x00100000;				\
	REG_GPIO_PXDIRC(0) = 0x00100000;				\
	REG_GPIO_PXPES(0) = 0x00100000;					\
} while (0)

/*
 *  SLCD
 */
#define __gpio_as_slcd_16bit() \
do {						\
	REG_GPIO_PXFUNS(2) = 0x03cff0fc;	\
	REG_GPIO_PXTRGC(2) = 0x03cff0fc;	\
	REG_GPIO_PXSELC(2) = 0x03cff0fc;    \
	REG_GPIO_PXPES(2) = 0x03cff0fc;    \
} while (0)

/*
 * LCD_R3~LCD_R7, LCD_G2~LCD_G7, LCD_B3~LCD_B7,
 * LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 */
#define __gpio_as_lcd_16bit()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x0f8ff3f8;	\
	REG_GPIO_PXTRGC(2) = 0x0f8ff3f8;	\
	REG_GPIO_PXSELC(2) = 0x0f8ff3f8;	\
	REG_GPIO_PXPES(2) = 0x0f8ff3f8;		\
} while (0)

/*
 * LCD_R2~LCD_R7, LCD_G2~LCD_G7, LCD_B2~LCD_B7,
 * LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 */
#define __gpio_as_lcd_18bit()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x0fcff3fc;	\
	REG_GPIO_PXTRGC(2) = 0x0fcff3fc;	\
	REG_GPIO_PXSELC(2) = 0x0fcff3fc;	\
	REG_GPIO_PXPES(2) = 0x0fcff3fc;		\
} while (0)

/*
 * LCD_R0~LCD_R7, LCD_G0~LCD_G7, LCD_B0~LCD_B7,
 * LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 */
#define __gpio_as_lcd_24bit()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x0fffffff;	\
	REG_GPIO_PXTRGC(2) = 0x0fffffff;	\
	REG_GPIO_PXSELC(2) = 0x0fffffff;	\
	REG_GPIO_PXPES(2) = 0x0fffffff;		\
} while (0)

/*
 * LCD_R0~LCD_R7, LCD_G0~LCD_G7, LCD_B0~LCD_B7,
 * LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 */
#define __gpio_clear_lcd_24bit()		\
do {						\
	REG_GPIO_PXFUNC(2) = 0x0fffffff;	\
	REG_GPIO_PXTRGC(2) = 0x0fffffff;	\
	REG_GPIO_PXSELC(2) = 0x0fffffff;	\
	REG_GPIO_PXDIRS(2) = 0x0fffffff;	\
	REG_GPIO_PXDATC(2) = 0x0fffffff;	\
	REG_GPIO_PXPES(2) = 0x0fffffff;		\
} while (0)

/* Set data pin driver strength v: 0~7 */
#define __gpio_set_lcd_data_driving_strength(v) \
do {						\
	unsigned int d;			\
	d = v & 0x1;				\
	if(d) REG_GPIO_PXDS0S(2) = 0x0ff3fcff;	\
	else REG_GPIO_PXDS0C(2) = 0x0ff3fcff;	\
	d = v & 0x2;				\
	if(d) REG_GPIO_PXDS1S(2) = 0x0ff3fcff;	\
	else REG_GPIO_PXDS1C(2) = 0x0ff3fcff;	\
	d = v & 0x4;				\
	if(d) REG_GPIO_PXDS2S(2) = 0x0ff3fcff;	\
	else REG_GPIO_PXDS2C(2) = 0x0ff3fcff;	\
} while(0)
/* Set HSYNC VSYNC DE driver strength v: 0~7 */
#define __gpio_set_lcd_sync_driving_strength(v) \
do {						\
	unsigned int d;				\
	d = v & 0x1;				\
	if(d) REG_GPIO_PXDS0S(2) = 0x000c0200;	\
	else REG_GPIO_PXDS0C(2) = 0x000c0200;	\
	d = v & 0x2;				\
	if(d) REG_GPIO_PXDS1S(2) = 0x000c0200;	\
	else REG_GPIO_PXDS1C(2) = 0x000c0200;	\
	d = v & 0x4;				\
	if(d) REG_GPIO_PXDS2S(2) = 0x000c0200;	\
	else REG_GPIO_PXDS2C(2) = 0x000c0200;	\
} while(0)
/* Set PCLK driver strength v: 0~7 */
#define __gpio_set_lcd_clk_driving_strength(v)	\
do {						\
	unsigned int d;				\
	d = v & 0x1;				\
	if(d) REG_GPIO_PXDS0S(2) = (1 << 8);	\
	else REG_GPIO_PXDS0C(2) = (1 << 8);	\
	d = v & 0x2;				\
	if(d) REG_GPIO_PXDS1S(2) = (1 << 8);	\
	else REG_GPIO_PXDS1C(2) = (1 << 8);	\
	d = v & 0x4;				\
	if(d) REG_GPIO_PXDS2S(2) = (1 << 8);	\
	else REG_GPIO_PXDS2C(2) = (1 << 8);	\
} while(0)

/* Set fast slew rate */
#define __gpio_set_lcd_data_fslew(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLS(p) = 0x0ff3fcff;		\
} while(0)

/* Set slow slew rate */
#define __gpio_set_lcd_data_sslew(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLC(p) = 0x0ff3fcff;		\
} while(0)

/* Set fast slew rate */
#define __gpio_set_lcd_sync_fslew(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLS(p) = 0x000c0200;		\
} while(0)

/* Set slow slew rate */
#define __gpio_set_lcd_sync_sslew(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLC(p) = 0x000c0200;		\
} while(0)

/* Set fast slew rate */
#define __gpio_set_lcd_pclk_fslew(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLS(p) = (1 << 8);		\
} while(0)

/* Set slow slew rate */
#define __gpio_set_lcd_pclk_sslew(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLC(p) = (1 << 8);		\
} while(0)

/*
 *  LCD_CLS, LCD_SPL, LCD_PS, LCD_REV
 */
#define __gpio_as_lcd_special()			\
do {						\
	REG_GPIO_PXFUNS(2) = 0x0fffffff;	\
	REG_GPIO_PXTRGC(2) = 0x0fffffff;	\
	REG_GPIO_PXSELC(2) = 0x0feffbfc;	\
	REG_GPIO_PXSELS(2) = 0x00100403;	\
	REG_GPIO_PXPES(2) = 0x0fffffff;		\
} while (0)

#define __gpio_as_epd()				\
do {						\
	REG_GPIO_PXFUNS(1) = 0x00011e00;	\
	REG_GPIO_PXTRGS(1) = 0x00011e00;	\
	REG_GPIO_PXSELS(1) = 0x00011e00;	\
	REG_GPIO_PXPES(1)  = 0x00011e00;	\
} while (0)
/*
 * CIM_D0~CIM_D7, CIM_MCLK, CIM_PCLK, CIM_VSYNC, CIM_HSYNC
 */
#define __gpio_as_cim()				\
do {						\
	REG_GPIO_PXFUNS(1) = 0x0003ffc0;	\
	REG_GPIO_PXTRGC(1) = 0x0003ffc0;	\
	REG_GPIO_PXSELC(1) = 0x0003ffc0;	\
	REG_GPIO_PXPES(1)  = 0x0003ffc0;	\
} while (0)

/*
 * SDATO, SDATI, BCLK, SYNC, SCLK_RSTN(gpio sepc) or
 * SDATA_OUT, SDATA_IN, BIT_CLK, SYNC, SCLK_RESET(aic spec)
 */
#define __gpio_as_aic()		\
do {					\
	REG_GPIO_PXFUNS(3) = 0x00003000;	\
	REG_GPIO_PXTRGC(3) = 0x00003000;	\
	REG_GPIO_PXSELS(3) = 0x00001000;	\
	REG_GPIO_PXSELC(3) = 0x00002000;	\
	REG_GPIO_PXPES(3)  = 0x00003000;	\
	REG_GPIO_PXFUNS(4) = 0x000000e0;	\
	REG_GPIO_PXTRGS(4) = 0x00000020;	\
	REG_GPIO_PXTRGC(4) = 0x000000c0;	\
	REG_GPIO_PXSELC(4) = 0x000000e0;	\
	REG_GPIO_PXPES(4)  = 0x000000e0;	\
} while (0)

#define __gpio_as_spdif()		\
do {					\
	REG_GPIO_PXFUNS(3) = 0x00003000;	\
	REG_GPIO_PXTRGC(3) = 0x00003000;	\
	REG_GPIO_PXSELS(3) = 0x00001000;	\
	REG_GPIO_PXSELC(3) = 0x00002000;	\
	REG_GPIO_PXPES(3)  = 0x00003000;	\
	REG_GPIO_PXFUNS(4) = 0x000038e0;	\
	REG_GPIO_PXTRGC(4) = 0x000038c0;	\
	REG_GPIO_PXTRGS(4) = 0x00000020;	\
	REG_GPIO_PXSELC(4) = 0x000038e0;	\
	REG_GPIO_PXPES(4)  = 0x000038e0;	\
} while (0)

/*
 * MSC0_CMD, MSC0_CLK, MSC0_D0 ~ MSC0_D3
 */
#define __gpio_as_msc0_pa_4bit()		\
do {						\
	REG_GPIO_PXFUNS(0) = 0x00fc0000;	\
	REG_GPIO_PXTRGC(0) = 0x00fc0000;	\
	REG_GPIO_PXSELS(0) = 0x00ec0000;	\
	REG_GPIO_PXSELC(0) = 0x00100000;	\
	REG_GPIO_PXPES(0)  = 0x00fc0000;	\
} while (0)

/*
 * MSC0_CMD, MSC0_CLK, MSC0_D0 ~ MSC0_D7
 */
#define __gpio_as_msc0_pe_8bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x3ff00000;	\
	REG_GPIO_PXTRGC(4) = 0x3ff00000;	\
	REG_GPIO_PXSELC(4) = 0x3ff00000;	\
	REG_GPIO_PXPES(4)  = 0x3ff00000;	\
	REG_GPIO_PXDS0S(4) = 0x3ff00000;        \
} while (0)
/*
 * MSC0_CMD, MSC0_CLK, MSC0_D0 ~ MSC0_D3
 */
#define __gpio_as_msc0_pe_4bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x30f00000;	\
	REG_GPIO_PXTRGC(4) = 0x30f00000;	\
	REG_GPIO_PXSELC(4) = 0x30f00000;	\
	REG_GPIO_PXPES(4)  = 0x30f00000;	\
	REG_GPIO_PXDS0S(4) = 0x30f00000;        \
} while (0)

#define __gpio_as_msc0_boot()			\
do {        					\
	REG_GPIO_PXFUNS(0) = 0x00ec0000;	\
	REG_GPIO_PXTRGC(0) = 0x00ec0000;	\
	REG_GPIO_PXSELS(0) = 0x00ec0000;	\
	REG_GPIO_PXPES(0)  = 0x00ec0000;	\
	REG_GPIO_PXFUNS(0) = 0x00100000;	\
	REG_GPIO_PXTRGC(0) = 0x00100000;	\
	REG_GPIO_PXSELC(0) = 0x00100000;	\
	REG_GPIO_PXPES(0)  = 0x00100000;	\
						\
} while (0)

/*
 * MSC1_CMD, MSC1_CLK, MSC1_D0 ~ MSC1_D7
 */
#define __gpio_as_msc1_pe_8bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x3ff00000;	\
	REG_GPIO_PXTRGC(4) = 0x3ff00000;	\
	REG_GPIO_PXSELS(4) = 0x3ff00000;	\
	REG_GPIO_PXPES(4)  = 0x3ff00000;	\
	REG_GPIO_PXDS0S(4) = 0x3ff00000;        \
} while (0)
/*
 * MSC1_CMD, MSC1_CLK, MSC1_D0 ~ MSC1_D3
 */
#define __gpio_as_msc1_pe_4bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x30f00000;	\
	REG_GPIO_PXTRGC(4) = 0x30f00000;	\
	REG_GPIO_PXSELS(4) = 0x30f00000;	\
	REG_GPIO_PXPES(4)  = 0x30f00000;	\
	REG_GPIO_PXDS0S(4) = 0x30f00000;        \
} while (0)

/*
 * MSC1_CMD, MSC1_CLK, MSC1_D0 ~ MSC1_D3
 */
#define __gpio_as_msc1_pd_4bit()			\
do {						\
	REG_GPIO_PXFUNS(3) = 0x03f00000;	\
	REG_GPIO_PXTRGC(3) = 0x03f00000;	\
	REG_GPIO_PXSELC(3) = 0x03f00000;	\
	REG_GPIO_PXPES(3)  = 0x03f00000;	\
	REG_GPIO_PXDS0S(3) = 0x03f00000;        \
} while (0)

/* Port B
 * MSC2_CMD, MSC2_CLK, MSC2_D0 ~ MSC2_D3
 */
#define __gpio_as_msc2_pb_4bit()		\
do {						\
	REG_GPIO_PXFUNS(1) = 0xf0300000;	\
	REG_GPIO_PXTRGC(1) = 0xf0300000;	\
	REG_GPIO_PXSELC(1) = 0xf0300000;	\
	REG_GPIO_PXPES(1)  = 0xf0300000;	\
	REG_GPIO_PXDS0S(1) = 0xf0300000;        \
} while (0)

/*
 * MSC2_CMD, MSC2_CLK, MSC2_D0 ~ MSC2_D7
 */
#define __gpio_as_msc2_pe_8bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x3ff00000;	\
	REG_GPIO_PXTRGS(4) = 0x3ff00000;	\
	REG_GPIO_PXSELC(4) = 0x3ff00000;	\
	REG_GPIO_PXPES(4)  = 0x3ff00000;	\
	REG_GPIO_PXDS0S(4) = 0x3ff00000;        \
} while (0)
/*
 * MSC2_CMD, MSC2_CLK, MSC2_D0 ~ MSC2_D3
 */
#define __gpio_as_msc2_pe_4bit()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x30f00000;	\
	REG_GPIO_PXTRGS(4) = 0x30f00000;	\
	REG_GPIO_PXSELC(4) = 0x30f00000;	\
	REG_GPIO_PXPES(4)  = 0x30f00000;	\
	REG_GPIO_PXDS0S(4)= 0x30f00000;		\
} while (0)
#define __gpio_as_msc0_4bit	__gpio_as_msc0_pe_4bit /* default as msc0 4bit */
#define __gpio_as_msc1_4bit	__gpio_as_msc1_pd_4bit /* msc1 only support 4bit */
#define __gpio_as_msc 		__gpio_as_msc0_4bit /* default as msc0 4bit */
#define __gpio_as_msc0 		__gpio_as_msc0_4bit /* msc0 default as 4bit */
#define __gpio_as_msc1 		__gpio_as_msc1_4bit /* msc1 only support 4bit */
#define __gpio_as_msc0_8bit	__gpio_as_msc0_pe_8bit /* default as msc0 8bit */
#define __gpio_as_msc1_8bit 	__gpio_as_msc1_pd_8bit /* msc1 only support 8bit */
/*
 * TSCLK, TSSTR, TSFRM, TSFAIL, TSDI0~7
 */
#define __gpio_as_tssi_1()			\
do {						\
	REG_GPIO_PXFUNS(1) = 0x0003ffc0;	\
	REG_GPIO_PXTRGC(1) = 0x0003ffc0;	\
	REG_GPIO_PXSELS(1) = 0x0003ffc0;	\
	REG_GPIO_PXPES(1)  = 0x0003ffc0;	\
} while (0)

/*
 * TSCLK, TSSTR, TSFRM, TSFAIL, TSDI0~7
 */
#define __gpio_as_tssi_2()			\
do {						\
	REG_GPIO_PXFUNS(1) = 0xfff00000;	\
	REG_GPIO_PXTRGC(1) = 0x0fc00000;	\
	REG_GPIO_PXTRGS(1) = 0xf0300000;	\
	REG_GPIO_PXSELC(1) = 0xfff00000;	\
	REG_GPIO_PXPES(1)  = 0xfff00000;	\
} while (0)

/*
 * SSI_CE0, SSI_CE1, SSI_GPC, SSI_CLK, SSI_DT, SSI_DR
 */
#define __gpio_as_ssi()				\
do {						\
	REG_GPIO_PXFUNS(0) = 0x002c0000; /* SSI0_CE0, SSI0_CLK, SSI0_DT	*/ \
	REG_GPIO_PXTRGS(0) = 0x002c0000;	\
	REG_GPIO_PXSELC(0) = 0x002c0000;	\
	REG_GPIO_PXPES(0)  = 0x002c0000;	\
						\
	REG_GPIO_PXFUNS(0) = 0x00100000; /* SSI0_DR */	\
	REG_GPIO_PXTRGC(0) = 0x00100000;	\
	REG_GPIO_PXSELS(0) = 0x00100000;	\
	REG_GPIO_PXPES(0)  = 0x00100000;	\
} while (0)

/*
 * SSI_CE0, SSI_CE2, SSI_GPC, SSI_CLK, SSI_DT, SSI1_DR
 */
#define __gpio_as_ssi_1()			\
do {						\
	REG_GPIO_PXFUNS(5) = 0x0000fc00;	\
	REG_GPIO_PXTRGC(5) = 0x0000fc00;	\
	REG_GPIO_PXSELC(5) = 0x0000fc00;	\
	REG_GPIO_PXPES(5)  = 0x0000fc00;	\
} while (0)

/* Port B
 * SSI2_CE0, SSI2_CE2, SSI2_GPC, SSI2_CLK, SSI2_DT, SSI12_DR
 */
#define __gpio_as_ssi2_1()			\
do {						\
	REG_GPIO_PXFUNS(5) = 0xf0300000;	\
	REG_GPIO_PXTRGC(5) = 0xf0300000;	\
	REG_GPIO_PXSELS(5) = 0xf0300000;	\
	REG_GPIO_PXPES(5)  = 0xf0300000;	\
} while (0)

#define __gpio_as_pcm0() \
do {						\
	REG_GPIO_PXFUNS(3) = 0xf;	\
	REG_GPIO_PXTRGC(3) = 0xf;	\
	REG_GPIO_PXSELC(3) = 0xf;	\
	REG_GPIO_PXPES(3)  = 0xf;	\
} while (0)

#define __gpio_as_pcm1() \
do {						\
	REG_GPIO_PXFUNS(3) = 0x1000;	\
	REG_GPIO_PXTRGS(3) = 0x1000;	\
	REG_GPIO_PXSELC(3) = 0x1000;	\
	REG_GPIO_PXPES(3)  = 0x1000;	\
	REG_GPIO_PXFUNS(4) = 0x1000;	\
	REG_GPIO_PXTRGC(4) = 0x300;	\
	REG_GPIO_PXTRGS(4) = 0x20;	\
	REG_GPIO_PXSELS(4) = 0x320;	\
	REG_GPIO_PXPES(4)  = 0x320;	\
} while (0)
/*
 * I2C_SCK, I2C_SDA
 */
#define __gpio_as_i2c(n)		       \
do {						\
	REG_GPIO_PXFUNS(3+(n)) = 0xc0000000;	\
	REG_GPIO_PXTRGC(3+(n)) = 0xc0000000;	\
	REG_GPIO_PXSELC(3+(n)) = 0xc0000000;	\
	REG_GPIO_PXPES(3+(n))  = 0xc0000000;	\
} while (0)

/*
 * PWM0
 */
#define __gpio_as_pwm0()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x1;	\
	REG_GPIO_PXTRGC(4) = 0x1;	\
	REG_GPIO_PXSELC(4) = 0x1;	\
	REG_GPIO_PXPES(4) = 0x1;		\
} while (0)

/*
 * PWM1
 */
#define __gpio_as_pwm1()			\
do {						\
	REG_GPIO_PXFUNS(4) = 0x2;	        \
	REG_GPIO_PXTRGC(4) = 0x2;		\
	REG_GPIO_PXSELC(4) = 0x2;		\
	REG_GPIO_PXPEC(4) = 0x2;		\
} while (0)

/*
 * PWM2
 */
#define __gpio_as_pwm2()		\
do {					\
	REG_GPIO_PXFUNS(4) = 0x4;	\
	REG_GPIO_PXTRGC(4) = 0x4;	\
	REG_GPIO_PXSELC(4) = 0x4;	\
	REG_GPIO_PXPES(4) = 0x4;	\
} while (0)

/*
 * PWM3
 */
#define __gpio_as_pwm3()		\
do {					\
	REG_GPIO_PXFUNS(4) = 0x8;	\
	REG_GPIO_PXTRGC(4) = 0x8;	\
	REG_GPIO_PXSELC(4) = 0x8;	\
	REG_GPIO_PXPES(4) = 0x8;	\
} while (0)

/*
 * PWM4
 */
#define __gpio_as_pwm4()		\
do {					\
	REG_GPIO_PXFUNS(4) = 0x10;	\
	REG_GPIO_PXTRGC(4) = 0x10;	\
	REG_GPIO_PXSELC(4) = 0x10;	\
	REG_GPIO_PXPES(4) = 0x10;	\
} while (0)

/*
 * PWM5
 */
#define __gpio_as_pwm5()		\
do {					\
	REG_GPIO_PXFUNS(4) = 0x20;	\
	REG_GPIO_PXTRGC(4) = 0x20;	\
	REG_GPIO_PXSELC(4) = 0x20;	\
	REG_GPIO_PXPES(4) = 0x20;	\
} while (0)

/*
 * n = 0 ~ 5
 */
#define __gpio_as_pwm(n)	__gpio_as_pwm##n()

/*
 * OWI - PA29 function 1
 */
#define __gpio_as_owi()				\
do {						\
	REG_GPIO_PXFUNS(0) = 0x20000000;	\
	REG_GPIO_PXTRGC(0) = 0x20000000;	\
	REG_GPIO_PXSELS(0) = 0x20000000;	\
} while (0)

/*
 * SCC - PD08 function 0
 *       PD09 function 0
 */
#define __gpio_as_scc()				\
do {						\
	REG_GPIO_PXFUNS(3) = 0xc0000300;	\
	REG_GPIO_PXTRGC(3) = 0xc0000300;	\
	REG_GPIO_PXSELC(3) = 0xc0000300;	\
} while (0)

#define __gpio_as_otg_drvvbus()	\
do {	\
	REG_GPIO_PXDATC(4) = (1 << 10);		\
	REG_GPIO_PXPEC(4) = (1 << 10);		\
	REG_GPIO_PXSELC(4) = (1 << 10);		\
	REG_GPIO_PXTRGC(4) = (1 << 10);		\
	REG_GPIO_PXFUNS(4) = (1 << 10);		\
} while (0)

//-------------------------------------------
// GPIO or Interrupt Mode

#define __gpio_get_port(p)	(REG_GPIO_PXPIN(p))

#define __gpio_port_as_output(p, o)		\
do {						\
    REG_GPIO_PXFUNC(p) = (1 << (o));		\
    REG_GPIO_PXSELC(p) = (1 << (o));		\
    REG_GPIO_PXDIRS(p) = (1 << (o));		\
    REG_GPIO_PXPES(p) = (1 << (o));		\
} while (0)

#define __gpio_port_as_input(p, o)		\
do {						\
    REG_GPIO_PXFUNC(p) = (1 << (o));		\
    REG_GPIO_PXSELC(p) = (1 << (o));		\
    REG_GPIO_PXDIRC(p) = (1 << (o));		\
} while (0)

#define __gpio_as_output(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	__gpio_port_as_output(p, o);		\
} while (0)

#define __gpio_as_input(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	__gpio_port_as_input(p, o);		\
} while (0)

#define __gpio_set_pin(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXDATS(p) = (1 << o);		\
} while (0)

#define __gpio_clear_pin(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXDATC(p) = (1 << o);		\
} while (0)

#define __gpio_get_pin(n)			\
({						\
	unsigned int p, o, v;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	if (__gpio_get_port(p) & (1 << o))	\
		v = 1;				\
	else					\
		v = 0;				\
	v;					\
})

#define __gpio_as_irq_high_level(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
	REG_GPIO_PXDIRS(p) = (1 << o);		\
	REG_GPIO_PXFLGC(p) = (1 << o);		\
	REG_GPIO_PXIMC(p) = (1 << o);		\
} while (0)

#define __gpio_as_irq_low_level(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
	REG_GPIO_PXDIRC(p) = (1 << o);		\
	REG_GPIO_PXFLGC(p) = (1 << o);		\
	REG_GPIO_PXIMC(p) = (1 << o);		\
} while (0)

#define __gpio_as_irq_rise_edge(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGS(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
	REG_GPIO_PXDIRS(p) = (1 << o);		\
	REG_GPIO_PXFLGC(p) = (1 << o);		\
	REG_GPIO_PXIMC(p) = (1 << o);		\
} while (0)

#define __gpio_as_irq_fall_edge(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGS(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELS(p) = (1 << o);		\
	REG_GPIO_PXDIRC(p) = (1 << o);		\
	REG_GPIO_PXFLGC(p) = (1 << o);		\
	REG_GPIO_PXIMC(p) = (1 << o);		\
} while (0)

#define __gpio_mask_irq(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
} while (0)

#define __gpio_unmask_irq(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMC(p) = (1 << o);		\
} while (0)

#define __gpio_ack_irq(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXFLGC(p) = (1 << o);		\
} while (0)

#define __gpio_get_irq()			\
({						\
	unsigned int tmp, v = 0;		\
	for (int p = 5; p >= 0; p--) {		\
		tmp = REG_GPIO_PXFLG(p);	\
		for (int i = 0; i < 32; i++)	\
			if (tmp & (1 << i))	\
				v = (32*p + i);	\
	}					\
	v;					\
})

#define __gpio_group_irq(n)			\
({						\
	register int tmp, i;			\
	tmp = REG_GPIO_PXFLG(n) & (~REG_GPIO_PXIM(n));	\
	for (i=31;i>=0;i--)			\
		if (tmp & (1 << i))		\
			break;			\
	i;					\
})

#define __gpio_enable_pull(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXPEC(p) = (1 << o);		\
} while (0)

#define __gpio_disable_pull(n)			\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXPES(p) = (1 << o);		\
} while (0)

/* Set pin driver strength v: 0~7 */
#define __gpio_set_driving_strength(n, v)	\
do {						\
	unsigned int p, o, d;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	d = v & 0x1;				\
	if(d) REG_GPIO_PXDS0S(p) = (1 << o);	\
	else REG_GPIO_PXDS0C(p) = (1 << o);	\
	d = v & 0x2;				\
	if(d) REG_GPIO_PXDS1S(p) = (1 << o);	\
	else REG_GPIO_PXDS1C(p) = (1 << o);	\
	d = v & 0x4;				\
	if(d) REG_GPIO_PXDS2S(p) = (1 << o);	\
	else REG_GPIO_PXDS2C(p) = (1 << o);	\
} while(0)

/* Set fast slew rate */
#define __gpio_set_fslew(n)	\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLS(p) = (1 << o);	\
} while(0)

/* Set slow slew rate */
#define __gpio_set_sslew(n)	\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXSLC(p) = (1 << o);	\
} while(0)

#endif /* __MIPS_ASSEMBLER */

#define DMAC_BASE	0xB3420000

/*************************************************************************
 * DMAC (DMA Controller)
 *************************************************************************/

#define MAX_DMA_NUM 	12  /* max 12 channels */
#define MAX_MDMA_NUM    3   /* max 3  channels */
#define MAX_BDMA_NUM	3   /* max 3  channels */
#define HALF_DMA_NUM	6   /* the number of one dma controller's channels */

/* m is the DMA controller index (0, 1), n is the DMA channel index (0 - 11) */

#define DMAC_DSAR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x00 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA source address */
#define DMAC_DTAR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x04 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA target address */
#define DMAC_DTCR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x08 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA transfer count */
#define DMAC_DRSR(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x0c + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA request source */
#define DMAC_DCCSR(n) (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x10 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA control/status */
#define DMAC_DCMD(n)  (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x14 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA command */
#define DMAC_DDA(n)   (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0x18 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x20)) /* DMA descriptor address */
#define DMAC_DSD(n)   (DMAC_BASE + ((n)/HALF_DMA_NUM*0x100 + 0xc0 + ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM) * 0x04)) /* DMA Stride Address */

#define DMAC_DMACR(m)	(DMAC_BASE + 0x0300 + 0x100 * (m))              /* DMA control register */
#define DMAC_DMAIPR(m)	(DMAC_BASE + 0x0304 + 0x100 * (m))              /* DMA interrupt pending */
#define DMAC_DMADBR(m)	(DMAC_BASE + 0x0308 + 0x100 * (m))              /* DMA doorbell */
#define DMAC_DMADBSR(m)	(DMAC_BASE + 0x030C + 0x100 * (m))              /* DMA doorbell set */
#define DMAC_DMACKE(m)  (DMAC_BASE + 0x0310 + 0x100 * (m))
#define DMAC_DMACKES(m) (DMAC_BASE + 0x0314 + 0x100 * (m))
#define DMAC_DMACKEC(m) (DMAC_BASE + 0x0318 + 0x100 * (m))

#define REG_DMAC_DSAR(n)	REG32(DMAC_DSAR((n)))
#define REG_DMAC_DTAR(n)	REG32(DMAC_DTAR((n)))
#define REG_DMAC_DTCR(n)	REG32(DMAC_DTCR((n)))
#define REG_DMAC_DRSR(n)	REG32(DMAC_DRSR((n)))
#define REG_DMAC_DCCSR(n)	REG32(DMAC_DCCSR((n)))
#define REG_DMAC_DCMD(n)	REG32(DMAC_DCMD((n)))
#define REG_DMAC_DDA(n)		REG32(DMAC_DDA((n)))
#define REG_DMAC_DSD(n)         REG32(DMAC_DSD(n))
#define REG_DMAC_DMACR(m)	REG32(DMAC_DMACR(m))
#define REG_DMAC_DMAIPR(m)	REG32(DMAC_DMAIPR(m))
#define REG_DMAC_DMADBR(m)	REG32(DMAC_DMADBR(m))
#define REG_DMAC_DMADBSR(m)	REG32(DMAC_DMADBSR(m))
#define REG_DMAC_DMACKE(m)      REG32(DMAC_DMACKE(m))
#define REG_DMAC_DMACKES(m)     REG32(DMAC_DMACKES(m))
#define REG_DMAC_DMACKEC(m)     REG32(DMAC_DMACKEC(m))

// DMA request source register
#define DMAC_DRSR_RS_BIT	0
#define DMAC_DRSR_RS_MASK	(0x3f << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_NAND	(1 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_BCH_ENC	(2 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_BCH_DEC	(3 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AUTO	(0x08 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_TSSIIN	(0x09 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_EXT      (0x0c << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART3OUT	(0x0e << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART3IN	(0x0f << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART2OUT	(0x10 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART2IN	(0x11 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART1OUT	(0x12 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART1IN	(0x13 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0OUT	(0x14 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_UART0IN	(0x15 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSI0OUT	(0x16 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSI0IN	(0x17 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICOUT	(0x18 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_AICIN	(0x19 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC0OUT	(0x1a << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC0IN	(0x1b << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_TCU	(0x1c << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SADC	(0x1d << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC1OUT	(0x1e << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC1IN	(0x1f << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSI1OUT	(0x20 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_SSI1IN	(0x21 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_PMOUT	(0x22 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_PMIN	(0x23 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC2OUT	(0x24 << DMAC_DRSR_RS_BIT)
  #define DMAC_DRSR_RS_MSC2IN	(0x25 << DMAC_DRSR_RS_BIT)

// DMA channel control/status register
#define DMAC_DCCSR_NDES		(1 << 31) /* descriptor (0) or not (1) ? */
#define DMAC_DCCSR_DES8    	(1 << 30) /* Descriptor 8 Word */
#define DMAC_DCCSR_DES4    	(0 << 30) /* Descriptor 4 Word */
#define DMAC_DCCSR_CDOA_BIT	16        /* copy of DMA offset address */
#define DMAC_DCCSR_CDOA_MASK	(0xff << DMAC_DCCSR_CDOA_BIT)

#define DMAC_DCCSR_AR		(1 << 4)  /* address error */
#define DMAC_DCCSR_TT		(1 << 3)  /* transfer terminated */
#define DMAC_DCCSR_HLT		(1 << 2)  /* DMA halted */
#define DMAC_DCCSR_CT		(1 << 1)  /* count terminated */
#define DMAC_DCCSR_EN		(1 << 0)  /* channel enable bit */

// DMA channel command register 
#define DMAC_DCMD_EACKS_LOW  	(1 << 31) /* External DACK Output Level Select, active low */
#define DMAC_DCMD_EACKS_HIGH  	(0 << 31) /* External DACK Output Level Select, active high */
#define DMAC_DCMD_EACKM_WRITE 	(1 << 30) /* External DACK Output Mode Select, output in write cycle */
#define DMAC_DCMD_EACKM_READ 	(0 << 30) /* External DACK Output Mode Select, output in read cycle */
#define DMAC_DCMD_ERDM_BIT      28        /* External DREQ Detection Mode Select */
#define DMAC_DCMD_ERDM_MASK     (0x03 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_LOW    (0 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_FALL   (1 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_HIGH   (2 << DMAC_DCMD_ERDM_BIT)
  #define DMAC_DCMD_ERDM_RISE   (3 << DMAC_DCMD_ERDM_BIT)
#define DMAC_DCMD_BLAST		(1 << 25) /* BCH last */
#define DMAC_DCMD_SAI		(1 << 23) /* source address increment */
#define DMAC_DCMD_DAI		(1 << 22) /* dest address increment */
#define DMAC_DCMD_RDIL_BIT	16        /* request detection interval length */
#define DMAC_DCMD_RDIL_MASK	(0x0f << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_IGN	(0 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_2	(0x01 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_4	(0x02 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_8	(0x03 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_12	(0x04 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_16	(0x05 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_20	(0x06 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_24	(0x07 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_28	(0x08 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_32	(0x09 << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_48	(0x0a << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_60	(0x0b << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_64	(0x0c << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_124	(0x0d << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_128	(0x0e << DMAC_DCMD_RDIL_BIT)
  #define DMAC_DCMD_RDIL_200	(0x0f << DMAC_DCMD_RDIL_BIT)
#define DMAC_DCMD_SWDH_BIT	14  /* source port width */
#define DMAC_DCMD_SWDH_MASK	(0x03 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_32	(0 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_8	(1 << DMAC_DCMD_SWDH_BIT)
  #define DMAC_DCMD_SWDH_16	(2 << DMAC_DCMD_SWDH_BIT)
#define DMAC_DCMD_DWDH_BIT	12  /* dest port width */
#define DMAC_DCMD_DWDH_MASK	(0x03 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_32	(0 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_8	(1 << DMAC_DCMD_DWDH_BIT)
  #define DMAC_DCMD_DWDH_16	(2 << DMAC_DCMD_DWDH_BIT)
#define DMAC_DCMD_DS_BIT	8  /* transfer data size of a data unit */
#define DMAC_DCMD_DS_MASK	(0x07 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BIT	(0 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_8BIT	(1 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BIT	(2 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_16BYTE	(3 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_32BYTE	(4 << DMAC_DCMD_DS_BIT)
  #define DMAC_DCMD_DS_64BYTE	(5 << DMAC_DCMD_DS_BIT)

#define DMAC_DCMD_STDE	        (1 << 2)  /* Stride enable */
#define DMAC_DCMD_TIE		(1 << 1)  /* DMA transfer interrupt enable */
#define DMAC_DCMD_LINK		(1 << 0)  /* descriptor link enable */

// DMA descriptor address register
#define DMAC_DDA_BASE_BIT	12  /* descriptor base address */
#define DMAC_DDA_BASE_MASK	(0x0fffff << DMAC_DDA_BASE_BIT)
#define DMAC_DDA_OFFSET_BIT	4   /* descriptor offset address */
#define DMAC_DDA_OFFSET_MASK	(0x0ff << DMAC_DDA_OFFSET_BIT)

// DMA stride address register
#define DMAC_DSD_TSD_BIT        16  /* target stride address */
#define DMAC_DSD_TSD_MASK      	(0xffff << DMAC_DSD_TSD_BIT)
#define DMAC_DSD_SSD_BIT        0  /* source stride address */
#define DMAC_DSD_SSD_MASK      	(0xffff << DMAC_DSD_SSD_BIT)

// DMA control register
#define DMAC_DMACR_FMSC		(1 << 31)  /* MSC Fast DMA mode */
#define DMAC_DMACR_FSSI		(1 << 30)  /* SSI Fast DMA mode */
#define DMAC_DMACR_FTSSI	(1 << 29)  /* TSSI Fast DMA mode */
#define DMAC_DMACR_FUART	(1 << 28)  /* UART Fast DMA mode */
#define DMAC_DMACR_FAIC		(1 << 27)  /* AIC Fast DMA mode */
#define DMAC_DMACR_PR_BIT	8  /* channel priority mode */
#define DMAC_DMACR_PR_MASK	(0x03 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_012345	(0 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_120345	(1 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_230145	(2 << DMAC_DMACR_PR_BIT)
  #define DMAC_DMACR_PR_340125	(3 << DMAC_DMACR_PR_BIT)
#define DMAC_DMACR_HLT		(1 << 3)  /* DMA halt flag */
#define DMAC_DMACR_AR		(1 << 2)  /* address error flag */
#define DMAC_DMACR_DMAE		(1 << 0)  /* DMA enable bit */

// DMA doorbell register
#define DMAC_DMADBR_DB5		(1 << 5)  /* doorbell for channel 5 */
#define DMAC_DMADBR_DB4		(1 << 4)  /* doorbell for channel 4 */
#define DMAC_DMADBR_DB3		(1 << 3)  /* doorbell for channel 3 */
#define DMAC_DMADBR_DB2		(1 << 2)  /* doorbell for channel 2 */
#define DMAC_DMADBR_DB1		(1 << 1)  /* doorbell for channel 1 */
#define DMAC_DMADBR_DB0		(1 << 0)  /* doorbell for channel 0 */

// DMA doorbell set register
#define DMAC_DMADBSR_DBS5	(1 << 5)  /* enable doorbell for channel 5 */
#define DMAC_DMADBSR_DBS4	(1 << 4)  /* enable doorbell for channel 4 */
#define DMAC_DMADBSR_DBS3	(1 << 3)  /* enable doorbell for channel 3 */
#define DMAC_DMADBSR_DBS2	(1 << 2)  /* enable doorbell for channel 2 */
#define DMAC_DMADBSR_DBS1	(1 << 1)  /* enable doorbell for channel 1 */
#define DMAC_DMADBSR_DBS0	(1 << 0)  /* enable doorbell for channel 0 */

// DMA interrupt pending register
#define DMAC_DMAIPR_CIRQ5	(1 << 5)  /* irq pending status for channel 5 */
#define DMAC_DMAIPR_CIRQ4	(1 << 4)  /* irq pending status for channel 4 */
#define DMAC_DMAIPR_CIRQ3	(1 << 3)  /* irq pending status for channel 3 */
#define DMAC_DMAIPR_CIRQ2	(1 << 2)  /* irq pending status for channel 2 */
#define DMAC_DMAIPR_CIRQ1	(1 << 1)  /* irq pending status for channel 1 */
#define DMAC_DMAIPR_CIRQ0	(1 << 0)  /* irq pending status for channel 0 */

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * DMAC
 ***************************************************************************/

/* m is the DMA controller index (0, 1), n is the DMA channel index (0 - 11) */

#define __dmac_enable_module(m) \
	( REG_DMAC_DMACR(m) |= DMAC_DMACR_DMAE | DMAC_DMACR_PR_012345 )
#define __dmac_disable_module(m) \
	( REG_DMAC_DMACR(m) &= ~DMAC_DMACR_DMAE )

/* p=0,1,2,3 */
#define __dmac_set_priority(m,p)			\
do {							\
	REG_DMAC_DMACR(m) &= ~DMAC_DMACR_PR_MASK;	\
	REG_DMAC_DMACR(m) |= ((p) << DMAC_DMACR_PR_BIT);	\
} while (0)

#define __dmac_test_halt_error(m) ( REG_DMAC_DMACR(m) & DMAC_DMACR_HLT )
#define __dmac_test_addr_error(m) ( REG_DMAC_DMACR(m) & DMAC_DMACR_AR )

#define __dmac_channel_enable_clk(n) \
	REG_DMAC_DMACKES((n)/HALF_DMA_NUM) = 1 << ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM);

#define __dmac_channel_disable_clk(n) \
	REG_DMAC_DMACKEC((n)/HALF_DMA_NUM) = 1 << ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM);

#define __dmac_enable_descriptor(n) \
  ( REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_NDES )
#define __dmac_disable_descriptor(n) \
  ( REG_DMAC_DCCSR((n)) |= DMAC_DCCSR_NDES )

#define __dmac_enable_channel(n)                 \
do {                                             \
	REG_DMAC_DCCSR((n)) |= DMAC_DCCSR_EN;    \
} while (0)
#define __dmac_disable_channel(n)                \
do {                                             \
	REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_EN;   \
} while (0)
#define __dmac_channel_enabled(n) \
  ( REG_DMAC_DCCSR((n)) & DMAC_DCCSR_EN )

#define __dmac_channel_enable_irq(n) \
  ( REG_DMAC_DCMD((n)) |= DMAC_DCMD_TIE )
#define __dmac_channel_disable_irq(n) \
  ( REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_TIE )

#define __dmac_channel_transmit_halt_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_HLT )
#define __dmac_channel_transmit_end_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_TT )
#define __dmac_channel_address_error_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_AR )

#define __dmac_channel_count_terminated_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_CT )
#define __dmac_channel_descriptor_invalid_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_INV )

#define __dmac_channel_clear_transmit_halt(n)				\
	do {								\
		/* clear both channel halt error and globle halt error */ \
		REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_HLT;			\
		REG_DMAC_DMACR(n/HALF_DMA_NUM) &= ~DMAC_DMACR_HLT;	\
	} while (0)
#define __dmac_channel_clear_transmit_end(n) \
  (  REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_TT )
#define __dmac_channel_clear_address_error(n)				\
	do {								\
		REG_DMAC_DDA(n) = 0; /* clear descriptor address register */ \
		REG_DMAC_DSAR(n) = 0; /* clear source address register */ \
		REG_DMAC_DTAR(n) = 0; /* clear target address register */ \
		/* clear both channel addr error and globle address error */ \
		REG_DMAC_DCCSR(n) &= ~DMAC_DCCSR_AR;			\
		REG_DMAC_DMACR(n/HALF_DMA_NUM) &= ~DMAC_DMACR_AR;	\
	} while (0)

#define __dmac_channel_clear_count_terminated(n) \
  (  REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_CT )
#define __dmac_channel_clear_descriptor_invalid(n) \
  (  REG_DMAC_DCCSR((n)) &= ~DMAC_DCCSR_INV )

#define __dmac_channel_set_transfer_unit_32bit(n)	\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_DMAC_DCMD((n)) |= MAC_DCMD_DS_32BIT;	\
} while (0)

#define __dmac_channel_set_transfer_unit_16bit(n)	\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_16BIT;	\
} while (0)

#define __dmac_channel_set_transfer_unit_8bit(n)	\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_8BIT;	\
} while (0)

#define __dmac_channel_set_transfer_unit_16byte(n)	\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_16BYTE;	\
} while (0)

#define __dmac_channel_set_transfer_unit_32byte(n)	\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_DS_32BYTE;	\
} while (0)

/* w=8,16,32 */
#define __dmac_channel_set_dest_port_width(n,w)		\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DWDH_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_DWDH_##w;	\
} while (0)

/* w=8,16,32 */
#define __dmac_channel_set_src_port_width(n,w)		\
do {							\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_SWDH_MASK;	\
	REG_DMAC_DCMD((n)) |= DMAC_DCMD_SWDH_##w;	\
} while (0)

/* v=0-15 */
#define __dmac_channel_set_rdil(n,v)				\
do {								\
	REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_RDIL_MASK;		\
	REG_DMAC_DCMD((n) |= ((v) << DMAC_DCMD_RDIL_BIT);	\
} while (0)

#define __dmac_channel_dest_addr_fixed(n) \
  (  REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_DAI )
#define __dmac_channel_dest_addr_increment(n) \
  (  REG_DMAC_DCMD((n)) |= DMAC_DCMD_DAI )

#define __dmac_channel_src_addr_fixed(n) \
  (  REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_SAI )
#define __dmac_channel_src_addr_increment(n) \
  (  REG_DMAC_DCMD((n)) |= DMAC_DCMD_SAI )

#define __dmac_channel_set_doorbell(n)	\
	(  REG_DMAC_DMADBSR((n)/HALF_DMA_NUM) = (1 << ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM)) )

#define __dmac_channel_irq_detected(n)  ( REG_DMAC_DMAIPR((n)/HALF_DMA_NUM) & (1 << ((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM)) )
#define __dmac_channel_ack_irq(n)       ( REG_DMAC_DMAIPR((n)/HALF_DMA_NUM) &= ~(1 <<((n)-(n)/HALF_DMA_NUM*HALF_DMA_NUM)) )

static __inline__ int __dmac_get_irq(void)
{
	int i;
	for (i = 0; i < MAX_DMA_NUM; i++)
		if (__dmac_channel_irq_detected(i))
			return i;
	return -1;
}

#endif /* __MIPS_ASSEMBLER */

/*
 * Interrupt controller module(INTC) address definition
 */
#define	INTC_BASE	0xB0001000

/*
 * INTC registers offset address definition
 */
#define INTC_ICSR_OFFSET	(0x00)	/* 32, r,  0x00000000 */
#define INTC_ICMR_OFFSET	(0x04)	/* 32, rw, 0xffffffff */
#define INTC_ICMSR_OFFSET	(0x08)	/* 32, w,  0x???????? */
#define INTC_ICMCR_OFFSET	(0x0c)	/* 32, w,  0x???????? */
#define INTC_ICPR_OFFSET	(0x10)	/* 32, r,  0x00000000 */

/* INTC groups offset */
#define INTC_GOS		0x20

/*
 * INTC registers address definition
 */
#define INTC_ICSR(n)	(INTC_BASE + (n) * INTC_GOS + INTC_ICSR_OFFSET)
#define INTC_ICMR(n)	(INTC_BASE + (n) * INTC_GOS + INTC_ICMR_OFFSET)
#define INTC_ICMSR(n)	(INTC_BASE + (n) * INTC_GOS + INTC_ICMSR_OFFSET)
#define INTC_ICMCR(n)	(INTC_BASE + (n) * INTC_GOS + INTC_ICMCR_OFFSET)
#define INTC_ICPR(n)	(INTC_BASE + (n) * INTC_GOS + INTC_ICPR_OFFSET)

/*
 * INTC registers common define
 */

/* 1st-level interrupts */
#define IRQ_I2C1	0
#define IRQ_I2C0	1
#define IRQ_UART3	2
#define IRQ_UART2	3
#define IRQ_UART1	4
#define IRQ_UART0	5
#define IRQ_GPU		6
#define IRQ_SSI1	7
#define IRQ_SSI0	8
#define IRQ_TSSI	9
#define IRQ_BDMA	10
#define IRQ_KBC		11
#define IRQ_GPIO5	12
#define IRQ_GPIO4	13
#define IRQ_GPIO3	14
#define IRQ_GPIO2	15
#define IRQ_GPIO1	16
#define IRQ_GPIO0	17
#define IRQ_SADC	18
#define IRQ_ETH		19
#define IRQ_UHC		20
#define IRQ_OTG		21
#define IRQ_MDMA	22
#define IRQ_DMAC1	23
#define IRQ_DMAC0	24
#define IRQ_TCU2	25
#define IRQ_TCU1	26
#define IRQ_TCU0	27
#define IRQ_GPS		28
#define IRQ_IPU		29
#define IRQ_CIM		30
#define IRQ_LCD		31

#define IRQ_RTC		32	/* 32 + 0 */
#define IRQ_OWI		33	/* 32 + 1 */
#define IRQ_AIC 	34	/* 32 + 2 */
#define IRQ_MSC2	35	/* 32 + 3 */
#define IRQ_MSC1	36	/* 32 + 4 */
#define IRQ_MSC0	37	/* 32 + 5 */
#define IRQ_SCC		38	/* 32 + 6 */
#define IRQ_BCH		39	/* 32 + 7 */
#define IRQ_PCM		40	/* 32 + 8 */
#define IRQ_HARB0	41	/* 32 + 9 */
#define IRQ_HARB2	42	/* 32 + 10 */
#define IRQ_AOSD	43	/* 32 + 11 */
#define IRQ_CPM		44	/* 32 + 12 */

#define IRQ_INTC_MAX	45

/* 2nd-level interrupts */
#define IRQ_DMA_BASE	(IRQ_INTC_MAX)
#define IRQ_DMA_MAX	(IRQ_DMA_BASE + NUM_DMA)

#define IRQ_MDMA_BASE	(IRQ_DMA_MAX)
#define IRQ_MDMA_MAX	(IRQ_MDMA_BASE + NUM_MDMA)

#define IRQ_BDMA_BASE	(IRQ_MDMA_MAX)
#define IRQ_BDMA_MAX	(IRQ_BDMA_BASE + NUM_BDMA)

/* To be cleanup begin */
#define IRQ_DMA_0	46
#define IRQ_DMA_1	(IRQ_DMA_0 + HALF_DMA_NUM)	/* 46 +  6 = 52 */
#define IRQ_MDMA_0	(IRQ_DMA_0 + MAX_DMA_NUM)	/* 46 + 12 = 58 */
#define IRQ_BDMA_0	(IRQ_DMA_0 + MAX_DMA_NUM + MAX_MDMA_NUM)	/* 46 + 12 + 2 = 60 */

#define IRQ_GPIO_0		64  /* 64 to (64+MAX_GPIO_NUM-1) for GPIO pin 0 to MAX_GPIO_NUM-1 */

#define NUM_INTC		45
#define NUM_DMA         	MAX_DMA_NUM
#define NUM_MDMA		MAX_MDMA_NUM
#define NUM_BDMA		MAX_BDMA_NUM
#define NUM_GPIO        	MAX_GPIO_NUM
/* To be cleanup end */

#ifndef __MIPS_ASSEMBLER

#define REG_INTC_ICMR(n)	REG32(INTC_ICMR(n))
#define REG_INTC_ICMSR(n)	REG32(INTC_ICMSR(n))
#define REG_INTC_ICMCR(n)	REG32(INTC_ICMCR(n))
#define REG_INTC_ICPR(n)	REG32(INTC_ICPR(n))

#define __intc_unmask_irq(n)	(REG_INTC_ICMCR((n)/32) = (1 << ((n)%32)))
#define __intc_mask_irq(n)	(REG_INTC_ICMSR((n)/32) = (1 << ((n)%32)))

#endif /* __MIPS_ASSEMBLER */

/*
 * AC97 and I2S controller module(AIC) address definition
 */
#define	AIC_BASE	0xb0020000

/*
 * AIC registers offset address definition
 */
#define AIC_FR_OFFSET		(0x00)
#define AIC_CR_OFFSET		(0x04)
#define AIC_ACCR1_OFFSET	(0x08)
#define AIC_ACCR2_OFFSET	(0x0c)
#define AIC_I2SCR_OFFSET	(0x10)
#define AIC_SR_OFFSET		(0x14)
#define AIC_ACSR_OFFSET		(0x18)
#define AIC_I2SSR_OFFSET	(0x1c)
#define AIC_ACCAR_OFFSET	(0x20)
#define AIC_ACCDR_OFFSET	(0x24)
#define AIC_ACSAR_OFFSET	(0x28)
#define AIC_ACSDR_OFFSET	(0x2c)
#define AIC_I2SDIV_OFFSET	(0x30)
#define AIC_DR_OFFSET		(0x34)

#define SPDIF_ENA_OFFSET	(0x80)
#define SPDIF_CTRL_OFFSET	(0x84)
#define SPDIF_STATE_OFFSET	(0x88)
#define SPDIF_CFG1_OFFSET	(0x8c)
#define SPDIF_CFG2_OFFSET	(0x90)
#define SPDIF_FIFO_OFFSET	(0x94)

#define ICDC_RGADW_OFFSET	(0xa4)
#define ICDC_RGDATA_OFFSET	(0xa8)

/*
 * AIC registers address definition
 */
#define AIC_FR		(AIC_BASE + AIC_FR_OFFSET)
#define AIC_CR		(AIC_BASE + AIC_CR_OFFSET)
#define AIC_ACCR1	(AIC_BASE + AIC_ACCR1_OFFSET)
#define AIC_ACCR2	(AIC_BASE + AIC_ACCR2_OFFSET)
#define AIC_I2SCR	(AIC_BASE + AIC_I2SCR_OFFSET)
#define AIC_SR		(AIC_BASE + AIC_SR_OFFSET)
#define AIC_ACSR	(AIC_BASE + AIC_ACSR_OFFSET)
#define AIC_I2SSR	(AIC_BASE + AIC_I2SSR_OFFSET)
#define AIC_ACCAR	(AIC_BASE + AIC_ACCAR_OFFSET)
#define AIC_ACCDR	(AIC_BASE + AIC_ACCDR_OFFSET)
#define AIC_ACSAR	(AIC_BASE + AIC_ACSAR_OFFSET)
#define AIC_ACSDR	(AIC_BASE + AIC_ACSDR_OFFSET)
#define AIC_I2SDIV	(AIC_BASE + AIC_I2SDIV_OFFSET)
#define AIC_DR		(AIC_BASE + AIC_DR_OFFSET)

#define SPDIF_ENA	(AIC_BASE + SPDIF_ENA_OFFSET)
#define SPDIF_CTRL	(AIC_BASE + SPDIF_CTRL_OFFSET)
#define SPDIF_STATE	(AIC_BASE + SPDIF_STATE_OFFSET)
#define SPDIF_CFG1	(AIC_BASE + SPDIF_CFG1_OFFSET)
#define SPDIF_CFG2	(AIC_BASE + SPDIF_CFG2_OFFSET)
#define SPDIF_FIFO	(AIC_BASE + SPDIF_FIFO_OFFSET)

#define ICDC_RGADW	(AIC_BASE + ICDC_RGADW_OFFSET)
#define ICDC_RGDATA	(AIC_BASE + ICDC_RGDATA_OFFSET)

/*
 * AIC registers common define
 */

/* AIC controller configuration register(AICFR) */
#define	AIC_FR_LSMP		BIT6
#define	AIC_FR_ICDC		BIT5
#define	AIC_FR_AUSEL		BIT4
#define	AIC_FR_RST		BIT3
#define	AIC_FR_BCKD		BIT2
#define	AIC_FR_SYNCD		BIT1
#define	AIC_FR_ENB		BIT0

#define	AIC_FR_RFTH_LSB		24
#define	AIC_FR_RFTH_MASK	BITS_H2L(27, AIC_FR_RFTH_LSB)

#define	AIC_FR_TFTH_LSB		16
#define	AIC_FR_TFTH_MASK	BITS_H2L(20, AIC_FR_TFTH_LSB)

/* AIC controller common control register(AICCR) */
#define AIC_CR_PACK16		BIT28
#define	AIC_CR_RDMS		BIT15
#define	AIC_CR_TDMS		BIT14
#define	AIC_CR_M2S		BIT11
#define	AIC_CR_ENDSW		BIT10
#define	AIC_CR_AVSTSU		BIT9
#define	AIC_CR_TFLUSH		BIT8
#define	AIC_CR_RFLUSH		BIT7
#define	AIC_CR_EROR		BIT6
#define	AIC_CR_ETUR		BIT5
#define	AIC_CR_ERFS		BIT4
#define	AIC_CR_ETFS		BIT3
#define	AIC_CR_ENLBF		BIT2
#define	AIC_CR_ERPL		BIT1
#define	AIC_CR_EREC		BIT0

#define AIC_CR_CHANNEL_LSB	24
#define AIC_CR_CHANNEL_MASK	BITS_H2L(26, AIC_CR_CHANNEL_LSB)

#define	AIC_CR_OSS_LSB		19
#define	AIC_CR_OSS_MASK		BITS_H2L(21, AIC_CR_OSS_LSB)
 #define AIC_CR_OSS(n)		(((n) > 18 ? (n)/6 : (n)/9) << AIC_CR_OSS_LSB)	/* n = 8, 16, 18, 20, 24 */

#define	AIC_CR_ISS_LSB		16
#define	AIC_CR_ISS_MASK		BITS_H2L(18, AIC_CR_ISS_LSB)
 #define AIC_CR_ISS(n)		(((n) > 18 ? (n)/6 : (n)/9) << AIC_CR_ISS_LSB)	/* n = 8, 16, 18, 20, 24 */

/* AIC controller AC-link control register 1(ACCR1) */
#define AIC_ACCR1_RS_LSB	16
#define	AIC_ACCR1_RS_MASK	BITS_H2L(25, AIC_ACCR1_RS_LSB)
 #define AIC_ACCR1_RS_SLOT(n)	((1 << ((n) - 3)) << AIC_ACCR1_RS_LSB)	/* n = 3 .. 12 */

#define AIC_ACCR1_XS_LSB	0
#define	AIC_ACCR1_XS_MASK	BITS_H2L(9, AIC_ACCR1_XS_LSB)
 #define AIC_ACCR1_XS_SLOT(n)	((1 << ((n) - 3)) << AIC_ACCR1_XS_LSB)	/* n = 3 .. 12 */

/* AIC controller AC-link control register 2 (ACCR2) */
#define	AIC_ACCR2_ERSTO		BIT18
#define	AIC_ACCR2_ESADR		BIT17
#define	AIC_ACCR2_ECADT		BIT16
#define	AIC_ACCR2_SO		BIT3
#define	AIC_ACCR2_SR		BIT2
#define	AIC_ACCR2_SS		BIT1
#define	AIC_ACCR2_SA		BIT0

/* AIC controller i2s/msb-justified control register (I2SCR) */
#define	AIC_I2SCR_RFIRST	BIT17
#define	AIC_I2SCR_SWLH   	BIT16
#define	AIC_I2SCR_STPBK		BIT12
#define AIC_I2SCR_ESCLK		BIT4
#define	AIC_I2SCR_AMSL		BIT0

/* AIC controller FIFO status register (AICSR) */
#define	AIC_SR_ROR		BIT6
#define	AIC_SR_TUR		BIT5
#define	AIC_SR_RFS		BIT4
#define	AIC_SR_TFS		BIT3

#define	AIC_SR_RFL_LSB		24
#define	AIC_SR_RFL_MASK		BITS_H2L(29, AIC_SR_RFL_LSB)

#define	AIC_SR_TFL_LSB		8
#define	AIC_SR_TFL_MASK		BITS_H2L(13, AIC_SR_TFL_LSB)

/* AIC controller AC-link status register (ACSR) */
#define	AIC_ACSR_SLTERR		BIT21
#define	AIC_ACSR_CRDY		BIT20
#define	AIC_ACSR_CLPM		BIT19
#define	AIC_ACSR_RSTO		BIT18
#define	AIC_ACSR_SADR		BIT17
#define	AIC_ACSR_CADT		BIT16

/* AIC controller I2S/MSB-justified status register (I2SSR) */
#define	AIC_I2SSR_CHBSY		BIT5
#define	AIC_I2SSR_TBSY		BIT4
#define	AIC_I2SSR_RBSY		BIT3
#define	AIC_I2SSR_BSY		BIT2

/* AIC controller AC97 codec command address register (ACCAR) */
#define	AIC_ACCAR_CAR_LSB	0
#define	AIC_ACCAR_CAR_MASK	BITS_H2L(19, AIC_ACCAR_CAR_LSB)

/* AIC controller AC97 codec command data register (ACCDR) */
#define	AIC_ACCDR_CDR_LSB	0
#define	AIC_ACCDR_CDR_MASK	BITS_H2L(19, AIC_ACCDR_CDR_LSB)

/* AC97 read and write macro based on ACCAR and ACCDR */
#define AC97_READ_CMD		BIT19
#define AC97_WRITE_CMD		(BIT19 & ~BIT19)

#define AC97_INDEX_LSB		12
#define AC97_INDEX_MASK		BITS_H2L(18, AC97_INDEX_LSB)

#define AC97_DATA_LSB		4
#define AC97_DATA_MASK		BITS_H2L(19, AC97_DATA_LSB)

/* AIC controller AC97 codec status address register (ACSAR) */
#define	AIC_ACSAR_SAR_LSB	0
#define	AIC_ACSAR_SAR_MASK	BITS_H2L(19, AIC_ACSAR_SAR_LSB)

/* AIC controller AC97 codec status data register (ACSDR) */
#define	AIC_ACSDR_SDR_LSB	0
#define	AIC_ACSDR_SDR_MASK	BITS_H2L(19, AIC_ACSDR_SDR_LSB)

/* AIC controller I2S/MSB-justified clock divider register (I2SDIV) */
#define	AIC_I2SDIV_DIV_LSB	0
#define	AIC_I2SDIV_DIV_MASK	BITS_H2L(3, AIC_I2SDIV_DIV_LSB)

/* SPDIF enable register (SPDIF_ENA) */
#define SPDIF_ENA_SPEN		BIT0

/* SPDIF control register (SPDIF_CTRL) */
#define SPDIF_CTRL_DMAEN        BIT15
#define SPDIF_CTRL_DTYPE        BIT14
#define SPDIF_CTRL_SIGN         BIT13
#define SPDIF_CTRL_INVALID      BIT12
#define SPDIF_CTRL_RST          BIT11
#define SPDIF_CTRL_SPDIFI2S     BIT10
#define SPDIF_CTRL_MTRIG        BIT1
#define SPDIF_CTRL_MFFUR        BIT0

/* SPDIF state register (SPDIF_STAT) */
#define SPDIF_STAT_BUSY		BIT7
#define SPDIF_STAT_FTRIG	BIT1
#define SPDIF_STAT_FUR		BIT0

#define SPDIF_STAT_FLVL_LSB	8
#define SPDIF_STAT_FLVL_MASK	BITS_H2L(14, SPDIF_STAT_FLVL_LSB)

/* SPDIF configure 1 register (SPDIF_CFG1) */
#define SPDIF_CFG1_INITLVL	BIT17
#define SPDIF_CFG1_ZROVLD	BIT16

#define SPDIF_CFG1_TRIG_LSB	12
#define SPDIF_CFG1_TRIG_MASK	BITS_H2L(13, SPDIF_CFG1_TRIG_LSB)
#define SPDIF_CFG1_TRIG(n)	(((n) > 16 ? 3 : (n)/8) << SPDIF_CFG1_TRIG_LSB)	/* n = 4, 8, 16, 32 */

#define SPDIF_CFG1_SRCNUM_LSB	8
#define SPDIF_CFG1_SRCNUM_MASK	BITS_H2L(11, SPDIF_CFG1_SRCNUM_LSB)

#define SPDIF_CFG1_CH1NUM_LSB	4
#define SPDIF_CFG1_CH1NUM_MASK	BITS_H2L(7, SPDIF_CFG1_CH1NUM_LSB)

#define SPDIF_CFG1_CH2NUM_LSB	0
#define SPDIF_CFG1_CH2NUM_MASK	BITS_H2L(3, SPDIF_CFG1_CH2NUM_LSB)

/* SPDIF configure 2 register (SPDIF_CFG2) */
#define SPDIF_CFG2_MAXWL	BIT18
#define SPDIF_CFG2_PRE		BIT3
#define SPDIF_CFG2_COPYN	BIT2
#define SPDIF_CFG2_AUDION	BIT1
#define SPDIF_CFG2_CONPRO	BIT0

#define SPDIF_CFG2_FS_LSB	26
#define SPDIF_CFG2_FS_MASK	BITS_H2L(29, SPDIF_CFG2_FS_LSB)

#define SPDIF_CFG2_ORGFRQ_LSB	22
#define SPDIF_CFG2_ORGFRQ_MASK	BITS_H2L(25, SPDIF_CFG2_ORGFRQ_LSB)

#define SPDIF_CFG2_SAMWL_LSB	19
#define SPDIF_CFG2_SAMWL_MASK	BITS_H2L(21, SPDIF_CFG2_SAMWL_LSB)

#define SPDIF_CFG2_CLKACU_LSB	16
#define SPDIF_CFG2_CLKACU_MASK	BITS_H2L(17, SPDIF_CFG2_CLKACU_LSB)

#define SPDIF_CFG2_CATCODE_LSB	8
#define SPDIF_CFG2_CATCODE_MASK	BITS_H2L(15, SPDIF_CFG2_CATCODE_LSB)

#define SPDIF_CFG2_CHMD_LSB	6
#define SPDIF_CFG2_CHMD_MASK	BITS_H2L(7, SPDIF_CFG2_CHMD_LSB)

/* ICDC internal register access control register(RGADW) */
#define ICDC_RGADW_RGWR		BIT16

#define ICDC_RGADW_RGADDR_LSB	8
#define	ICDC_RGADW_RGADDR_MASK	BITS_H2L(14, ICDC_RGADW_RGADDR_LSB)

#define ICDC_RGADW_RGDIN_LSB	0
#define	ICDC_RGADW_RGDIN_MASK	BITS_H2L(7, ICDC_RGADW_RGDIN_LSB)

/* ICDC internal register data output register (RGDATA)*/
#define ICDC_RGDATA_IRQ		BIT8

#define ICDC_RGDATA_RGDOUT_LSB	0
#define ICDC_RGDATA_RGDOUT_MASK	BITS_H2L(7, ICDC_RGDATA_RGDOUT_LSB)

#ifndef __MIPS_ASSEMBLER

#define	REG_AIC_FR		REG32(AIC_FR)
#define	REG_AIC_CR		REG32(AIC_CR)
#define	REG_AIC_ACCR1		REG32(AIC_ACCR1)
#define	REG_AIC_ACCR2		REG32(AIC_ACCR2)
#define	REG_AIC_I2SCR		REG32(AIC_I2SCR)
#define	REG_AIC_SR		REG32(AIC_SR)
#define	REG_AIC_ACSR		REG32(AIC_ACSR)
#define	REG_AIC_I2SSR		REG32(AIC_I2SSR)
#define	REG_AIC_ACCAR		REG32(AIC_ACCAR)
#define	REG_AIC_ACCDR		REG32(AIC_ACCDR)
#define	REG_AIC_ACSAR		REG32(AIC_ACSAR)
#define	REG_AIC_ACSDR		REG32(AIC_ACSDR)
#define	REG_AIC_I2SDIV		REG32(AIC_I2SDIV)
#define	REG_AIC_DR		REG32(AIC_DR)

#define REG_SPDIF_ENA		REG32(SPDIF_ENA)
#define REG_SPDIF_CTRL		REG32(SPDIF_CTRL)
#define REG_SPDIF_STATE		REG32(SPDIF_STATE)
#define REG_SPDIF_CFG1		REG32(SPDIF_CFG1)
#define REG_SPDIF_CFG2		REG32(SPDIF_CFG2)
#define REG_SPDIF_FIFO		REG32(SPDIF_FIFO)

#define	REG_ICDC_RGADW		REG32(ICDC_RGADW)
#define	REG_ICDC_RGDATA		REG32(ICDC_RGDATA)

#define __aic_enable()		( REG_AIC_FR |= AIC_FR_ENB )
#define __aic_disable()		( REG_AIC_FR &= ~AIC_FR_ENB )

#define __aic_select_ac97()	( REG_AIC_FR &= ~AIC_FR_AUSEL )
#define __aic_select_i2s()	( REG_AIC_FR |= AIC_FR_AUSEL )

#define __aic_play_zero()	( REG_AIC_FR &= ~AIC_FR_LSMP )
#define __aic_play_lastsample()	( REG_AIC_FR |= AIC_FR_LSMP )

#define __i2s_as_master()	( REG_AIC_FR |= AIC_FR_BCKD | AIC_FR_SYNCD )
#define __i2s_as_slave()	( REG_AIC_FR &= ~(AIC_FR_BCKD | AIC_FR_SYNCD) )
#define __aic_reset_status()          ( REG_AIC_FR & AIC_FR_RST )

#define __i2s_enable_sclk()	( REG_AIC_I2SCR |= AIC_I2SCR_ESCLK )
#define __i2s_disable_sclk()	( REG_AIC_I2SCR &= ~AIC_I2SCR_ESCLK )

#define __aic_reset()                                   \
do {                                                    \
        REG_AIC_FR |= AIC_FR_RST;                       \
} while(0)

#define __aic_set_transmit_trigger(n) 			\
do {							\
	REG_AIC_FR &= ~AIC_FR_TFTH_MASK;		\
	REG_AIC_FR |= ((n) << AIC_FR_TFTH_LSB);		\
} while(0)

#define __aic_set_receive_trigger(n) 			\
do {							\
	REG_AIC_FR &= ~AIC_FR_RFTH_MASK;		\
	REG_AIC_FR |= ((n) << AIC_FR_RFTH_LSB);		\
} while(0)

#define __aic_enable_oldstyle()
#define __aic_enable_newstyle()
#define __aic_enable_pack16()   ( REG_AIC_CR |= AIC_CR_PACK16 )
#define __aic_enable_unpack16() ( REG_AIC_CR &= ~AIC_CR_PACK16)

/* n = AIC_CR_CHANNEL_MONO,AIC_CR_CHANNEL_STEREO ... */
#define __aic_out_channel_select(n)                    \
do {                                                   \
	REG_AIC_CR &= ~AIC_CR_CHANNEL_MASK;            \
        REG_AIC_CR |= ((n) << AIC_CR_CHANNEL_LSB );			       \
} while(0)

#define __aic_enable_record()	( REG_AIC_CR |= AIC_CR_EREC )
#define __aic_disable_record()	( REG_AIC_CR &= ~AIC_CR_EREC )
#define __aic_enable_replay()	( REG_AIC_CR |= AIC_CR_ERPL )
#define __aic_disable_replay()	( REG_AIC_CR &= ~AIC_CR_ERPL )
#define __aic_enable_loopback()	( REG_AIC_CR |= AIC_CR_ENLBF )
#define __aic_disable_loopback() ( REG_AIC_CR &= ~AIC_CR_ENLBF )

#define __aic_flush_tfifo()	( REG_AIC_CR |= AIC_CR_TFLUSH )
#define __aic_unflush_tfifo()	( REG_AIC_CR &= ~AIC_CR_TFLUSH )
#define __aic_flush_rfifo()	( REG_AIC_CR |= AIC_CR_RFLUSH )
#define __aic_unflush_rfifo()	( REG_AIC_CR &= ~AIC_CR_RFLUSH )

#define __aic_enable_transmit_intr() \
  ( REG_AIC_CR |= (AIC_CR_ETFS | AIC_CR_ETUR) )
#define __aic_disable_transmit_intr() \
  ( REG_AIC_CR &= ~(AIC_CR_ETFS | AIC_CR_ETUR) )
#define __aic_enable_receive_intr() \
  ( REG_AIC_CR |= (AIC_CR_ERFS | AIC_CR_EROR) )
#define __aic_disable_receive_intr() \
  ( REG_AIC_CR &= ~(AIC_CR_ERFS | AIC_CR_EROR) )

#define __aic_enable_transmit_dma()  ( REG_AIC_CR |= AIC_CR_TDMS )
#define __aic_disable_transmit_dma() ( REG_AIC_CR &= ~AIC_CR_TDMS )
#define __aic_enable_receive_dma()   ( REG_AIC_CR |= AIC_CR_RDMS )
#define __aic_disable_receive_dma()  ( REG_AIC_CR &= ~AIC_CR_RDMS )

#define __aic_enable_mono2stereo()   ( REG_AIC_CR |= AIC_CR_M2S )
#define __aic_disable_mono2stereo()  ( REG_AIC_CR &= ~AIC_CR_M2S )
#define __aic_enable_byteswap()      ( REG_AIC_CR |= AIC_CR_ENDSW )
#define __aic_disable_byteswap()     ( REG_AIC_CR &= ~AIC_CR_ENDSW )
#define __aic_enable_unsignadj()     ( REG_AIC_CR |= AIC_CR_AVSTSU )
#define __aic_disable_unsignadj()    ( REG_AIC_CR &= ~AIC_CR_AVSTSU )

#define AC97_PCM_XS_L_FRONT   	AIC_ACCR1_XS_SLOT(3)
#define AC97_PCM_XS_R_FRONT   	AIC_ACCR1_XS_SLOT(4)
#define AC97_PCM_XS_CENTER    	AIC_ACCR1_XS_SLOT(6)
#define AC97_PCM_XS_L_SURR    	AIC_ACCR1_XS_SLOT(7)
#define AC97_PCM_XS_R_SURR    	AIC_ACCR1_XS_SLOT(8)
#define AC97_PCM_XS_LFE       	AIC_ACCR1_XS_SLOT(9)

#define AC97_PCM_RS_L_FRONT   	AIC_ACCR1_RS_SLOT(3)
#define AC97_PCM_RS_R_FRONT   	AIC_ACCR1_RS_SLOT(4)
#define AC97_PCM_RS_CENTER    	AIC_ACCR1_RS_SLOT(6)
#define AC97_PCM_RS_L_SURR    	AIC_ACCR1_RS_SLOT(7)
#define AC97_PCM_RS_R_SURR    	AIC_ACCR1_RS_SLOT(8)
#define AC97_PCM_RS_LFE       	AIC_ACCR1_RS_SLOT(9)

#define __ac97_set_xs_none()	( REG_AIC_ACCR1 &= ~AIC_ACCR1_XS_MASK )
#define __ac97_set_xs_mono() 						\
do {									\
	REG_AIC_ACCR1 &= ~AIC_ACCR1_XS_MASK;				\
	REG_AIC_ACCR1 |= AC97_PCM_XS_R_FRONT;				\
} while(0)
#define __ac97_set_xs_stereo() 						\
do {									\
	REG_AIC_ACCR1 &= ~AIC_ACCR1_XS_MASK;				\
	REG_AIC_ACCR1 |= AC97_PCM_XS_L_FRONT | AC97_PCM_XS_R_FRONT;	\
} while(0)

/* In fact, only stereo is support now. */
#define __ac97_set_rs_none()	( REG_AIC_ACCR1 &= ~AIC_ACCR1_RS_MASK )
#define __ac97_set_rs_mono() 						\
do {									\
	REG_AIC_ACCR1 &= ~AIC_ACCR1_RS_MASK;				\
	REG_AIC_ACCR1 |= AC97_PCM_RS_R_FRONT;				\
} while(0)
#define __ac97_set_rs_stereo() 						\
do {									\
	REG_AIC_ACCR1 &= ~AIC_ACCR1_RS_MASK;				\
	REG_AIC_ACCR1 |= AC97_PCM_RS_L_FRONT | AC97_PCM_RS_R_FRONT;	\
} while(0)

#define __ac97_warm_reset_codec()		\
 do {						\
	REG_AIC_ACCR2 |= AIC_ACCR2_SA;		\
	REG_AIC_ACCR2 |= AIC_ACCR2_SS;		\
	udelay(2);				\
	REG_AIC_ACCR2 &= ~AIC_ACCR2_SS;		\
	REG_AIC_ACCR2 &= ~AIC_ACCR2_SA;		\
 } while (0)

#define __ac97_cold_reset_codec()		\
 do {						\
	REG_AIC_ACCR2 |=  AIC_ACCR2_SR;		\
	udelay(2);				\
	REG_AIC_ACCR2 &= ~AIC_ACCR2_SR;		\
 } while (0)

/* n=8,16,18,20 */
#define __ac97_set_iass(n) \
 ( REG_AIC_ACCR2 = (REG_AIC_ACCR2 & ~AIC_ACCR2_IASS_MASK) | AIC_ACCR2_IASS_##n##BIT )
#define __ac97_set_oass(n) \
 ( REG_AIC_ACCR2 = (REG_AIC_ACCR2 & ~AIC_ACCR2_OASS_MASK) | AIC_ACCR2_OASS_##n##BIT )

/* This bit should only be set in 2 channels configuration */
#define __i2s_send_rfirst()   ( REG_AIC_I2SCR |= AIC_I2SCR_RFIRST )  /* RL */
#define __i2s_send_lfirst()   ( REG_AIC_I2SCR &= ~AIC_I2SCR_RFIRST ) /* LR */

/* This bit should only be set in 2 channels configuration and 16bit-packed mode */
#define __i2s_switch_lr()     ( REG_AIC_I2SCR |= AIC_I2SCR_SWLH )
#define __i2s_unswitch_lr()   ( REG_AIC_I2SCR &= ~AIC_I2SCR_SWLH )

#define __i2s_select_i2s()            ( REG_AIC_I2SCR &= ~AIC_I2SCR_AMSL )
#define __i2s_select_msbjustified()   ( REG_AIC_I2SCR |= AIC_I2SCR_AMSL )

/* n=8,16,18,20,24 */
/*#define __i2s_set_sample_size(n) \
 ( REG_AIC_I2SCR |= (REG_AIC_I2SCR & ~AIC_I2SCR_WL_MASK) | AIC_I2SCR_WL_##n##BIT )*/

#define __i2s_out_channel_select(n) __aic_out_channel_select(n)

#define __i2s_set_oss_sample_size(n) \
 ( REG_AIC_CR = (REG_AIC_CR & ~AIC_CR_OSS_MASK) | AIC_CR_OSS(n))
#define __i2s_set_iss_sample_size(n) \
 ( REG_AIC_CR = (REG_AIC_CR & ~AIC_CR_ISS_MASK) | AIC_CR_ISS(n))

#define __i2s_stop_bitclk()   ( REG_AIC_I2SCR |= AIC_I2SCR_STPBK )
#define __i2s_start_bitclk()  ( REG_AIC_I2SCR &= ~AIC_I2SCR_STPBK )

#define __aic_transmit_request()  ( REG_AIC_SR & AIC_SR_TFS )
#define __aic_receive_request()   ( REG_AIC_SR & AIC_SR_RFS )
#define __aic_transmit_underrun() ( REG_AIC_SR & AIC_SR_TUR )
#define __aic_receive_overrun()   ( REG_AIC_SR & AIC_SR_ROR )

#define __aic_clear_errors()      ( REG_AIC_SR &= ~(AIC_SR_TUR | AIC_SR_ROR) )

#define __aic_get_transmit_resident() \
  ( (REG_AIC_SR & AIC_SR_TFL_MASK) >> AIC_SR_TFL_LSB )
#define __aic_get_receive_count() \
  ( (REG_AIC_SR & AIC_SR_RFL_MASK) >> AIC_SR_RFL_LSB )

#define __ac97_command_transmitted()     ( REG_AIC_ACSR & AIC_ACSR_CADT )
#define __ac97_status_received()         ( REG_AIC_ACSR & AIC_ACSR_SADR )
#define __ac97_status_receive_timeout()  ( REG_AIC_ACSR & AIC_ACSR_RSTO )
#define __ac97_codec_is_low_power_mode() ( REG_AIC_ACSR & AIC_ACSR_CLPM )
#define __ac97_codec_is_ready()          ( REG_AIC_ACSR & AIC_ACSR_CRDY )
#define __ac97_slot_error_detected()     ( REG_AIC_ACSR & AIC_ACSR_SLTERR )
#define __ac97_clear_slot_error()        ( REG_AIC_ACSR &= ~AIC_ACSR_SLTERR )

#define __i2s_is_busy()         ( REG_AIC_I2SSR & AIC_I2SSR_BSY )

#define __ac97_out_rcmd_addr(reg) 					\
do { 									\
    REG_AIC_ACCAR = AC97_READ_CMD | ((reg) << AC97_INDEX_LSB); 	\
} while (0)

#define __ac97_out_wcmd_addr(reg) 					\
do { 									\
    REG_AIC_ACCAR = AC97_WRITE_CMD | ((reg) << AC97_INDEX_LSB); 	\
} while (0)

#define __ac97_out_data(value) 						\
do { 									\
    REG_AIC_ACCDR = ((value) << AC97_DATA_LSB); 			\
} while (0)

#define __ac97_in_data() \
 ( (REG_AIC_ACSDR & CODEC_REG_DATA_MASK) >> AC97_DATA_LSB )

#define __ac97_in_status_addr() \
 ( (REG_AIC_ACSAR & AC97_INDEX_MASK) >> AC97_INDEX_LSB )

#define __i2s_set_sample_rate(i2sclk, sync) \
  ( REG_AIC_I2SDIV = ((i2sclk) / (4*64)) / (sync) )

#define __i2s_set_i2sdiv(n) \
  ( REG_AIC_I2SDIV = (n) )

#define __aic_write_tfifo(v)  ( REG_AIC_DR = (v) )
#define __aic_read_rfifo()    ( REG_AIC_DR )

#define __aic_internal_codec()  ( REG_AIC_FR |= AIC_FR_ICDC )
#define __aic_external_codec()  ( REG_AIC_FR &= ~AIC_FR_ICDC )

//
// Define next ops for AC97 compatible
//

#define AC97_ACSR	AIC_ACSR

#define __ac97_enable()		__aic_enable(); __aic_select_ac97()
#define __ac97_disable()	__aic_disable()
#define __ac97_reset()		__aic_reset()

#define __ac97_set_transmit_trigger(n)	__aic_set_transmit_trigger(n)
#define __ac97_set_receive_trigger(n)	__aic_set_receive_trigger(n)

#define __ac97_enable_record()		__aic_enable_record()
#define __ac97_disable_record()		__aic_disable_record()
#define __ac97_enable_replay()		__aic_enable_replay()
#define __ac97_disable_replay()		__aic_disable_replay()
#define __ac97_enable_loopback()	__aic_enable_loopback()
#define __ac97_disable_loopback()	__aic_disable_loopback()

#define __ac97_enable_transmit_dma()	__aic_enable_transmit_dma()
#define __ac97_disable_transmit_dma()	__aic_disable_transmit_dma()
#define __ac97_enable_receive_dma()	__aic_enable_receive_dma()
#define __ac97_disable_receive_dma()	__aic_disable_receive_dma()

#define __ac97_transmit_request()	__aic_transmit_request()
#define __ac97_receive_request()	__aic_receive_request()
#define __ac97_transmit_underrun()	__aic_transmit_underrun()
#define __ac97_receive_overrun()	__aic_receive_overrun()

#define __ac97_clear_errors()		__aic_clear_errors()

#define __ac97_get_transmit_resident()	__aic_get_transmit_resident()
#define __ac97_get_receive_count()	__aic_get_receive_count()

#define __ac97_enable_transmit_intr()	__aic_enable_transmit_intr()
#define __ac97_disable_transmit_intr()	__aic_disable_transmit_intr()
#define __ac97_enable_receive_intr()	__aic_enable_receive_intr()
#define __ac97_disable_receive_intr()	__aic_disable_receive_intr()

#define __ac97_write_tfifo(v)		__aic_write_tfifo(v)
#define __ac97_read_rfifo()		__aic_read_rfifo()

//
// Define next ops for I2S compatible
//

#define I2S_ACSR	AIC_I2SSR

#define __i2s_enable()		 __aic_enable(); __aic_select_i2s()
#define __i2s_disable()		__aic_disable()
#define __i2s_reset()		__aic_reset()

#define __i2s_set_transmit_trigger(n)	__aic_set_transmit_trigger(n)
#define __i2s_set_receive_trigger(n)	__aic_set_receive_trigger(n)

#define __i2s_enable_record()		__aic_enable_record()
#define __i2s_disable_record()		__aic_disable_record()
#define __i2s_enable_replay()		__aic_enable_replay()
#define __i2s_disable_replay()		__aic_disable_replay()
#define __i2s_enable_loopback()		__aic_enable_loopback()
#define __i2s_disable_loopback()	__aic_disable_loopback()

#define __i2s_enable_transmit_dma()	__aic_enable_transmit_dma()
#define __i2s_disable_transmit_dma()	__aic_disable_transmit_dma()
#define __i2s_enable_receive_dma()	__aic_enable_receive_dma()
#define __i2s_disable_receive_dma()	__aic_disable_receive_dma()

#define __i2s_transmit_request()	__aic_transmit_request()
#define __i2s_receive_request()		__aic_receive_request()
#define __i2s_transmit_underrun()	__aic_transmit_underrun()
#define __i2s_receive_overrun()		__aic_receive_overrun()

#define __i2s_clear_errors()		__aic_clear_errors()

#define __i2s_get_transmit_resident()	__aic_get_transmit_resident()
#define __i2s_get_receive_count()	__aic_get_receive_count()

#define __i2s_enable_transmit_intr()	__aic_enable_transmit_intr()
#define __i2s_disable_transmit_intr()	__aic_disable_transmit_intr()
#define __i2s_enable_receive_intr()	__aic_enable_receive_intr()
#define __i2s_disable_receive_intr()	__aic_disable_receive_intr()

#define __i2s_write_tfifo(v)		__aic_write_tfifo(v)
#define __i2s_read_rfifo()		__aic_read_rfifo()

#define __i2s_enable_pack16()		__aic_enable_pack16()
#define __i2s_enable_unpack16()		__aic_enable_unpack16()

#define __i2s_reset_codec()			\
 do {						\
 } while (0)

/*************************************************************************
 * SPDIF INTERFACE in AIC Controller
 *************************************************************************/

#define __spdif_enable()        ( REG_SPDIF_ENA |= SPDIF_ENA_SPEN )
#define __spdif_disable()       ( REG_SPDIF_ENA &= ~SPDIF_ENA_SPEN )

#define __spdif_enable_transmit_dma()     ( REG_SPDIF_CTRL |= SPDIF_CTRL_DMAEN )
#define __spdif_disable_transmit_dma()    ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_DMAEN )
#define __spdif_enable_dtype()            ( REG_SPDIF_CTRL |= SPDIF_CTRL_DTYPE )
#define __spdif_disable_dtype()           ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_DTYPE )
#define __spdif_enable_sign()             ( REG_SPDIF_CTRL |= SPDIF_CTRL_SIGN )
#define __spdif_disable_sign()            ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_SIGN )
#define __spdif_enable_invalid()          ( REG_SPDIF_CTRL |= SPDIF_CTRL_INVALID )
#define __spdif_disable_invalid()         ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_INVALID )
#define __spdif_enable_reset()            ( REG_SPDIF_CTRL |= SPDIF_CTRL_RST )
#define __spdif_select_spdif()            ( REG_SPDIF_CTRL |= SPDIF_CTRL_SPDIFI2S )
#define __spdif_select_i2s()              ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_SPDIFI2S )
#define __spdif_enable_MTRIGmask()        ( REG_SPDIF_CTRL |= SPDIF_CTRL_MTRIG )
#define __spdif_disable_MTRIGmask()       ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_MTRIG )
#define __spdif_enable_MFFURmask()        ( REG_SPDIF_CTRL |= SPDIF_CTRL_MFFUR )
#define __spdif_disable_MFFURmask()       ( REG_SPDIF_CTRL &= ~SPDIF_CTRL_MFFUR )

#define __spdif_enable_initlvl_high()     ( REG_SPDIF_CFG1 |=  SPDIF_CFG1_INITLVL )
#define __spdif_enable_initlvl_low()      ( REG_SPDIF_CFG1 &=  ~SPDIF_CFG1_INITLVL )
#define __spdif_enable_zrovld_invald()    ( REG_SPDIF_CFG1 |=  SPDIF_CFG1_ZROVLD )
#define __spdif_enable_zrovld_vald()      ( REG_SPDIF_CFG1 &=  ~SPDIF_CFG1_ZROVLD )

/* 0, 1, 2, 3 */
#define __spdif_set_transmit_trigger(n)			\
do {							\
	REG_SPDIF_CFG1 &= ~SPDIF_CFG1_TRIG_MASK;	\
	REG_SPDIF_CFG1 |= SPDIF_CFG1_TRIG(n);	\
} while(0)

/* 1 ~ 15 */
#define __spdif_set_srcnum(n)    			\
do {							\
	REG_SPDIF_CFG1 &= ~SPDIF_CFG1_SRCNUM_MASK;	\
	REG_SPDIF_CFG1 |= ((n) << SPDIF_CFG1_SRCNUM_LSB);	\
} while(0)

/* 1 ~ 15 */
#define __spdif_set_ch1num(n)    			\
do {							\
	REG_SPDIF_CFG1 &= ~SPDIF_CFG1_CH1NUM_MASK;	\
	REG_SPDIF_CFG1 |= ((n) << SPDIF_CFG1_CH1NUM_LSB);	\
} while(0)

/* 1 ~ 15 */
#define __spdif_set_ch2num(n)    			\
do {							\
	REG_SPDIF_CFG1 &= ~SPDIF_CFG1_CH2NUM_MASK;	\
	REG_SPDIF_CFG1 |= ((n) << SPDIF_CFG1_CH2NUM_LSB);	\
} while(0)

/* 0x0, 0x2, 0x3, 0xa, 0xe */
#define __spdif_set_fs(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_FS_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_FS_LSB);	\
} while(0)

/* 0xd, 0xc, 0x5, 0x1 */
#define __spdif_set_orgfrq(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_ORGFRQ_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_ORGFRQ_LSB);	\
} while(0)

/* 0x1, 0x6, 0x2, 0x4, 0x5 */
#define __spdif_set_samwl(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_SAMWL_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_SAMWL_LSB);	\
} while(0)

#define __spdif_enable_samwl_24()    ( REG_SPDIF_CFG2 |= SPDIF_CFG2_MAXWL )
#define __spdif_enable_samwl_20()    ( REG_SPDIF_CFG1 &= ~SPDIF_CFG2_MAXWL )

/* 0x1, 0x1, 0x2, 0x3 */
#define __spdif_set_clkacu(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_CLKACU_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_CLKACU_LSB);	\
} while(0)

/* see IEC60958-3 */
#define __spdif_set_catcode(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_CATCODE_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_CATCODE_LSB);	\
} while(0)

/* n = 0x0, */
#define __spdif_set_chmode(n)				\
do {							\
	REG_SPDIF_CFG2 &= ~SPDIF_CFG2_CHMD_MASK;   	\
	REG_SPDIF_CFG2 |= ((n) << SPDIF_CFG2_CHMD_LSB);	\
} while(0)

#define __spdif_enable_pre()       ( REG_SPDIF_CFG2 |= SPDIF_CFG2_PRE )
#define __spdif_disable_pre()      ( REG_SPDIF_CFG2 &= ~SPDIF_CFG2_PRE )
#define __spdif_enable_copyn()     ( REG_SPDIF_CFG2 |= SPDIF_CFG2_COPYN )
#define __spdif_disable_copyn()    ( REG_SPDIF_CFG2 &= ~SPDIF_CFG2_COPYN )
/* audio sample word represents linear PCM samples */
#define __spdif_enable_audion()    ( REG_SPDIF_CFG2 &= ~SPDIF_CFG2_AUDION )
/* udio sample word used for other purpose */
#define __spdif_disable_audion()   ( REG_SPDIF_CFG2 |= SPDIF_CFG2_AUDION )
#define __spdif_enable_conpro()    ( REG_SPDIF_CFG2 &= ~SPDIF_CFG2_CONPRO )
#define __spdif_disable_conpro()   ( REG_SPDIF_CFG2 |= SPDIF_CFG2_CONPRO )

/***************************************************************************
 * ICDC
 ***************************************************************************/
#define __i2s_internal_codec()         __aic_internal_codec()
#define __i2s_external_codec()         __aic_external_codec()

#define __icdc_clk_ready()             ( REG_ICDC_CKCFG & ICDC_CKCFG_CKRDY )
#define __icdc_sel_adc()               ( REG_ICDC_CKCFG |= ICDC_CKCFG_SELAD )
#define __icdc_sel_dac()               ( REG_ICDC_CKCFG &= ~ICDC_CKCFG_SELAD )

#define __icdc_set_rgwr()              ( REG_ICDC_RGADW |= ICDC_RGADW_RGWR )
#define __icdc_clear_rgwr()            ( REG_ICDC_RGADW &= ~ICDC_RGADW_RGWR )
#define __icdc_rgwr_ready()            ( REG_ICDC_RGADW & ICDC_RGADW_RGWR )

#define __icdc_set_addr(n)				\
do {          						\
	REG_ICDC_RGADW &= ~ICDC_RGADW_RGADDR_MASK;	\
	REG_ICDC_RGADW |= (n) << ICDC_RGADW_RGADDR_LSB;	\
} while(0)

#define __icdc_set_cmd(n)				\
do {          						\
	REG_ICDC_RGADW &= ~ICDC_RGADW_RGDIN_MASK;	\
	REG_ICDC_RGADW |= (n) << ICDC_RGADW_RGDIN_LSB;	\
} while(0)

#define __icdc_irq_pending()            ( REG_ICDC_RGDATA & ICDC_RGDATA_IRQ )
#define __icdc_get_value()              ( REG_ICDC_RGDATA & ICDC_RGDATA_RGDOUT_MASK )

#endif /* __MIPS_ASSEMBLER */

/*
 * Bose-Chaudhuri-Hocquenghem controller module(BCH) address definition
 */
#define BCH_BASE	0xb34d0000

/*
 * BCH registers offset addresses definition
 */
#define	BCH_BHCR_OFFSET		(0x00)	/*  r, 32, 0x00000000 */
#define	BCH_BHCSR_OFFSET	(0x04)	/*  w, 32, 0x???????? */
#define	BCH_BHCCR_OFFSET	(0x08)	/*  w, 32, 0x???????? */
#define	BCH_BHCNT_OFFSET	(0x0c)	/* rw, 32, 0x00000000 */
#define	BCH_BHDR_OFFSET		(0x10)	/*  w,  8, 0x???????? */
#define	BCH_BHPAR0_OFFSET	(0x14)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR1_OFFSET	(0x18)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR2_OFFSET	(0x1c)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR3_OFFSET	(0x20)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR4_OFFSET	(0x24)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR5_OFFSET	(0x28)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR6_OFFSET	(0x2c)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR7_OFFSET	(0x30)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR8_OFFSET	(0x34)	/* rw, 32, 0x00000000 */
#define	BCH_BHPAR9_OFFSET	(0x38)	/* rw, 32, 0x00000000 */
#define	BCH_BHERR0_OFFSET	(0x3c)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR1_OFFSET	(0x40)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR2_OFFSET	(0x44)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR3_OFFSET	(0x48)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR4_OFFSET	(0x4c)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR5_OFFSET	(0x50)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR6_OFFSET	(0x54)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR7_OFFSET	(0x58)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR8_OFFSET	(0x5c)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR9_OFFSET	(0x60)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR10_OFFSET	(0x64)	/*  r, 32, 0x00000000 */
#define	BCH_BHERR11_OFFSET	(0x68)	/*  r, 32, 0x00000000 */
#define	BCH_BHINT_OFFSET	(0x6c)	/*  r, 32, 0x00000000 */
#define	BCH_BHINTE_OFFSET	(0x70)	/* rw, 32, 0x00000000 */
#define	BCH_BHINTES_OFFSET	(0x74)	/*  w, 32, 0x???????? */
#define	BCH_BHINTEC_OFFSET	(0x78)	/*  w, 32, 0x???????? */

/*
 * BCH registers addresses definition
 */
#define	BCH_BHCR        (BCH_BASE + BCH_BHCR_OFFSET)
#define	BCH_BHCSR       (BCH_BASE + BCH_BHCSR_OFFSET)
#define	BCH_BHCCR       (BCH_BASE + BCH_BHCCR_OFFSET)
#define	BCH_BHCNT    	(BCH_BASE + BCH_BHCNT_OFFSET)
#define	BCH_BHDR     	(BCH_BASE + BCH_BHDR_OFFSET)
#define	BCH_BHPAR0    	(BCH_BASE + BCH_BHPAR0_OFFSET)
#define	BCH_BHPAR1    	(BCH_BASE + BCH_BHPAR1_OFFSET)
#define	BCH_BHPAR2    	(BCH_BASE + BCH_BHPAR2_OFFSET)
#define	BCH_BHPAR3    	(BCH_BASE + BCH_BHPAR3_OFFSET)
#define	BCH_BHPAR4    	(BCH_BASE + BCH_BHPAR4_OFFSET)
#define	BCH_BHPAR5    	(BCH_BASE + BCH_BHPAR5_OFFSET)
#define	BCH_BHPAR6    	(BCH_BASE + BCH_BHPAR6_OFFSET)
#define	BCH_BHPAR7    	(BCH_BASE + BCH_BHPAR7_OFFSET)
#define	BCH_BHPAR8    	(BCH_BASE + BCH_BHPAR8_OFFSET)
#define	BCH_BHPAR9    	(BCH_BASE + BCH_BHPAR9_OFFSET)
#define	BCH_BHERR0      (BCH_BASE + BCH_BHERR0_OFFSET)
#define	BCH_BHERR1      (BCH_BASE + BCH_BHERR1_OFFSET)
#define	BCH_BHERR2      (BCH_BASE + BCH_BHERR2_OFFSET)
#define	BCH_BHERR3      (BCH_BASE + BCH_BHERR3_OFFSET)
#define	BCH_BHERR4      (BCH_BASE + BCH_BHERR4_OFFSET)
#define	BCH_BHERR5      (BCH_BASE + BCH_BHERR5_OFFSET)
#define	BCH_BHERR6      (BCH_BASE + BCH_BHERR6_OFFSET)
#define	BCH_BHERR7      (BCH_BASE + BCH_BHERR7_OFFSET)
#define	BCH_BHERR8      (BCH_BASE + BCH_BHERR8_OFFSET)
#define	BCH_BHERR9      (BCH_BASE + BCH_BHERR9_OFFSET)
#define	BCH_BHERR10     (BCH_BASE + BCH_BHERR10_OFFSET)
#define	BCH_BHERR11     (BCH_BASE + BCH_BHERR11_OFFSET)
#define	BCH_BHINT    	(BCH_BASE + BCH_BHINT_OFFSET)
#define	BCH_BHINTES     (BCH_BASE + BCH_BHINTES_OFFSET)
#define	BCH_BHINTEC     (BCH_BASE + BCH_BHINTEC_OFFSET)
#define	BCH_BHINTE	(BCH_BASE + BCH_BHINTE_OFFSET)

/*
 * BCH registers common define
 */

/* BCH control register (BHCR) */
#define	BHCR_DMAE		BIT7  /* BCH DMA enable */
#define BHCR_ENCE		BIT2
#define	BHCR_BRST		BIT1  /* BCH reset */
#define	BHCR_BCHE		BIT0  /* BCH enable */

#define	BHCR_BSEL_LSB		3
#define	BHCR_BSEL_MASK		BITS_H2L(5, BHCR_BSEL_LSB)
 #define BHCR_BSEL(n)		(((n)/4 - 1) << BHCR_BSEL_LSB)	/* n = 4, 8, 12, 16, 20, 24 */

/* BCH interrupt status register (BHINT) */
#define BHINT_ALL_F		BIT4
#define	BHINT_DECF		BIT3
#define	BHINT_ENCF		BIT2
#define	BHINT_UNCOR		BIT1
#define	BHINT_ERR		BIT0

#define	BHINT_ERRC_LSB		27
#define	BHINT_ERRC_MASK		BITS_H2L(31, BHINT_ERRC_LSB)

/* BCH ENC/DEC count register (BHCNT) */
#define BHCNT_DEC_LSB		16
#define BHCNT_DEC_MASK		BITS_H2L(26, BHCNT_DEC_LSB)

#define BHCNT_ENC_LSB		0
#define BHCNT_ENC_MASK		BITS_H2L(10, BHCNT_ENC_LSB)

/* BCH error report register (BCHERR)*/
#define BCH_ERR_INDEX_LSB	0
#define BCH_ERR_INDEX_MASK	BITS_H2L(12, BCH_ERR_INDEX_LSB)

/* BCH common macro define */
#define BCH_ENCODE		1
#define BCH_DECODE		0

#ifndef __MIPS_ASSEMBLER

#define	REG_BCH_BHCR		REG32(BCH_BHCR)
#define	REG_BCH_BHCSR		REG32(BCH_BHCSR)
#define	REG_BCH_BHCCR		REG32(BCH_BHCCR)
#define	REG_BCH_BHCNT		REG32(BCH_BHCNT)
#define	REG_BCH_BHDR		REG8(BCH_BHDR)
#define	REG_BCH_BHPAR0		REG32(BCH_BHPAR0)
#define	REG_BCH_BHPAR1		REG32(BCH_BHPAR1)
#define	REG_BCH_BHPAR2		REG32(BCH_BHPAR2)
#define	REG_BCH_BHPAR3		REG32(BCH_BHPAR3)
#define	REG_BCH_BHPAR4		REG32(BCH_BHPAR4)
#define	REG_BCH_BHPAR5		REG32(BCH_BHPAR5)
#define	REG_BCH_BHPAR6		REG32(BCH_BHPAR6)
#define	REG_BCH_BHPAR7		REG32(BCH_BHPAR7)
#define	REG_BCH_BHPAR8		REG32(BCH_BHPAR8)
#define	REG_BCH_BHPAR9		REG32(BCH_BHPAR9)
#define	REG_BCH_BHERR0		REG32(BCH_BHERR0)
#define	REG_BCH_BHERR1		REG32(BCH_BHERR1)
#define	REG_BCH_BHERR2		REG32(BCH_BHERR2)
#define	REG_BCH_BHERR3		REG32(BCH_BHERR3)
#define	REG_BCH_BHERR4		REG32(BCH_BHERR4)
#define	REG_BCH_BHERR5		REG32(BCH_BHERR5)
#define	REG_BCH_BHERR6		REG32(BCH_BHERR6)
#define	REG_BCH_BHERR7		REG32(BCH_BHERR7)
#define	REG_BCH_BHERR8		REG32(BCH_BHERR8)
#define	REG_BCH_BHERR9		REG32(BCH_BHERR9)
#define	REG_BCH_BHERR10		REG32(BCH_BHERR10)
#define	REG_BCH_BHERR11		REG32(BCH_BHERR11)
#define	REG_BCH_BHINT		REG32(BCH_BHINT)
#define	REG_BCH_BHINTE		REG32(BCH_BHINTE)
#define	REG_BCH_BHINTEC		REG32(BCH_BHINTEC)
#define	REG_BCH_BHINTES		REG32(BCH_BHINTES)

#define __ecc_enable(encode, bit)			\
do {							\
	unsigned int tmp = BHCR_BRST | BHCR_BCHE;	\
	if (encode)					\
		tmp |= BHCR_ENCE;			\
	tmp |= BHCR_BSEL(bit);				\
	REG_BCH_BHCSR = tmp;				\
	REG_BCH_BHCCR = ~tmp;				\
} while (0)
#define __ecc_disable()		(REG_BCH_BHCCR = BHCR_BCHE)

#define __ecc_dma_enable()	(REG_BCH_BHCSR = BHCR_DMAE)
#define __ecc_dma_disable()	(REG_BCH_BHCCR = BHCR_DMAE)

#define __ecc_cnt_enc(n)	CMSREG32(BCH_BHCNT, (n) << BHCNT_ENC_LSB, BHCNT_ENC_MASK)
#define __ecc_cnt_dec(n)	CMSREG32(BCH_BHCNT, (n) << BHCNT_DEC_LSB, BHCNT_DEC_MASK)

#define __ecc_encode_sync()				\
do {							\
	unsigned int i = 1;				\
	while (!(REG_BCH_BHINT & BHINT_ENCF) && i++);	\
} while (0)

#define __ecc_decode_sync()				\
do {							\
	unsigned int i = 1;				\
	while (!(REG_BCH_BHINT & BHINT_DECF) && i++);	\
} while (0)

#endif /* __MIPS_ASSEMBLER */

#define BDMAC_BASE  0xB3450000

/*************************************************************************
 * BDMAC (BCH & NAND DMA Controller)
 *************************************************************************/

/* n is the DMA channel index (0 - 2) */
#define BDMAC_DSAR(n)		(BDMAC_BASE + (0x00 + (n) * 0x20)) /* DMA source address */
#define BDMAC_DTAR(n)  		(BDMAC_BASE + (0x04 + (n) * 0x20)) /* DMA target address */
#define BDMAC_DTCR(n)  		(BDMAC_BASE + (0x08 + (n) * 0x20)) /* DMA transfer count */
#define BDMAC_DRSR(n)  		(BDMAC_BASE + (0x0c + (n) * 0x20)) /* DMA request source */
#define BDMAC_DCCSR(n) 		(BDMAC_BASE + (0x10 + (n) * 0x20)) /* DMA control/status */
#define BDMAC_DCMD(n)  		(BDMAC_BASE + (0x14 + (n) * 0x20)) /* DMA command */
#define BDMAC_DDA(n)   		(BDMAC_BASE + (0x18 + (n) * 0x20)) /* DMA descriptor address */
#define BDMAC_DSD(n)   		(BDMAC_BASE + (0x1c + (n) * 0x20)) /* DMA Stride Address */
#define BDMAC_DNT(n)  		(BDMAC_BASE + (0xc0 + (n) * 0x04)) /* NAND Detect Timer */

#define BDMAC_DMACR		(BDMAC_BASE + 0x0300) 	/* DMA control register */
#define BDMAC_DMAIPR		(BDMAC_BASE + 0x0304) 	/* DMA interrupt pending */
#define BDMAC_DMADBR		(BDMAC_BASE + 0x0308) 	/* DMA doorbell */
#define BDMAC_DMADBSR		(BDMAC_BASE + 0x030C) 	/* DMA doorbell set */
#define BDMAC_DMACKE  		(BDMAC_BASE + 0x0310)
#define BDMAC_DMACKES  		(BDMAC_BASE + 0x0314)
#define BDMAC_DMACKEC  		(BDMAC_BASE + 0x0318)

#define REG_BDMAC_DSAR(n)	REG32(BDMAC_DSAR((n)))
#define REG_BDMAC_DTAR(n)	REG32(BDMAC_DTAR((n)))
#define REG_BDMAC_DTCR(n)	REG32(BDMAC_DTCR((n)))
#define REG_BDMAC_DRSR(n)	REG32(BDMAC_DRSR((n)))
#define REG_BDMAC_DCCSR(n)	REG32(BDMAC_DCCSR((n)))
#define REG_BDMAC_DCMD(n)	REG32(BDMAC_DCMD((n)))
#define REG_BDMAC_DDA(n)	REG32(BDMAC_DDA((n)))
#define REG_BDMAC_DSD(n)    REG32(BDMAC_DSD(n))
#define REG_BDMAC_DNT(n)	REG32(BDMAC_DNT(n))

#define REG_BDMAC_DMACR		REG32(BDMAC_DMACR)
#define REG_BDMAC_DMAIPR	REG32(BDMAC_DMAIPR)
#define REG_BDMAC_DMADBR	REG32(BDMAC_DMADBR)
#define REG_BDMAC_DMADBSR	REG32(BDMAC_DMADBSR)
#define REG_BDMAC_DMACKE    	REG32(BDMAC_DMACKE)
#define REG_BDMAC_DMACKES	REG32(BDMAC_DMACKES)
#define REG_BDMAC_DMACKEC	REG32(BDMAC_DMACKEC)

// BDMA request source register
#define BDMAC_DRSR_RS_BIT	0
  #define BDMAC_DRSR_RS_MASK	(0x3f << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_BCH_ENC	(2 << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_BCH_DEC	(3 << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_NAND0	(6 << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_NAND1	(7 << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_NAND 	(BDMAC_DRSR_RS_NAND0)
  #define BDMAC_DRSR_RS_AUTO	(8 << DMAC_DRSR_RS_BIT)
  #define BDMAC_DRSR_RS_EXT	(12 << DMAC_DRSR_RS_BIT)

// BDMA channel control/status register
#define BDMAC_DCCSR_NDES	(1 << 31) /* descriptor (0) or not (1) ? */
#define BDMAC_DCCSR_DES8    	(1 << 30) /* Descriptor 8 Word */
#define BDMAC_DCCSR_DES4    	(0 << 30) /* Descriptor 4 Word */
#define BDMAC_DCCSR_LASTMD_BIT	28
#define BDMAC_DCCSR_LASTMD_MASK	(0x3 << BDMAC_DCCSR_LASTMD_BIT)
#define BDMAC_DCCSR_LASTMD0    	(0 << 28) /* BCH Decoding last mode 0, there's one descriptor for decoding blcok*/
#define BDMAC_DCCSR_LASTMD1    	(1 << 28) /* BCH Decoding last mode 1, there's two descriptor for decoding blcok*/
#define BDMAC_DCCSR_LASTMD2    	(2 << 28) /* BCH Decoding last mode 2, there's three descriptor for decoding blcok*/
#define BDMAC_DCCSR_FRBS(n)	((n) << 24)
#define BDMAC_DCCSR_FRBS_BIT	24
#define BDMAC_DCCSR_FRBS_MASK	(0x7 << BDMAC_DCCSR_FRBS_BIT)
#define BDMAC_DCCSR_CDOA_BIT	16        /* copy of DMA offset address */
#define BDMAC_DCCSR_CDOA_MASK	(0xff << BDMACC_DCCSR_CDOA_BIT)
#define BDMAC_DCCSR_BERR_BIT	7
#define BDMAC_DCCSR_BERR_MASK	(0x1f << BDMAC_DCCSR_BERR_BIT)
#define BDMAC_DCCSR_BUERR	(1 << 5)
#define BDMAC_DCCSR_AR		(1 << 4)  /* address error */
#define BDMAC_DCCSR_TT		(1 << 3)  /* transfer terminated */
#define BDMAC_DCCSR_HLT		(1 << 2)  /* DMA halted */
#define BDMAC_DCCSR_BAC		(1 << 1)
#define BDMAC_DCCSR_EN		(1 << 0)  /* channel enable bit */

// BDMA channel command register
#define BDMAC_DCMD_EACKS_LOW  	(1 << 31) /* External DACK Output Level Select, active low */
#define BDMAC_DCMD_EACKS_HIGH  	(0 << 31) /* External DACK Output Level Select, active high */
#define BDMAC_DCMD_EACKM_WRITE 	(1 << 30) /* External DACK Output Mode Select, output in write cycle */
#define BDMAC_DCMD_EACKM_READ 	(0 << 30) /* External DACK Output Mode Select, output in read cycle */
#define BDMAC_DCMD_ERDM_BIT	28        /* External DREQ Detection Mode Select */
  #define BDMAC_DCMD_ERDM_MASK	(0x03 << BDMAC_DCMD_ERDM_BIT)
  #define BDMAC_DCMD_ERDM_LOW	(0 << BDMAC_DCMD_ERDM_BIT)
  #define BDMAC_DCMD_ERDM_FALL	(1 << BDMAC_DCMD_ERDM_BIT)
  #define BDMAC_DCMD_ERDM_HIGH	(2 << BDMAC_DCMD_ERDM_BIT)
  #define BDMAC_DCMD_ERDM_RISE	(3 << BDMAC_DCMD_ERDM_BIT)
#define BDMAC_DCMD_BLAST	(1 << 25) /* BCH last */
#define BDMAC_DCMD_SAI		(1 << 23) /* source address increment */
#define BDMAC_DCMD_DAI		(1 << 22) /* dest address increment */
#define BDMAC_DCMD_SWDH_BIT	14  /* source port width */
  #define BDMAC_DCMD_SWDH_MASK	(0x03 << BDMAC_DCMD_SWDH_BIT)
  #define BDMAC_DCMD_SWDH_32	(0 << BDMAC_DCMD_SWDH_BIT)
  #define BDMAC_DCMD_SWDH_8	(1 << BDMAC_DCMD_SWDH_BIT)
  #define BDMAC_DCMD_SWDH_16	(2 << BDMAC_DCMD_SWDH_BIT)
#define BDMAC_DCMD_DWDH_BIT	12  /* dest port width */
  #define BDMAC_DCMD_DWDH_MASK	(0x03 << BDMAC_DCMD_DWDH_BIT)
  #define BDMAC_DCMD_DWDH_32	(0 << BDMAC_DCMD_DWDH_BIT)
  #define BDMAC_DCMD_DWDH_8	(1 << BDMAC_DCMD_DWDH_BIT)
  #define BDMAC_DCMD_DWDH_16	(2 << BDMAC_DCMD_DWDH_BIT)
#define BDMAC_DCMD_DS_BIT	8  /* transfer data size of a data unit */
  #define BDMAC_DCMD_DS_MASK	(0x07 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_32BIT	(0 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_8BIT	(1 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_16BIT	(2 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_16BYTE	(3 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_32BYTE	(4 << BDMAC_DCMD_DS_BIT)
  #define BDMAC_DCMD_DS_64BYTE	(5 << BDMAC_DCMD_DS_BIT)
#define BDMAC_DCMD_NRD   	(1 << 7)  /* NAND direct read */
#define BDMAC_DCMD_NWR   	(1 << 6)  /* NAND direct write */
#define BDMAC_DCMD_NAC   	(1 << 5)  /* NAND AL/CL enable */
#define BDMAC_DCMD_NSTA   	(1 << 4)
#define BDMAC_DCMD_STDE   	(1 << 2)  /* Stride Disable/Enable */
#define BDMAC_DCMD_TIE		(1 << 1)  /* DMA transfer interrupt enable */
#define BDMAC_DCMD_LINK		(1 << 0)  /* descriptor link enable */

// BDMA descriptor address register
#define BDMAC_DDA_BASE_BIT	12  /* descriptor base address */
  #define BDMAC_DDA_BASE_MASK	(0x0fffff << BDMAC_DDA_BASE_BIT)
#define BDMAC_DDA_OFFSET_BIT	4   /* descriptor offset address */
  #define BDMAC_DDA_OFFSET_MASK	(0x0ff << BDMAC_DDA_OFFSET_BIT)

// BDMA stride address register
#define BDMAC_DSD_TSD_BIT	16	/* target stride address */
  #define BDMAC_DSD_TSD_MASK	(0xffff << BDMAC_DSD_TSD_BIT)
#define BDMAC_DSD_SSD_BIT	0	/* source stride address */
  #define BDMAC_DSD_SSD_MASK	(0xffff << BDMAC_DSD_SSD_BIT)

// BDMA NAND Detect timer register
#define BDMAC_NDTCTIMER_EN	(1 << 15)  /* enable detect timer */
#define BDMAC_TAILCNT_BIT	16

// BDMA control register
#define BDMAC_DMACR_PR_BIT	8	/* channel priority mode */
  #define BDMAC_DMACR_PR_MASK	(0x03 << DMAC_DMACR_PR_BIT)
  #define BDMAC_DMACR_PR_01_2	(0 << BDMAC_DMACR_PR_BIT)
  #define BDMAC_DMACR_PR_12_0	(1 << BDMAC_DMACR_PR_BIT)
  #define BDMAC_DMACR_PR_20_1	(2 << BDMAC_DMACR_PR_BIT)
  #define BDMAC_DMACR_PR_012	(3 << BDMAC_DMACR_PR_BIT)
#define BDMAC_DMACR_HLT		(1 << 3)  /* DMA halt flag */
#define BDMAC_DMACR_AR		(1 << 2)  /* address error flag */
#define BDMAC_DMACR_DMAE	(1 << 0)  /* DMA enable bit */

// BDMA interrupt pending register
#define BDMAC_DMAIPR_CIRQ2	(1 << 2)  /* irq pending status for channel 2 */
#define BDMAC_DMAIPR_CIRQ1	(1 << 1)  /* irq pending status for channel 1 */
#define BDMAC_DMAIPR_CIRQ0	(1 << 0)  /* irq pending status for channel 0 */

// BDMA doorbell register
#define BDMAC_DMADBR_DB2	(1 << 2)  /* doorbell for channel 2 */
#define BDMAC_DMADBR_DB1	(1 << 1)  /* doorbell for channel 1 */
#define BDMAC_DMADBR_DB0	(1 << 0)  /* doorbell for channel 0 */

// BDMA doorbell set register
#define BDMAC_DMADBSR_DBS2	(1 << 2)  /* enable doorbell for channel 2 */
#define BDMAC_DMADBSR_DBS1	(1 << 1)  /* enable doorbell for channel 1 */
#define BDMAC_DMADBSR_DBS0	(1 << 0)  /* enable doorbell for channel 0 */

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * BCH & NAND DMAC
 ***************************************************************************/

#define __bdmac_enable_module() \
	( REG_BDMAC_DMACR |= BDMAC_DMACR_DMAE )
#define __bdmac_disable_module() \
	( REG_BDMAC_DMACR &= ~BDMAC_DMACR_DMAE )

/* n is the DMA channel index (0 - 2) */

#define __bdmac_test_halt_error ( REG_BDMAC_DMACR & BDMAC_DMACR_HLT )
#define __bdmac_test_addr_error ( REG_BDMAC_DMACR & BDMAC_DMACR_AR )

#define __bdmac_channel_enable_clk(n)           \
	REG_BDMAC_DMACKES = 1 << (n);

#define __bdmac_enable_descriptor(n) \
  ( REG_BDMAC_DCCSR((n)) &= ~BDMAC_DCCSR_NDES )
#define __bdmac_disable_descriptor(n) \
  ( REG_BDMAC_DCCSR((n)) |= BDMAC_DCCSR_NDES )

#define __bdmac_enable_channel(n)                 \
do {                                             \
	REG_BDMAC_DCCSR((n)) |= BDMAC_DCCSR_EN;    \
} while (0)
#define __bdmac_disable_channel(n)                \
do {                                             \
	REG_BDMAC_DCCSR((n)) &= ~BDMAC_DCCSR_EN;   \
} while (0)

#define __bdmac_channel_enable_irq(n) \
  ( REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_TIE )
#define __bdmac_channel_disable_irq(n) \
  ( REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_TIE )

#define __bdmac_channel_transmit_halt_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_HLT )
#define __bdmac_channel_transmit_end_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_TT )
/* Nand ops status error, only for channel 1 */
#define __bdmac_channel_status_error_detected() \
  (  REG_BDMAC_DCCSR(1) & BDMAC_DCCSR_NSERR )
#define __bdmac_channel_address_error_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_AR )
#define __bdmac_channel_count_terminated_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_CT )
#define __bdmac_channel_descriptor_invalid_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_INV )
#define __bdmac_BCH_error_detected(n) \
  (  REG_BDMAC_DCCSR((n)) & BDMAC_DCCSR_BERR )

#define __bdmac_channel_clear_transmit_halt(n)				\
	do {								\
		/* clear both channel halt error and globle halt error */ \
		REG_BDMAC_DCCSR(n) &= ~BDMAC_DCCSR_HLT;			\
		REG_BDMAC_DMACR &= ~BDMAC_DMACR_HLT;	\
	} while (0)
#define __bdmac_channel_clear_transmit_end(n) \
  (  REG_BDMAC_DCCSR(n) &= ~BDMAC_DCCSR_TT )
/* Nand ops status error, only for channel 1 */
#define __bdmac_channel_clear_status_error() \
  ( REG_BDMAC_DCCSR(1) &= ~BDMAC_DCCSR_NSERR )
#define __bdmac_channel_clear_address_error(n)				\
	do {								\
		REG_BDMAC_DDA(n) = 0; /* clear descriptor address register */ \
		REG_BDMAC_DSAR(n) = 0; /* clear source address register */ \
		REG_BDMAC_DTAR(n) = 0; /* clear target address register */ \
		/* clear both channel addr error and globle address error */ \
		REG_BDMAC_DCCSR(n) &= ~BDMAC_DCCSR_AR;			\
		REG_BDMAC_DMACR &= ~BDMAC_DMACR_AR;	\
	} while (0)
#define __bdmac_channel_clear_count_terminated(n) \
  (  REG_BDMAC_DCCSR((n)) &= ~BDMAC_DCCSR_CT )
#define __bdmac_channel_clear_descriptor_invalid(n) \
  (  REG_BDMAC_DCCSR((n)) &= ~BDMAC_DCCSR_INV )

#define __bdmac_channel_set_transfer_unit_32bit(n)	\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DS_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DS_32BIT;	\
} while (0)

#define __bdmac_channel_set_transfer_unit_16bit(n)	\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DS_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DS_16BIT;	\
} while (0)

#define __bdmac_channel_set_transfer_unit_8bit(n)	\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DS_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DS_8BIT;	\
} while (0)

#define __bdmac_channel_set_transfer_unit_16byte(n)	\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DS_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DS_16BYTE;	\
} while (0)

#define __bdmac_channel_set_transfer_unit_32byte(n)	\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DS_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DS_32BYTE;	\
} while (0)

/* w=8,16,32 */
#define __bdmac_channel_set_dest_port_width(n,w)		\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DWDH_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DWDH_##w;	\
} while (0)

/* w=8,16,32 */
#define __bdmac_channel_set_src_port_width(n,w)		\
do {							\
	REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_SWDH_MASK;	\
	REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_SWDH_##w;	\
} while (0)

#define __bdmac_channel_dest_addr_fixed(n) \
	(REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_DAI)
#define __bdmac_channel_dest_addr_increment(n) \
	(REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_DAI)

#define __bdmac_channel_src_addr_fixed(n) \
	(REG_BDMAC_DCMD((n)) &= ~BDMAC_DCMD_SAI)
#define __bdmac_channel_src_addr_increment(n) \
	(REG_BDMAC_DCMD((n)) |= BDMAC_DCMD_SAI)

#define __bdmac_channel_set_doorbell(n)	\
	(REG_BDMAC_DMADBSR = (1 << (n)))

#define __bdmac_channel_irq_detected(n)  (REG_BDMAC_DMAIPR & (1 << (n)))
#define __bdmac_channel_ack_irq(n)       (REG_BDMAC_DMAIPR &= ~(1 <<(n)))

static __inline__ int __bdmac_get_irq(void)
{
	int i;
	for (i = 0; i < MAX_BDMA_NUM; i++)
		if (__bdmac_channel_irq_detected(i))
			return i;
	return -1;
}

#endif /* __MIPS_ASSEMBLER */

#define	CIM_BASE	0xB3060000

/*************************************************************************
 * CIM
 *************************************************************************/
#define	CIM_CFG			(CIM_BASE + 0x0000)
#define	CIM_CTRL		(CIM_BASE + 0x0004)
#define	CIM_STATE		(CIM_BASE + 0x0008)
#define	CIM_IID			(CIM_BASE + 0x000C)
#define	CIM_DA			(CIM_BASE + 0x0020)
#define	CIM_FA			(CIM_BASE + 0x0024)
#define	CIM_FID			(CIM_BASE + 0x0028)
#define	CIM_CMD			(CIM_BASE + 0x002C)
#define	CIM_SIZE		(CIM_BASE + 0x0030)
#define	CIM_OFFSET		(CIM_BASE + 0x0034)
#define CIM_YFA			(CIM_BASE + 0x0038)
#define CIM_YCMD		(CIM_BASE + 0x003C)
#define CIM_CBFA		(CIM_BASE + 0x0040)
#define CIM_CBCMD		(CIM_BASE + 0x0044)
#define CIM_CRFA		(CIM_BASE + 0x0048)
#define CIM_CRCMD		(CIM_BASE + 0x004C)
#define CIM_CTRL2		(CIM_BASE + 0x0050)

#define	CIM_RAM_ADDR		(CIM_BASE + 0x1000)

#define	REG_CIM_CFG		REG32(CIM_CFG)
#define	REG_CIM_CTRL		REG32(CIM_CTRL)
#define	REG_CIM_STATE		REG32(CIM_STATE)
#define	REG_CIM_IID		REG32(CIM_IID)
#define	REG_CIM_DA		REG32(CIM_DA)
#define	REG_CIM_FA		REG32(CIM_FA)
#define	REG_CIM_FID		REG32(CIM_FID)
#define	REG_CIM_CMD		REG32(CIM_CMD)
#define	REG_CIM_SIZE		REG32(CIM_SIZE)
#define	REG_CIM_OFFSET		REG32(CIM_OFFSET)
#define REG_CIM_YFA		REG32(CIM_YFA)
#define REG_CIM_YCMD		REG32(CIM_YCMD)
#define REG_CIM_CBFA		REG32(CIM_CBFA)
#define REG_CIM_CBCMD		REG32(CIM_CBCMD)
#define REG_CIM_CRFA		REG32(CIM_CRFA)
#define REG_CIM_CRCMD		REG32(CIM_CRCMD)
#define	REG_CIM_CTRL2		REG32(CIM_CTRL2)

#define CIM_CFG_EEOFEN			(1 << 31)
#define CIM_CFG_EXP			(1 << 30)

#define CIM_CFG_RXF_TRIG_BIT		24
#define CIM_CFG_RXF_TRIG_MASK		(0x3f << CIM_CFG_RXF_TRIG_BIT)

#define CIM_CFG_BW_BIT			22
#define CIM_CFG_BW_MASK			(0x3 << CIM_CFG_BW_BIT)

#define CIM_CFG_SEP			(1 << 20)

#define	CIM_CFG_ORDER_BIT		18
#define	CIM_CFG_ORDER_MASK		(0x3 << CIM_CFG_ORDER_BIT)
#define CIM_CFG_ORDER_0	  		(0x0 << CIM_CFG_ORDER_BIT) 	/* Y0CbY1Cr; YCbCr */
#define CIM_CFG_ORDER_1	  		(0x1 << CIM_CFG_ORDER_BIT)	/* Y0CrY1Cb; YCrCb */
#define CIM_CFG_ORDER_2	  		(0x2 << CIM_CFG_ORDER_BIT)	/* CbY0CrY1; CbCrY */
#define CIM_CFG_ORDER_3	  		(0x3 << CIM_CFG_ORDER_BIT)	/* CrY0CbY1; CrCbY */

#define	CIM_CFG_DF_BIT			16
#define	CIM_CFG_DF_MASK			(0x3 << CIM_CFG_DF_BIT)
#define CIM_CFG_DF_YUV444		(0x1 << CIM_CFG_DF_BIT) 	/* YCbCr444 */
#define CIM_CFG_DF_YUV422		(0x2 << CIM_CFG_DF_BIT)	/* YCbCr422 */
#define CIM_CFG_DF_ITU656		(0x3 << CIM_CFG_DF_BIT)	/* ITU656 YCbCr422 */

#define	CIM_CFG_INV_DAT			(1 << 15)
#define	CIM_CFG_VSP			(1 << 14) /* VSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_HSP			(1 << 13) /* HSYNC Polarity:0-rising edge active,1-falling edge active */
#define	CIM_CFG_PCP			(1 << 12) /* PCLK working edge: 0-rising, 1-falling */

#define	CIM_CFG_DMA_BURST_TYPE_BIT	10
#define	CIM_CFG_DMA_BURST_TYPE_MASK	(0x3 << CIM_CFG_DMA_BURST_TYPE_BIT)
#define	CIM_CFG_DMA_BURST_INCR4		(0 << CIM_CFG_DMA_BURST_TYPE_BIT)
#define	CIM_CFG_DMA_BURST_INCR8		(1 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested */
#define	CIM_CFG_DMA_BURST_INCR16	(2 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/
#define	CIM_CFG_DMA_BURST_INCR32	(3 << CIM_CFG_DMA_BURST_TYPE_BIT)	/* Suggested High speed AHB*/

#define	CIM_CFG_DUMMY_ZERO		(1 << 9)
#define	CIM_CFG_EXT_VSYNC		(1 << 8)	/* Only for ITU656 Progressive mode */
#define	CIM_CFG_LM			(1 << 7)	/* Only for ITU656 Progressive mode */
#define	CIM_CFG_PACK_BIT		4
#define	CIM_CFG_PACK_MASK		(0x7 << CIM_CFG_PACK_BIT)
#define CIM_CFG_PACK_0	  		(0 << CIM_CFG_PACK_BIT) /* 11 22 33 44 0xY0CbY1Cr */
#define CIM_CFG_PACK_1	  		(1 << CIM_CFG_PACK_BIT) /* 22 33 44 11 0xCbY1CrY0 */
#define CIM_CFG_PACK_2	  		(2 << CIM_CFG_PACK_BIT) /* 33 44 11 22 0xY1CrY0Cb */
#define CIM_CFG_PACK_3	  		(3 << CIM_CFG_PACK_BIT) /* 44 11 22 33 0xCrY0CbY1 */
#define CIM_CFG_PACK_4	  		(4 << CIM_CFG_PACK_BIT) /* 44 33 22 11 0xCrY1CbY0 */
#define CIM_CFG_PACK_5	  		(5 << CIM_CFG_PACK_BIT) /* 33 22 11 44 0xY1CbY0Cr */
#define CIM_CFG_PACK_6	  		(6 << CIM_CFG_PACK_BIT) /* 22 11 44 33 0xCbY0CrY1 */
#define CIM_CFG_PACK_7	  		(7 << CIM_CFG_PACK_BIT) /* 11 44 33 22 0xY0CrY1Cb */
#define	CIM_CFG_FP			(1 << 3)	/* Only for ITU656 Progressive mode */
#define	CIM_CFG_BYPASS_BIT		2
#define	CIM_CFG_BYPASS_MASK		(1 << CIM_CFG_BYPASS_BIT)
#define CIM_CFG_BYPASS	  		(1 << CIM_CFG_BYPASS_BIT)
#define	CIM_CFG_DSM_BIT			0
#define	CIM_CFG_DSM_MASK		(0x3 << CIM_CFG_DSM_BIT)
#define CIM_CFG_DSM_CPM	  		(0 << CIM_CFG_DSM_BIT) /* CCIR656 Progressive Mode */
#define CIM_CFG_DSM_CIM	  		(1 << CIM_CFG_DSM_BIT) /* CCIR656 Interlace Mode */
#define CIM_CFG_DSM_GCM	  		(2 << CIM_CFG_DSM_BIT) /* Gated Clock Mode */

/* CIM Control Register  (CIM_CTRL) */
#define	CIM_CTRL_EEOF_LINE_BIT	20
#define	CIM_CTRL_EEOF_LINE_MASK	(0xfff << CIM_CTRL_EEOF_LINE_BIT)

#define	CIM_CTRL_FRC_BIT	16
#define	CIM_CTRL_FRC_MASK	(0xf << CIM_CTRL_FRC_BIT)
#define CIM_CTRL_FRC_1	  (0x0 << CIM_CTRL_FRC_BIT) /* Sample every frame */
#define CIM_CTRL_FRC_2	  (0x1 << CIM_CTRL_FRC_BIT) /* Sample 1/2 frame */
#define CIM_CTRL_FRC_3	  (0x2 << CIM_CTRL_FRC_BIT) /* Sample 1/3 frame */
#define CIM_CTRL_FRC_4	  (0x3 << CIM_CTRL_FRC_BIT) /* Sample 1/4 frame */
#define CIM_CTRL_FRC_5	  (0x4 << CIM_CTRL_FRC_BIT) /* Sample 1/5 frame */
#define CIM_CTRL_FRC_6	  (0x5 << CIM_CTRL_FRC_BIT) /* Sample 1/6 frame */
#define CIM_CTRL_FRC_7	  (0x6 << CIM_CTRL_FRC_BIT) /* Sample 1/7 frame */
#define CIM_CTRL_FRC_8	  (0x7 << CIM_CTRL_FRC_BIT) /* Sample 1/8 frame */
#define CIM_CTRL_FRC_9	  (0x8 << CIM_CTRL_FRC_BIT) /* Sample 1/9 frame */
#define CIM_CTRL_FRC_10	  (0x9 << CIM_CTRL_FRC_BIT) /* Sample 1/10 frame */
#define CIM_CTRL_FRC_11	  (0xA << CIM_CTRL_FRC_BIT) /* Sample 1/11 frame */
#define CIM_CTRL_FRC_12	  (0xB << CIM_CTRL_FRC_BIT) /* Sample 1/12 frame */
#define CIM_CTRL_FRC_13	  (0xC << CIM_CTRL_FRC_BIT) /* Sample 1/13 frame */
#define CIM_CTRL_FRC_14	  (0xD << CIM_CTRL_FRC_BIT) /* Sample 1/14 frame */
#define CIM_CTRL_FRC_15	  (0xE << CIM_CTRL_FRC_BIT) /* Sample 1/15 frame */
#define CIM_CTRL_FRC_16	  (0xF << CIM_CTRL_FRC_BIT) /* Sample 1/16 frame */

#define	CIM_CTRL_DMA_EEOF	(1 << 15)	/* Enable EEOF interrupt */
#define	CIM_CTRL_WIN_EN		(1 << 14)
#define	CIM_CTRL_VDDM		(1 << 13) /* VDD interrupt enable */
#define	CIM_CTRL_DMA_SOFM	(1 << 12)
#define	CIM_CTRL_DMA_EOFM	(1 << 11)
#define	CIM_CTRL_DMA_STOPM	(1 << 10)
#define	CIM_CTRL_RXF_TRIGM	(1 << 9)
#define	CIM_CTRL_RXF_OFM	(1 << 8)
#define	CIM_CTRL_DMA_SYNC	(1 << 7)	/*when change DA, do frame sync */
#define CIM_CTRL_H_SYNC         (1 << 6) /*Enable horizental sync when CIMCFG.SEP is 1*/       

#define	CIM_CTRL_PPW_BIT	3
#define	CIM_CTRL_PPW_MASK	(0x3 << CIM_CTRL_PPW_BIT)

#define	CIM_CTRL_DMA_EN		(1 << 2) /* Enable DMA */
#define	CIM_CTRL_RXF_RST	(1 << 1) /* RxFIFO reset */
#define	CIM_CTRL_ENA		(1 << 0) /* Enable CIM */

/* cim control2 */
#define CIM_CTRL2_OPG_BIT	4
#define CIM_CTRL2_OPG_MASK	(0x3 << CIM_CTRL2_OPG_BIT)
#define CIM_CTRL2_OPE		(1 << 2)
#define CIM_CTRL2_EME		(1 << 1)
#define CIM_CTRL2_APM		(1 << 0)

/* CIM State Register  (CIM_STATE) */
#define CIM_STATE_CR_RF_OF	(1 << 27)
#define CIM_STATE_CR_RF_TRIG	(1 << 26)
#define CIM_STATE_CR_RF_EMPTY	(1 << 25)
#define CIM_STATE_CB_RF_OF	(1 << 19)
#define CIM_STATE_CB_RF_TRIG	(1 << 18)
#define CIM_STATE_CB_RF_EMPTY	(1 << 17)
#define CIM_STATE_Y_RF_OF	(1 << 11)
#define CIM_STATE_Y_RF_TRIG	(1 << 10)
#define CIM_STATE_Y_RF_EMPTY	(1 << 9)
#define	CIM_STATE_DMA_EEOF	(1 << 7) /* DMA Line EEOf irq */
#define	CIM_STATE_DMA_SOF	(1 << 6) /* DMA start irq */
#define	CIM_STATE_DMA_EOF	(1 << 5) /* DMA end irq */
#define	CIM_STATE_DMA_STOP	(1 << 4) /* DMA stop irq */
#define	CIM_STATE_RXF_OF	(1 << 3) /* RXFIFO over flow irq */
#define	CIM_STATE_RXF_TRIG	(1 << 2) /* RXFIFO triger meet irq */
#define	CIM_STATE_RXF_EMPTY	(1 << 1) /* RXFIFO empty irq */
#define	CIM_STATE_VDD		(1 << 0) /* CIM disabled irq */

/* CIM DMA Command Register (CIM_CMD) */

#define	CIM_CMD_SOFINT		(1 << 31) /* enable DMA start irq */
#define	CIM_CMD_EOFINT		(1 << 30) /* enable DMA end irq */
#define	CIM_CMD_EEOFINT		(1 << 29) /* enable DMA EEOF irq */
#define	CIM_CMD_STOP		(1 << 28) /* enable DMA stop irq */
#define	CIM_CMD_OFRCV		(1 << 27) /* enable recovery when TXFiFo overflow */
#define	CIM_CMD_LEN_BIT		0
#define	CIM_CMD_LEN_MASK	(0xffffff << CIM_CMD_LEN_BIT)

/* CIM Window-Image Size Register  (CIM_SIZE) */
#define	CIM_SIZE_LPF_BIT	16 /* Lines per freame for csc output image */
#define	CIM_SIZE_LPF_MASK	(0x1fff << CIM_SIZE_LPF_BIT)
#define	CIM_SIZE_PPL_BIT	0 /* Pixels per line for csc output image, should be an even number */
#define	CIM_SIZE_PPL_MASK	(0x1fff << CIM_SIZE_PPL_BIT)

/* CIM Image Offset Register  (CIM_OFFSET) */
#define	CIM_OFFSET_V_BIT	16 /* Vertical offset */
#define	CIM_OFFSET_V_MASK	(0xfff << CIM_OFFSET_V_BIT)
#define	CIM_OFFSET_H_BIT	0 /* Horizontal offset, should be an enen number */
#define	CIM_OFFSET_H_MASK	(0xfff << CIM_OFFSET_H_BIT) /*OFFSET_H should be even number*/

#define	CIM_YCMD_SOFINT		(1 << 31) /* enable DMA start irq */
#define	CIM_YCMD_EOFINT		(1 << 30) /* enable DMA end irq */
#define	CIM_YCMD_EEOFINT	(1 << 29) /* enable DMA EEOF irq */
#define	CIM_YCMD_STOP		(1 << 28) /* enable DMA stop irq */
#define	CIM_YCMD_OFRCV		(1 << 27) /* enable recovery when TXFiFo overflow */
#define	CIM_YCMD_LEN_BIT	0
#define	CIM_YCMD_LEN_MASK	(0xffffff << CIM_YCMD_LEN_BIT)

#define	CIM_CBCMD_LEN_BIT	0
#define	CIM_CBCMD_LEN_MASK	(0xffffff << CIM_CBCMD_LEN_BIT)

#define	CIM_CRCMD_LEN_BIT	0
#define	CIM_CRCMD_LEN_MASK	(0xffffff << CIM_CRCMD_LEN_BIT)

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * CIM
 ***************************************************************************/

#define __cim_enable()	( REG_CIM_CTRL |= CIM_CTRL_ENA )
#define __cim_disable()	( REG_CIM_CTRL &= ~CIM_CTRL_ENA )

#define __cim_enable_sep() (REG_CIM_CFG |= CIM_CFG_SEP)
#define __cim_disable_sep() (REG_CIM_CFG &= ~CIM_CFG_SEP)

/* n = 0, 1, 2, 3 */
#define __cim_set_input_data_stream_order(n)				\
	do {								\
		REG_CIM_CFG &= ~CIM_CFG_ORDER_MASK;			\
		REG_CIM_CFG |= ((n)<<CIM_CFG_ORDER_BIT)&CIM_CFG_ORDER_MASK; \
	} while (0)

#define __cim_input_data_format_select_YUV444()		\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DF_MASK;		\
		REG_CIM_CFG |= CIM_CFG_DF_YUV444;	\
	} while (0)

#define __cim_input_data_format_select_YUV422()		\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DF_MASK;		\
		REG_CIM_CFG |= CIM_CFG_DF_YUV422;	\
	} while (0)

#define __cim_input_data_format_select_ITU656()		\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DF_MASK;		\
		REG_CIM_CFG |= CIM_CFG_DF_ITU656;	\
	} while (0)

#define __cim_input_data_inverse()	( REG_CIM_CFG |= CIM_CFG_INV_DAT )
#define __cim_input_data_normal()	( REG_CIM_CFG &= ~CIM_CFG_INV_DAT )

#define __cim_vsync_active_low()	( REG_CIM_CFG |= CIM_CFG_VSP )
#define __cim_vsync_active_high()	( REG_CIM_CFG &= ~CIM_CFG_VSP )

#define __cim_hsync_active_low()	( REG_CIM_CFG |= CIM_CFG_HSP )
#define __cim_hsync_active_high()	( REG_CIM_CFG &= ~CIM_CFG_HSP )

#define __cim_sample_data_at_pclk_falling_edge() \
	( REG_CIM_CFG |= CIM_CFG_PCP )
#define __cim_sample_data_at_pclk_rising_edge() \
	( REG_CIM_CFG &= ~CIM_CFG_PCP )

#define __cim_enable_dummy_zero()	( REG_CIM_CFG |= CIM_CFG_DUMMY_ZERO )
#define __cim_disable_dummy_zero()	( REG_CIM_CFG &= ~CIM_CFG_DUMMY_ZERO )

#define __cim_select_external_vsync()	( REG_CIM_CFG |= CIM_CFG_EXT_VSYNC )
#define __cim_select_internal_vsync()	( REG_CIM_CFG &= ~CIM_CFG_EXT_VSYNC )

/* n=0-7 */
#define __cim_set_data_packing_mode(n) 		\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_PACK_MASK;	\
		REG_CIM_CFG |= (CIM_CFG_PACK_##n);	\
	} while (0)

#define __cim_enable_bypass_func() 	(REG_CIM_CFG &= ~CIM_CFG_BYPASS)
#define __cim_disable_bypass_func() 	(REG_CIM_CFG |= CIM_CFG_BYPASS)

#define __cim_enable_ccir656_progressive_mode()	\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DSM_MASK;	\
		REG_CIM_CFG |= CIM_CFG_DSM_CPM;		\
	} while (0)

#define __cim_enable_ccir656_interlace_mode()	\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DSM_MASK;	\
		REG_CIM_CFG |= CIM_CFG_DSM_CIM;		\
	} while (0)

#define __cim_enable_gated_clock_mode()		\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DSM_MASK;	\
		REG_CIM_CFG |= CIM_CFG_DSM_GCM;		\
	} while (0)

#define __cim_enable_nongated_clock_mode()	\
	do {						\
		REG_CIM_CFG &= ~CIM_CFG_DSM_MASK;	\
		REG_CIM_CFG |= CIM_CFG_DSM_NGCM;	\
	} while (0)

/* n=1-16 */
#define __cim_set_frame_rate(n) 		\
	do {						\
		REG_CIM_CTRL &= ~CIM_CTRL_FRC_MASK; 	\
		REG_CIM_CTRL |= CIM_CTRL_FRC_##n;	\
	} while (0)

#define __cim_enable_size_func() \
	( REG_CIM_CTRL |= CIM_CTRL_WIN_EN)
#define __cim_disable_size_func() \
	( REG_CIM_CTRL &= ~CIM_CTRL_WIN_EN )

#define __cim_enable_vdd_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_VDDM )
#define __cim_disable_vdd_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_VDDM )

#define __cim_enable_sof_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_DMA_SOFM )
#define __cim_disable_sof_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_DMA_SOFM )

#define __cim_enable_eof_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_DMA_EOFM )
#define __cim_disable_eof_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_DMA_EOFM )

#define __cim_enable_eeof_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_DMA_EEOFM )
#define __cim_disable_eeof_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_DMA_EEOFM )

#define __cim_enable_stop_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_DMA_STOPM )
#define __cim_disable_stop_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_DMA_STOPM )

#define __cim_enable_trig_intr() \
	( REG_CIM_CTRL |= CIM_CTRL_RXF_TRIGM )
#define __cim_disable_trig_intr() \
	( REG_CIM_CTRL &= ~CIM_CTRL_RXF_TRIGM )

#define __cim_enable_rxfifo_overflow_intr()	\
	( REG_CIM_CTRL |= CIM_CTRL_RXF_OFM )
#define __cim_disable_rxfifo_overflow_intr()	\
	( REG_CIM_CTRL &= ~CIM_CTRL_RXF_OFM )

#define __cim_enable_dma()   ( REG_CIM_CTRL |= CIM_CTRL_DMA_EN )
#define __cim_disable_dma()  ( REG_CIM_CTRL &= ~CIM_CTRL_DMA_EN )
#define __cim_reset_rxfifo() ( REG_CIM_CTRL |= CIM_CTRL_RXF_RST )
#define __cim_unreset_rxfifo() ( REG_CIM_CTRL &= ~CIM_CTRL_RXF_RST )

/* cim control2 */
#define __cim_enable_priority_control()		( REG_CIM_CTRL2 |= CIM_CTRL2_OPE)
#define __cim_disable_priority_control()	( REG_CIM_CTRL2 &= ~CIM_CTRL2_OPE)
#define __cim_enable_auto_priority()		( REG_CIM_CTRL2 |= CIM_CTRL2_APM)
#define __cim_disable_auto_priority()		( REG_CIM_CTRL2 &= ~CIM_CTRL2_APM)
#define __cim_enable_emergency()		( REG_CIM_CTRL2 |= CIM_CTRL2_EME)
#define __cim_disable_emergency()		( REG_CIM_CTRL2 &= ~CIM_CTRL2_EME);
/* 0, 1, 2, 3
 ** 0: highest priority
 ** 3: lowest priority
 ** 1 maybe best for SEP=1
 ** 3 maybe best for SEP=0
 **/
#define __cim_set_opg(n)				\
	do {								\
		REG_CIM_CTRL2 &= ~CIM_CTRL2_OPG_MASK;			\
		REG_CIM_CTRL2 |= ((n) << CIM_CTRL2_OPG_BIT) & CIM_CTRL2_OPG_MASK; \
	} while (0)

#define __cim_clear_state()   	     ( REG_CIM_STATE = 0 )

#define __cim_disable_done()   	     ( REG_CIM_STATE & CIM_STATE_VDD )
#define __cim_rxfifo_empty()   	     ( REG_CIM_STATE & CIM_STATE_RXF_EMPTY )
#define __cim_rxfifo_reach_trigger() ( REG_CIM_STATE & CIM_STATE_RXF_TRIG )
#define __cim_rxfifo_overflow()      ( REG_CIM_STATE & CIM_STATE_RXF_OF )
#define __cim_clear_rxfifo_overflow() ( REG_CIM_STATE &= ~CIM_STATE_RXF_OF )
#define __cim_dma_stop()   	     ( REG_CIM_STATE & CIM_STATE_DMA_STOP )
#define __cim_dma_eof()   	     ( REG_CIM_STATE & CIM_STATE_DMA_EOF )
#define __cim_dma_sof()   	     ( REG_CIM_STATE & CIM_STATE_DMA_SOF )

#define __cim_get_iid()   	     ( REG_CIM_IID )
#define __cim_get_fid()   	     ( REG_CIM_FID )
//#define __cim_get_image_data()       ( REG_CIM_RXFIFO )
#define __cim_get_dma_cmd()          ( REG_CIM_CMD )

#define __cim_set_da(a)              ( REG_CIM_DA = (a) )

#define __cim_set_line(a) 	( REG_CIM_SIZE = (REG_CIM_SIZE&(~CIM_SIZE_LPF_MASK))|((a)<<CIM_SIZE_LPF_BIT) )
#define __cim_set_pixel(a) 	( REG_CIM_SIZE = (REG_CIM_SIZE&(~CIM_SIZE_PPL_MASK))|((a)<<CIM_SIZE_PPL_BIT) )
#define __cim_get_line() 	((REG_CIM_SIZE&CIM_SIZE_LPF_MASK)>>CIM_SIZE_LPF_BIT)
#define __cim_get_pixel() 	((REG_CIM_SIZE&CIM_SIZE_PPL_MASK)>>CIM_SIZE_PPL_BIT)

#define __cim_set_v_offset(a) 	( REG_CIM_OFFSET = (REG_CIM_OFFSET&(~CIM_OFFSET_V_MASK)) | ((a)<<CIM_OFFSET_V_BIT) )
#define __cim_set_h_offset(a) 	( REG_CIM_OFFSET = (REG_CIM_OFFSET&(~CIM_OFFSET_H_MASK)) | ((a)<<CIM_OFFSET_H_BIT) )
#define __cim_get_v_offset() 	((REG_CIM_OFFSET&CIM_OFFSET_V_MASK)>>CIM_OFFSET_V_BIT)
#define __cim_get_h_offset() 	((REG_CIM_OFFSET&CIM_OFFSET_H_MASK)>>CIM_OFFSET_H_BIT)

#endif /* __MIPS_ASSEMBLER */

/*
 * Clock reset and power controller module(CPM) address definition
 */
#define	CPM_BASE	0xb0000000

/*
 * CPM registers offset address definition
 */
#define CPM_CPCCR_OFFSET        (0x00)  /* rw, 32, 0x01011100 */
#define CPM_LCR_OFFSET          (0x04)  /* rw, 32, 0x000000f8 */
#define CPM_RSR_OFFSET          (0x08)  /* rw, 32, 0x???????? */
#define CPM_CPPCR0_OFFSET       (0x10)  /* rw, 32, 0x28080011 */
#define CPM_CPPSR_OFFSET        (0x14)  /* rw, 32, 0x80000000 */
#define CPM_CLKGR0_OFFSET       (0x20)  /* rw, 32, 0x3fffffe0 */
#define CPM_OPCR_OFFSET         (0x24)  /* rw, 32, 0x00001570 */
#define CPM_CLKGR1_OFFSET       (0x28)  /* rw, 32, 0x0000017f */
#define CPM_CPPCR1_OFFSET       (0x30)  /* rw, 32, 0x28080002 */
#define CPM_CPSPR_OFFSET        (0x34)  /* rw, 32, 0x???????? */
#define CPM_CPSPPR_OFFSET       (0x38)  /* rw, 32, 0x0000a5a5 */
#define CPM_USBPCR_OFFSET       (0x3c)  /* rw, 32, 0x42992198 */
#define CPM_USBRDT_OFFSET       (0x40)  /* rw, 32, 0x00000096 */
#define CPM_USBVBFIL_OFFSET     (0x44)  /* rw, 32, 0x00000080 */
#define CPM_USBCDR_OFFSET       (0x50)  /* rw, 32, 0x00000000 */
#define CPM_I2SCDR_OFFSET       (0x60)  /* rw, 32, 0x00000000 */
#define CPM_LPCDR_OFFSET        (0x64)  /* rw, 32, 0x00000000 */
#define CPM_MSCCDR_OFFSET       (0x68)  /* rw, 32, 0x00000000 */
#define CPM_UHCCDR_OFFSET       (0x6c)  /* rw, 32, 0x00000000 */
#define CPM_SSICDR_OFFSET       (0x74)  /* rw, 32, 0x00000000 */
#define CPM_CIMCDR_OFFSET       (0x7c)  /* rw, 32, 0x00000000 */
#define CPM_GPSCDR_OFFSET       (0x80)  /* rw, 32, 0x00000000 */
#define CPM_PCMCDR_OFFSET       (0x84)  /* rw, 32, 0x00000000 */
#define CPM_GPUCDR_OFFSET       (0x88)  /* rw, 32, 0x00000000 */
#define CPM_PSWC0ST_OFFSET      (0x90)  /* rw, 32, 0x00000000 */
#define CPM_PSWC1ST_OFFSET      (0x94)  /* rw, 32, 0x00000000 */
#define CPM_PSWC2ST_OFFSET      (0x98)  /* rw, 32, 0x00000000 */
#define CPM_PSWC3ST_OFFSET      (0x9c)  /* rw, 32, 0x00000000 */

/*
 * CPM registers address definition
 */
#define CPM_CPCCR        (CPM_BASE + CPM_CPCCR_OFFSET)
#define CPM_LCR          (CPM_BASE + CPM_LCR_OFFSET)
#define CPM_RSR          (CPM_BASE + CPM_RSR_OFFSET)
#define CPM_CPPCR0       (CPM_BASE + CPM_CPPCR0_OFFSET)
#define CPM_CPPSR        (CPM_BASE + CPM_CPPSR_OFFSET)
#define CPM_CLKGR0       (CPM_BASE + CPM_CLKGR0_OFFSET)
#define CPM_OPCR         (CPM_BASE + CPM_OPCR_OFFSET)
#define CPM_CLKGR1       (CPM_BASE + CPM_CLKGR1_OFFSET)
#define CPM_CPPCR1       (CPM_BASE + CPM_CPPCR1_OFFSET)
#define CPM_CPSPR        (CPM_BASE + CPM_CPSPR_OFFSET)
#define CPM_CPSPPR       (CPM_BASE + CPM_CPSPPR_OFFSET)
#define CPM_USBPCR       (CPM_BASE + CPM_USBPCR_OFFSET)
#define CPM_USBRDT       (CPM_BASE + CPM_USBRDT_OFFSET)
#define CPM_USBVBFIL     (CPM_BASE + CPM_USBVBFIL_OFFSET)
#define CPM_USBCDR       (CPM_BASE + CPM_USBCDR_OFFSET)
#define CPM_I2SCDR       (CPM_BASE + CPM_I2SCDR_OFFSET)
#define CPM_LPCDR        (CPM_BASE + CPM_LPCDR_OFFSET)
#define CPM_MSCCDR       (CPM_BASE + CPM_MSCCDR_OFFSET)
#define CPM_UHCCDR       (CPM_BASE + CPM_UHCCDR_OFFSET)
#define CPM_SSICDR       (CPM_BASE + CPM_SSICDR_OFFSET)
#define CPM_CIMCDR       (CPM_BASE + CPM_CIMCDR_OFFSET)
#define CPM_GPSCDR       (CPM_BASE + CPM_GPSCDR_OFFSET)
#define CPM_PCMCDR       (CPM_BASE + CPM_PCMCDR_OFFSET)
#define CPM_GPUCDR       (CPM_BASE + CPM_GPUCDR_OFFSET)
#define CPM_PSWC0ST      (CPM_BASE + CPM_PSWC0ST_OFFSET)
#define CPM_PSWC1ST      (CPM_BASE + CPM_PSWC1ST_OFFSET)
#define CPM_PSWC2ST      (CPM_BASE + CPM_PSWC2ST_OFFSET)
#define CPM_PSWC3ST      (CPM_BASE + CPM_PSWC3ST_OFFSET)

/*
 * CPM registers common define
 */

/* Clock control register(CPCCR) */
#define CPCCR_ECS               BIT31
#define CPCCR_MEM               BIT30
#define CPCCR_CE                BIT22
#define CPCCR_PCS               BIT21

#define CPCCR_SDIV_LSB          24
#define CPCCR_SDIV_MASK         BITS_H2L(27, CPCCR_SDIV_LSB)

#define CPCCR_H2DIV_LSB         16
#define CPCCR_H2DIV_MASK        BITS_H2L(19, CPCCR_H2DIV_LSB)

#define CPCCR_MDIV_LSB          12
#define CPCCR_MDIV_MASK         BITS_H2L(15, CPCCR_MDIV_LSB)

#define CPCCR_PDIV_LSB		8
#define CPCCR_PDIV_MASK         BITS_H2L(11, CPCCR_PDIV_LSB)

#define CPCCR_HDIV_LSB		4
#define CPCCR_HDIV_MASK         BITS_H2L(7, CPCCR_HDIV_LSB)

#define CPCCR_CDIV_LSB		0
#define CPCCR_CDIV_MASK         BITS_H2L(3, CPCCR_CDIV_LSB)

/* Low power control register(LCR) */
#define LCR_PDAHB1              BIT30
#define LCR_PDAHB1S             BIT26
#define LCR_DOZE                BIT2

#define LCR_PST_LSB             8
#define LCR_PST_MASK            BITS_H2L(19, LCR_PST_LSB)

#define LCR_DUTY_LSB            3
#define LCR_DUTY_MASK           BITS_H2L(7, LCR_DUTY_LSB)

#define LCR_LPM_LSB             0
#define LCR_LPM_MASK            BITS_H2L(1, LCR_LPM_LSB)
#define LCR_LPM_IDLE            (0x0 << LCR_LPM_LSB)
#define LCR_LPM_SLEEP           (0x1 << LCR_LPM_LSB)

/* Reset status register(RSR) */
#define RSR_P0R         BIT2
#define RSR_WR          BIT1
#define RSR_PR          BIT0

/* PLL control register 0(CPPCR0) */
#define CPPCR0_LOCK             BIT15   /* LOCK0 bit */
#define CPPCR0_PLLS             BIT10
#define CPPCR0_PLLBP            BIT9
#define CPPCR0_PLLEN            BIT8

#define CPPCR0_PLLM_LSB         24
#define CPPCR0_PLLM_MASK        BITS_H2L(30, CPPCR0_PLLM_LSB)

#define CPPCR0_PLLN_LSB         18
#define CPPCR0_PLLN_MASK        BITS_H2L(21, CPPCR0_PLLN_LSB)

#define CPPCR0_PLLOD_LSB        16
#define CPPCR0_PLLOD_MASK       BITS_H2L(17, CPPCR0_PLLOD_LSB)

#define CPPCR0_PLLST_LSB        0
#define CPPCR0_PLLST_MASK       BITS_H2L(7, CPPCR0_PLLST_LSB)

/* PLL switch and status register(CPPSR) */
#define CPPSR_PLLOFF            BIT31
#define CPPSR_PLLBP             BIT30
#define CPPSR_PLLON             BIT29
#define CPPSR_PS                BIT28
#define CPPSR_FS                BIT27
#define CPPSR_CS                BIT26
#define CPPSR_SM                BIT2
#define CPPSR_PM                BIT1
#define CPPSR_FM                BIT0

/* Clock gate register 0(CGR0) */
#define CLKGR0_AHB_MON		BIT31
#define CLKGR0_DDR              BIT30
#define CLKGR0_IPU              BIT29
#define CLKGR0_LCD              BIT28
#define CLKGR0_TVE              BIT27
#define CLKGR0_CIM              BIT26
#define CLKGR0_MDMA             BIT25
#define CLKGR0_UHC              BIT24
#define CLKGR0_MAC              BIT23
#define CLKGR0_GPS              BIT22
#define CLKGR0_DMAC             BIT21
#define CLKGR0_SSI2             BIT20
#define CLKGR0_SSI1             BIT19
#define CLKGR0_UART3            BIT18
#define CLKGR0_UART2            BIT17
#define CLKGR0_UART1            BIT16
#define CLKGR0_UART0            BIT15
#define CLKGR0_SADC             BIT14
#define CLKGR0_KBC              BIT13
#define CLKGR0_MSC2             BIT12
#define CLKGR0_MSC1             BIT11
#define CLKGR0_OWI              BIT10
#define CLKGR0_TSSI             BIT9
#define CLKGR0_AIC              BIT8
#define CLKGR0_SCC              BIT7
#define CLKGR0_I2C1             BIT6
#define CLKGR0_I2C0             BIT5
#define CLKGR0_SSI0             BIT4
#define CLKGR0_MSC0             BIT3
#define CLKGR0_OTG              BIT2
#define CLKGR0_BCH              BIT1
#define CLKGR0_NEMC             BIT0

/* Oscillator and power control register(OPCR) */
#define OPCR_OTGPHY_ENABLE      BIT7    /* SPENDN bit */
#define OPCR_GPSEN              BIT6
#define OPCR_UHCPHY_DISABLE     BIT5    /* SPENDH bit */
#define OPCR_O1SE               BIT4
#define OPCR_PD                 BIT3
#define OPCR_ERCS               BIT2

#define OPCR_O1ST_LSB           8
#define OPCR_O1ST_MASK          BITS_H2L(15, OPCR_O1ST_LSB)

/* Clock gate register 1(CGR1) */
#define CLKGR1_AUX		BIT11
#define CLKGR1_OSD		BIT10
#define CLKGR1_GPU              BIT9
#define CLKGR1_PCM              BIT8
#define CLKGR1_AHB1             BIT7
#define CLKGR1_CABAC            BIT6
#define CLKGR1_SRAM             BIT5
#define CLKGR1_DCT              BIT4
#define CLKGR1_ME               BIT3
#define CLKGR1_DBLK             BIT2
#define CLKGR1_MC               BIT1
#define CLKGR1_BDMA             BIT0

/* PLL control register 1(CPPCR1) */
#define CPPCR1_P1SCS            BIT15
#define CPPCR1_PLL1EN           BIT7
#define CPPCR1_PLL1S            BIT6
#define CPPCR1_LOCK             BIT2    /* LOCK1 bit */
#define CPPCR1_PLL1OFF          BIT1
#define CPPCR1_PLL1ON           BIT0

#define CPPCR1_PLL1M_LSB        24
#define CPPCR1_PLL1M_MASK       BITS_H2L(30, CPPCR1_PLL1M_LSB)

#define CPPCR1_PLL1N_LSB        18
#define CPPCR1_PLL1N_MASK       BITS_H2L(21, CPPCR1_PLL1N_LSB)

#define CPPCR1_PLL1OD_LSB       16
#define CPPCR1_PLL1OD_MASK      BITS_H2L(17, CPPCR1_PLL1OD_LSB)

#define CPPCR1_P1SDIV_LSB       9
#define CPPCR1_P1SDIV_MASK      BITS_H2L(14, CPPCR1_P1SDIV_LSB)

/* CPM scratch pad protected register(CPSPPR) */
#define CPSPPR_CPSPR_WRITABLE   (0x00005a5a)

/* OTG parameter control register(USBPCR) */
#define USBPCR_USB_MODE         BIT31
#define USBPCR_AVLD_REG         BIT30
#define USBPCR_INCRM            BIT27   /* INCR_MASK bit */
#define USBPCR_CLK12_EN         BIT26
#define USBPCR_COMMONONN        BIT25
#define USBPCR_VBUSVLDEXT       BIT24
#define USBPCR_VBUSVLDEXTSEL    BIT23
#define USBPCR_POR              BIT22
#define USBPCR_SIDDQ            BIT21
#define USBPCR_OTG_DISABLE      BIT20
#define USBPCR_TXPREEMPHTUNE    BIT6

#define USBPCR_IDPULLUP_LSB             28   /* IDPULLUP_MASK bit */
#define USBPCR_IDPULLUP_MASK            BITS_H2L(29, USBPCR_USBPCR_IDPULLUP_LSB)

#define USBPCR_COMPDISTUNE_LSB          17
#define USBPCR_COMPDISTUNE_MASK         BITS_H2L(19, USBPCR_COMPDISTUNE_LSB)

#define USBPCR_OTGTUNE_LSB              14
#define USBPCR_OTGTUNE_MASK             BITS_H2L(16, USBPCR_OTGTUNE_LSB)

#define USBPCR_SQRXTUNE_LSB             11
#define USBPCR_SQRXTUNE_MASK            BITS_H2L(13, USBPCR_SQRXTUNE_LSB)

#define USBPCR_TXFSLSTUNE_LSB           7
#define USBPCR_TXFSLSTUNE_MASK          BITS_H2L(10, USBPCR_TXFSLSTUNE_LSB)

#define USBPCR_TXRISETUNE_LSB           4
#define USBPCR_TXRISETUNE_MASK          BITS_H2L(5, USBPCR_TXRISETUNE_LSB)

#define USBPCR_TXVREFTUNE_LSB           0
#define USBPCR_TXVREFTUNE_MASK          BITS_H2L(3, USBPCR_TXVREFTUNE_LSB)

/* OTG reset detect timer register(USBRDT) */
#define USBRDT_HB_MASK		BIT26
#define USBRDT_VBFIL_LD_EN      BIT25
#define USBRDT_IDDIG_EN         BIT24
#define USBRDT_IDDIG_REG        BIT23

#define USBRDT_USBRDT_LSB       0
#define USBRDT_USBRDT_MASK      BITS_H2L(22, USBRDT_USBRDT_LSB)

/* OTG PHY clock divider register(USBCDR) */
#define USBCDR_UCS              BIT31
#define USBCDR_UPCS             BIT30

#define USBCDR_OTGDIV_LSB       0       /* USBCDR bit */
#define USBCDR_OTGDIV_MASK      BITS_H2L(5, USBCDR_OTGDIV_LSB)

/* I2S device clock divider register(I2SCDR) */ 
#define I2SCDR_I2CS             BIT31
#define I2SCDR_I2PCS            BIT30

#define I2SCDR_I2SDIV_LSB       0       /* I2SCDR bit */
#define I2SCDR_I2SDIV_MASK      BITS_H2L(8, I2SCDR_I2SDIV_LSB)

/* LCD pix clock divider register(LPCDR) */
//#define LPCDR_LSCS              BIT31
#define LPCDR_LTCS              BIT30
#define LPCDR_LPCS              BIT29

#define LPCDR_PIXDIV_LSB        0       /* LPCDR bit */
#define LPCDR_PIXDIV_MASK       BITS_H2L(10, LPCDR_PIXDIV_LSB)

/* MSC clock divider register(MSCCDR) */
#define MSCCDR_MCS              BIT31

#define MSCCDR_MSCDIV_LSB       0       /* MSCCDR bit */
#define MSCCDR_MSCDIV_MASK      BITS_H2L(4, MSCCDR_MSCDIV_LSB)

/* UHC device clock divider register(UHCCDR) */
#define UHCCDR_UHPCS            BIT31

#define UHCCDR_UHCDIV_LSB       0       /* UHCCDR bit */
#define UHCCDR_UHCDIV_MASK      BITS_H2L(3, UHCCDR_UHCDIV_LSB)

/* SSI clock divider register(SSICDR) */
#define SSICDR_SCS              BIT31

#define SSICDR_SSIDIV_LSB       0       /* SSICDR bit */
#define SSICDR_SSIDIV_MASK      BITS_H2L(3, SSICDR_SSIDIV_LSB)

/* CIM mclk clock divider register(CIMCDR) */
#define CIMCDR_CIMDIV_LSB       0       /* CIMCDR bit */
#define CIMCDR_CIMDIV_MASK      BITS_H2L(7, CIMCDR_CIMDIV_LSB)

/* GPS clock divider register(GPSCDR) */
#define GPSCDR_GPCS             BIT31

#define GPSCDR_GPSDIV_LSB       0       /* GPSCDR bit */
#define GSPCDR_GPSDIV_MASK      BITS_H2L(3, GPSCDR_GPSDIV_LSB)

/* PCM device clock divider register(PCMCDR) */
#define PCMCDR_PCMS             BIT31
#define PCMCDR_PCMPCS           BIT30

#define PCMCDR_PCMDIV_LSB       0       /* PCMCDR bit */
#define PCMCDR_PCMDIV_MASK      BITS_H2L(8, PCMCDR_PCMDIV_LSB)

/* GPU clock divider register */
#define GPUCDR_GPCS             BIT31
#define GPUCDR_GPUDIV_LSB       0       /* GPUCDR bit */
#define GPUCDR_GPUDIV_MASK      BITS_H2L(2, GPUCDR_GPUDIV_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_CPM_CPCCR           REG32(CPM_CPCCR)
#define REG_CPM_RSR             REG32(CPM_RSR)
#define REG_CPM_CPPCR0          REG32(CPM_CPPCR0)
#define REG_CPM_CPPSR           REG32(CPM_CPPSR)
#define REG_CPM_CPPCR1          REG32(CPM_CPPCR1)
#define REG_CPM_CPSPR           REG32(CPM_CPSPR)
#define REG_CPM_CPSPPR          REG32(CPM_CPSPPR)
#define REG_CPM_USBPCR          REG32(CPM_USBPCR)
#define REG_CPM_USBRDT          REG32(CPM_USBRDT)
#define REG_CPM_USBVBFIL        REG32(CPM_USBVBFIL)
#define REG_CPM_USBCDR          REG32(CPM_USBCDR)
#define REG_CPM_I2SCDR          REG32(CPM_I2SCDR)
#define REG_CPM_LPCDR           REG32(CPM_LPCDR)
#define REG_CPM_MSCCDR          REG32(CPM_MSCCDR)
#define REG_CPM_UHCCDR          REG32(CPM_UHCCDR)
#define REG_CPM_SSICDR          REG32(CPM_SSICDR)
#define REG_CPM_CIMCDR          REG32(CPM_CIMCDR)
#define REG_CPM_GPSCDR          REG32(CPM_GPSCDR)
#define REG_CPM_PCMCDR          REG32(CPM_PCMCDR)
#define REG_CPM_GPUCDR          REG32(CPM_GPUCDR)

#define REG_CPM_PSWC0ST         REG32(CPM_PSWC0ST)
#define REG_CPM_PSWC1ST         REG32(CPM_PSWC1ST)
#define REG_CPM_PSWC2ST         REG32(CPM_PSWC2ST)
#define REG_CPM_PSWC3ST         REG32(CPM_PSWC3ST)

#define REG_CPM_LCR             REG32(CPM_LCR)
#define REG_CPM_CLKGR0          REG32(CPM_CLKGR0)
#define REG_CPM_OPCR            REG32(CPM_OPCR)
#define REG_CPM_CLKGR1          REG32(CPM_CLKGR1)
#define REG_CPM_CLKGR           REG32(CPM_CLKGR0)

#define cpm_get_scrpad()	INREG32(CPM_CPSPR)
#define cpm_set_scrpad(data)				\
do {							\
	OUTREG32(CPM_CPSPPR, CPSPPR_CPSPR_WRITABLE);	\
	OUTREG32(CPM_CPSPR, data);			\
	OUTREG32(CPM_CPSPPR, ~CPSPPR_CPSPR_WRITABLE);	\
} while (0)

#define CPM_POWER_ON    1
#define CPM_POWER_OFF   0

/***************************************************************************
 * CPM									   *
 ***************************************************************************/
#define __cpm_get_pllm() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLM_MASK) >> CPPCR0_PLLM_LSB)
#define __cpm_get_plln() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLN_MASK) >> CPPCR0_PLLN_LSB)
#define __cpm_get_pllod() \
	((REG_CPM_CPPCR0 & CPPCR0_PLLOD_MASK) >> CPPCR0_PLLOD_LSB)

#define __cpm_get_pll1m() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1M_MASK) >> CPPCR1_PLL1M_LSB)
#define __cpm_get_pll1n() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1N_MASK) >> CPPCR1_PLL1N_LSB)
#define __cpm_get_pll1od() \
	((REG_CPM_CPPCR1 & CPPCR1_PLL1OD_MASK) >> CPPCR1_PLL1OD_LSB)

#define __cpm_get_cdiv() \
	((REG_CPM_CPCCR & CPCCR_CDIV_MASK) >> CPCCR_CDIV_LSB)
#define __cpm_get_hdiv() \
	((REG_CPM_CPCCR & CPCCR_HDIV_MASK) >> CPCCR_HDIV_LSB)
#define __cpm_get_h2div() \
	((REG_CPM_CPCCR & CPCCR_H2DIV_MASK) >> CPCCR_H2DIV_LSB)
#define __cpm_get_pdiv() \
	((REG_CPM_CPCCR & CPCCR_PDIV_MASK) >> CPCCR_PDIV_LSB)
#define __cpm_get_mdiv() \
	((REG_CPM_CPCCR & CPCCR_MDIV_MASK) >> CPCCR_MDIV_LSB)
#define __cpm_get_sdiv() \
	((REG_CPM_CPCCR & CPCCR_SDIV_MASK) >> CPCCR_SDIV_LSB)
#define __cpm_get_i2sdiv() \
	((REG_CPM_I2SCDR & I2SCDR_I2SDIV_MASK) >> I2SCDR_I2SDIV_LSB)
#define __cpm_get_pixdiv() \
	((REG_CPM_LPCDR & LPCDR_PIXDIV_MASK) >> LPCDR_PIXDIV_LSB)
#define __cpm_get_mscdiv() \
	((REG_CPM_MSCCDR & MSCCDR_MSCDIV_MASK) >> MSCCDR_MSCDIV_LSB)
#define __cpm_get_ssidiv() \
	((REG_CPM_SSICCDR & SSICDR_SSICDIV_MASK) >> SSICDR_SSIDIV_LSB)
#define __cpm_get_pcmdiv() \
	((REG_CPM_PCMCDR & PCMCDR_PCMCD_MASK) >> PCMCDR_PCMCD_LSB)
#define __cpm_get_pll1div() \
	((REG_CPM_CPPCR1 & CPCCR1_P1SDIV_MASK) >> CPCCR1_P1SDIV_LSB)

#define __cpm_set_cdiv(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_CDIV_MASK) | ((v) << (CPCCR_CDIV_LSB)))
#define __cpm_set_hdiv(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_HDIV_MASK) | ((v) << (CPCCR_HDIV_LSB)))
#define __cpm_set_pdiv(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_PDIV_MASK) | ((v) << (CPCCR_PDIV_LSB)))
#define __cpm_set_mdiv(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_MDIV_MASK) | ((v) << (CPCCR_MDIV_LSB)))
#define __cpm_set_h1div(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_H1DIV_MASK) | ((v) << (CPCCR_H1DIV_LSB)))
#define __cpm_set_udiv(v) \
	(REG_CPM_CPCCR = (REG_CPM_CPCCR & ~CPCCR_UDIV_MASK) | ((v) << (CPCCR_UDIV_LSB)))
#define __cpm_set_i2sdiv(v) \
	(REG_CPM_I2SCDR = (REG_CPM_I2SCDR & ~I2SCDR_I2SDIV_MASK) | ((v) << (I2SCDR_I2SDIV_LSB)))
#define __cpm_set_pixdiv(v) \
	(REG_CPM_LPCDR = (REG_CPM_LPCDR & ~LPCDR_PIXDIV_MASK) | ((v) << (LPCDR_PIXDIV_LSB)))
#define __cpm_set_mscdiv(v) \
	(REG_CPM_MSCCDR = (REG_CPM_MSCCDR & ~MSCCDR_MSCDIV_MASK) | ((v) << (MSCCDR_MSCDIV_LSB)))
#define __cpm_set_ssidiv(v) \
	(REG_CPM_SSICDR = (REG_CPM_SSICDR & ~SSICDR_SSIDIV_MASK) | ((v) << (SSICDR_SSIDIV_LSB)))
#define __cpm_set_pcmdiv(v) \
	(REG_CPM_PCMCDR = (REG_CPM_PCMCDR & ~PCMCDR_PCMCD_MASK) | ((v) << (PCMCDR_PCMCD_LSB)))
#define __cpm_set_pll1div(v) \
	(REG_CPM_CPPCR1 = (REG_CPM_CPPCR1 & ~CPCCR1_P1SDIV_MASK) | ((v) << (CPCCR1_P1SDIV_LSB)))

#define __cpm_select_i2sclk_pll1() 	(REG_CPM_I2SCDR |= I2SCDR_I2PCS)
#define __cpm_select_i2sclk_pll0()	(REG_CPM_I2SCDR &= ~I2SCDR_I2PCS)
#define __cpm_select_otgclk_pll1() 	(REG_CPM_USBCDR |= USBCDR_UPCS)
#define __cpm_select_otgclk_pll0()	(REG_CPM_USBCDR &= ~USBCDR_UPCS)
#define __cpm_select_lcdpclk_pll1() 	(REG_CPM_LPCDR |= LPCDR_LPCS)
#define __cpm_select_lcdpclk_pll0()	(REG_CPM_LPCDR &= ~LPCDR_LPCS)
#define __cpm_select_uhcclk_pll1() 	(REG_CPM_UHCCDR |= UHCCDR_UHPCS)
#define __cpm_select_uhcclk_pll0()	(REG_CPM_UHCCDR &= ~UHCCDR_UHPCS)
#define __cpm_select_gpsclk_pll1() 	(REG_CPM_GPSCDR |= GPSCDR_GPCS)
#define __cpm_select_gpsclk_pll0()	(REG_CPM_GPSCDR &= ~GPSCDR_GPCS)
#define __cpm_select_pcmclk_pll1() 	(REG_CPM_PCMCDR |= PCMCDR_PCMPCS)
#define __cpm_select_pcmclk_pll0()	(REG_CPM_PCMCDR &= ~PCMCDR_PCMPCS)
#define __cpm_select_gpuclk_pll1() 	(REG_CPM_GPUCDR |= GPUCDR_GPCS)
#define __cpm_select_gpuclk_pll0()	(REG_CPM_GPUCDR &= ~GPUCDR_GPCS)
#define __cpm_select_clk_pll1() 	(REG_CPM_CDR |= CDR_PCS)
#define __cpm_select_clk_pll0()		(REG_CPM_CDR &= ~CDR_PCS)


#define __cpm_select_pcmclk_pll() 	(REG_CPM_PCMCDR |= PCMCDR_PCMS)
#define __cpm_select_pcmclk_exclk() 	(REG_CPM_PCMCDR &= ~PCMCDR_PCMS)
#define __cpm_select_pixclk_ext()	(REG_CPM_LPCDR |= LPCDR_LPCS)
#define __cpm_select_pixclk_pll()	(REG_CPM_LPCDR &= ~LPCDR_LPCS)
#define __cpm_select_tveclk_exclk()	(REG_CPM_LPCDR |= CPCCR_LSCS)
#define __cpm_select_tveclk_pll()	(REG_CPM_LPCDR &= ~LPCDR_LSCS)
#define __cpm_select_pixclk_lcd()	(REG_CPM_LPCDR &= ~LPCDR_LTCS)
#define __cpm_select_pixclk_tve()	(REG_CPM_LPCDR |= LPCDR_LTCS)
#define __cpm_select_i2sclk_exclk()	(REG_CPM_I2SCDR &= ~I2SCDR_I2CS)
#define __cpm_select_i2sclk_pll()	(REG_CPM_I2SCDR |= I2SCDR_I2CS)
//#define __cpm_select_usbclk_exclk()	(REG_CPM_CPCCR &= ~CPCCR_UCS)
//#define __cpm_select_usbclk_pll()	(REG_CPM_CPCCR |= CPCCR_UCS)

#define __cpm_enable_cko()
#define __cpm_exclk_direct()		(REG_CPM_CPCCR &= ~CPCCR_ECS)
#define __cpm_exclk_div2()             	(REG_CPM_CPCCR |= CPCCR_ECS)
#define __cpm_enable_pll_change()	(REG_CPM_CPCCR |= CPCCR_CE)

#define __cpm_pllout_div2()		(REG_CPM_CPCCR &= ~CPCCR_PCS)
#define __cpm_pll_enable()		(REG_CPM_CPPCR0 |= CPPCR0_PLLEN)

#define __cpm_pll1_enable()		(REG_CPM_CPPCR1 |= CPPCR1_PLL1EN)

#define __cpm_pll_is_off()		(REG_CPM_CPPSR & CPPSR_PLLOFF)
#define __cpm_pll_is_on()		(REG_CPM_CPPSR & CPPSR_PLLON)
#define __cpm_pll_bypass()		(REG_CPM_CPPSR |= CPPSR_PLLBP)

#define __cpm_get_cclk_doze_duty() \
	((REG_CPM_LCR & LCR_DOZE_DUTY_MASK) >> LCR_DOZE_DUTY_LSB)
#define __cpm_set_cclk_doze_duty(v) \
	(REG_CPM_LCR = (REG_CPM_LCR & ~LCR_DOZE_DUTY_MASK) | ((v) << (LCR_DOZE_DUTY_LSB)))

#define __cpm_doze_mode()		(REG_CPM_LCR |= LCR_DOZE_ON)
#define __cpm_idle_mode() \
	(REG_CPM_LCR = (REG_CPM_LCR & ~LCR_LPM_MASK) | LCR_LPM_IDLE)
#define __cpm_sleep_mode() \
	(REG_CPM_LCR = (REG_CPM_LCR & ~LCR_LPM_MASK) | LCR_LPM_SLEEP)

#define __cpm_stop_all() 	\
	do {\
		(REG_CPM_CLKGR0 = 0xffffffff);\
		(REG_CPM_CLKGR1 = 0x3ff);\
	}while(0)
#define __cpm_stop_emc()	(REG_CPM_CLKGR0 |= CLKGR0_EMC)
#define __cpm_stop_ddr()	(REG_CPM_CLKGR0 |= CLKGR0_DDR)
#define __cpm_stop_ipu()	(REG_CPM_CLKGR0 |= CLKGR0_IPU)
#define __cpm_stop_lcd()	(REG_CPM_CLKGR0 |= CLKGR0_LCD)
#define __cpm_stop_tve()	(REG_CPM_CLKGR0 |= CLKGR0_TVE)
#define __cpm_stop_Cim()	(REG_CPM_CLKGR0 |= CLKGR0_CIM)
#define __cpm_stop_mdma()	(REG_CPM_CLKGR0 |= CLKGR0_MDMA)
#define __cpm_stop_uhc()	(REG_CPM_CLKGR0 |= CLKGR0_UHC)
#define __cpm_stop_mac()	(REG_CPM_CLKGR0 |= CLKGR0_MAC)
#define __cpm_stop_gps()	(REG_CPM_CLKGR0 |= CLKGR0_GPS)
#define __cpm_stop_dmac()	(REG_CPM_CLKGR0 |= CLKGR0_DMAC)
#define __cpm_stop_ssi2()	(REG_CPM_CLKGR0 |= CLKGR0_SSI2)
#define __cpm_stop_ssi1()	(REG_CPM_CLKGR0 |= CLKGR0_SSI1)
#define __cpm_stop_uart3()	(REG_CPM_CLKGR0 |= CLKGR0_UART3)
#define __cpm_stop_uart2()	(REG_CPM_CLKGR0 |= CLKGR0_UART2)
#define __cpm_stop_uart1()	(REG_CPM_CLKGR0 |= CLKGR0_UART1)
#define __cpm_stop_uart0()	(REG_CPM_CLKGR0 |= CLKGR0_UART0)
#define __cpm_stop_sadc()	(REG_CPM_CLKGR0 |= CLKGR0_SADC)
#define __cpm_stop_kbc()	(REG_CPM_CLKGR0 |= CLKGR0_KBC)
#define __cpm_stop_msc2()	(REG_CPM_CLKGR0 |= CLKGR0_MSC2)
#define __cpm_stop_msc1()	(REG_CPM_CLKGR0 |= CLKGR0_MSC1)
#define __cpm_stop_owi()	(REG_CPM_CLKGR0 |= CLKGR0_OWI)
#define __cpm_stop_tssi()	(REG_CPM_CLKGR0 |= CLKGR0_TSSI)
#define __cpm_stop_aic()	(REG_CPM_CLKGR0 |= CLKGR0_AIC)
#define __cpm_stop_scc()	(REG_CPM_CLKGR0 |= CLKGR0_SCC)
#define __cpm_stop_i2c1()	(REG_CPM_CLKGR0 |= CLKGR0_I2C1)
#define __cpm_stop_i2c0()	(REG_CPM_CLKGR0 |= CLKGR0_I2C0)
#define __cpm_stop_ssi0()	(REG_CPM_CLKGR0 |= CLKGR0_SSI0)
#define __cpm_stop_msc0()	(REG_CPM_CLKGR0 |= CLKGR0_MSC0)
#define __cpm_stop_otg()	(REG_CPM_CLKGR0 |= CLKGR0_OTG)
#define __cpm_stop_bch()	(REG_CPM_CLKGR0 |= CLKGR0_BCH)
#define __cpm_stop_nemc()	(REG_CPM_CLKGR0 |= CLKGR0_NEMC)
#define __cpm_stop_gpu()	(REG_CPM_CLKGR1 |= CLKGR1_GPU)
#define __cpm_stop_pcm()	(REG_CPM_CLKGR1 |= CLKGR1_PCM)
#define __cpm_stop_ahb1()	(REG_CPM_CLKGR1 |= CLKGR1_AHB1)
#define __cpm_stop_cabac()	(REG_CPM_CLKGR1 |= CLKGR1_CABAC)
#define __cpm_stop_sram()	(REG_CPM_CLKGR1 |= CLKGR1_SRAM)
#define __cpm_stop_dct()	(REG_CPM_CLKGR1 |= CLKGR1_DCT)
#define __cpm_stop_me()		(REG_CPM_CLKGR1 |= CLKGR1_ME)
#define __cpm_stop_dblk()	(REG_CPM_CLKGR1 |= CLKGR1_DBLK)
#define __cpm_stop_mc()		(REG_CPM_CLKGR1 |= CLKGR1_MC)
#define __cpm_stop_bdma()	(REG_CPM_CLKGR1 |= CLKGR1_BDMA)

#define __cpm_start_all() 	\
	do {\
		REG_CPM_CLKGR0 = 0x0;\
		REG_CPM_CLKGR1 = 0x0;\
	} while(0)
#define __cpm_start_emc()	(REG_CPM_CLKGR0 &= ~CLKGR0_EMC)
#define __cpm_start_ddr()	(REG_CPM_CLKGR0 &= ~CLKGR0_DDR)
#define __cpm_start_ipu()	(REG_CPM_CLKGR0 &= ~CLKGR0_IPU)
#define __cpm_start_lcd()	(REG_CPM_CLKGR0 &= ~CLKGR0_LCD)
#define __cpm_start_tve()	(REG_CPM_CLKGR0 &= ~CLKGR0_TVE)
#define __cpm_start_Cim()	(REG_CPM_CLKGR0 &= ~CLKGR0_CIM)
#define __cpm_start_mdma()	(REG_CPM_CLKGR0 &= ~CLKGR0_MDMA)
#define __cpm_start_uhc()	(REG_CPM_CLKGR0 &= ~CLKGR0_UHC)
#define __cpm_start_mac()	(REG_CPM_CLKGR0 &= ~CLKGR0_MAC)
#define __cpm_start_gps()	(REG_CPM_CLKGR0 &= ~CLKGR0_GPS)
#define __cpm_start_dmac()	(REG_CPM_CLKGR0 &= ~CLKGR0_DMAC)
#define __cpm_start_ssi2()	(REG_CPM_CLKGR0 &= ~CLKGR0_SSI2)
#define __cpm_start_ssi1()	(REG_CPM_CLKGR0 &= ~CLKGR0_SSI1)
#define __cpm_start_uart3()	(REG_CPM_CLKGR0 &= ~CLKGR0_UART3)
#define __cpm_start_uart2()	(REG_CPM_CLKGR0 &= ~CLKGR0_UART2)
#define __cpm_start_uart1()	(REG_CPM_CLKGR0 &= ~CLKGR0_UART1)
#define __cpm_start_uart0()	(REG_CPM_CLKGR0 &= ~CLKGR0_UART0)
#define __cpm_start_sadc()	(REG_CPM_CLKGR0 &= ~CLKGR0_SADC)
#define __cpm_start_kbc()	(REG_CPM_CLKGR0 &= ~CLKGR0_KBC)
#define __cpm_start_msc2()	(REG_CPM_CLKGR0 &= ~CLKGR0_MSC2)
#define __cpm_start_msc1()	(REG_CPM_CLKGR0 &= ~CLKGR0_MSC1)
#define __cpm_start_owi()	(REG_CPM_CLKGR0 &= ~CLKGR0_OWI)
#define __cpm_start_tssi()	(REG_CPM_CLKGR0 &= ~CLKGR0_TSSI)
#define __cpm_start_aic()	(REG_CPM_CLKGR0 &= ~CLKGR0_AIC)
#define __cpm_start_scc()	(REG_CPM_CLKGR0 &= ~CLKGR0_SCC)
#define __cpm_start_i2c1()	(REG_CPM_CLKGR0 &= ~CLKGR0_I2C1)
#define __cpm_start_i2c0()	(REG_CPM_CLKGR0 &= ~CLKGR0_I2C0)
#define __cpm_start_ssi0()	(REG_CPM_CLKGR0 &= ~CLKGR0_SSI0)
#define __cpm_start_msc0()	(REG_CPM_CLKGR0 &= ~CLKGR0_MSC0)
#define __cpm_start_otg()	(REG_CPM_CLKGR0 &= ~CLKGR0_OTG)
#define __cpm_start_bch()	(REG_CPM_CLKGR0 &= ~CLKGR0_BCH)
#define __cpm_start_nemc()	(REG_CPM_CLKGR0 &= ~CLKGR0_NEMC)
#define __cpm_start_gpu()	(REG_CPM_CLKGR1 &= ~CLKGR1_GPU)
#define __cpm_start_pcm()	(REG_CPM_CLKGR1 &= ~CLKGR1_PCM)
#define __cpm_start_ahb1()	(REG_CPM_CLKGR1 &= ~CLKGR1_AHB1)
#define __cpm_start_cabac()	(REG_CPM_CLKGR1 &= ~CLKGR1_CABAC)
#define __cpm_start_sram()	(REG_CPM_CLKGR1 &= ~CLKGR1_SRAM)
#define __cpm_start_dct()	(REG_CPM_CLKGR1 &= ~CLKGR1_DCT)
#define __cpm_start_me()	(REG_CPM_CLKGR1 &= ~CLKGR1_ME)
#define __cpm_start_dblk()	(REG_CPM_CLKGR1 &= ~CLKGR1_DBLK)
#define __cpm_start_mc()	(REG_CPM_CLKGR1 &= ~CLKGR1_MC)
#define __cpm_start_bdma()	(REG_CPM_CLKGR1 &= ~CLKGR1_BDMA)

#define __cpm_get_o1st() \
	((REG_CPM_OPCR & OPCR_O1ST_MASK) >> OPCR_O1ST_LSB)
#define __cpm_set_o1st(v) \
	(REG_CPM_OPCR = (REG_CPM_OPCR & ~OPCR_O1ST_MASK) | ((v) << (OPCR_O1ST_LSB)))
#define __cpm_suspend_otgphy()		(REG_CPM_OPCR &= ~OPCR_OTGPHY_ENABLE)
#define __cpm_resume_otgphy()		(REG_CPM_OPCR |= OPCR_OTGPHY_ENABLE)
#define __cpm_enable_osc_in_sleep()	(REG_CPM_OPCR |= OPCR_OSC_ENABLE)
#define __cpm_select_rtcclk_rtc()	(REG_CPM_OPCR |= OPCR_ERCS)
#define __cpm_select_rtcclk_exclk()	(REG_CPM_OPCR &= ~OPCR_ERCS)

#ifdef CFG_EXTAL
#define JZ_EXTAL		CFG_EXTAL
#else
#define JZ_EXTAL		12000000
#endif
#define JZ_EXTAL2		32768 /* RTC clock */

/* PLL output frequency */
static __inline__ unsigned int __cpm_get_pllout(void)
{
	unsigned long m, n, no, pllout;
	unsigned long cppcr = REG_CPM_CPPCR0;
	unsigned long od[4] = {1, 2, 4, 8};
	if ((cppcr & CPPCR0_PLLEN) && (!(cppcr & CPPCR0_PLLBP))) {
		m = __cpm_get_pllm() * 2;
		n = __cpm_get_plln();
		no = od[__cpm_get_pllod()];
		pllout = ((JZ_EXTAL) * m / (n * no));
	} else
		pllout = JZ_EXTAL;
	return pllout;
}

/* PLL output frequency */
static __inline__ unsigned int __cpm_get_pll1out(void)
{
	unsigned long m, n, no, pllout;
	unsigned long cppcr1 = REG_CPM_CPPCR1;
	unsigned long od[4] = {1, 2, 4, 8};
	if (cppcr1 & CPPCR1_PLL1EN)
	{
		m = __cpm_get_pll1m() * 2;
		n = __cpm_get_pll1n();
		no = od[__cpm_get_pll1od()];
		if (cppcr1 & CPPCR1_P1SCS)
			pllout = ((__cpm_get_pllout()) * m / (n * no));
		else
			pllout = ((JZ_EXTAL) * m / (n * no));

	} else
		pllout = JZ_EXTAL;
	return pllout;
}

/* PLL output frequency for MSC/I2S/LCD/USB */
static __inline__ unsigned int __cpm_get_pllout2(void)
{
	if (REG_CPM_CPCCR & CPCCR_PCS)
		return __cpm_get_pllout();
	else
		return __cpm_get_pllout()/2;
}

/* CPU core clock */
static __inline__ unsigned int __cpm_get_cclk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8};

	return __cpm_get_pllout() / div[__cpm_get_cdiv()];
}

/* AHB system bus clock */
static __inline__ unsigned int __cpm_get_hclk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8};

	return __cpm_get_pllout() / div[__cpm_get_hdiv()];
}

/* Memory bus clock */
static __inline__ unsigned int __cpm_get_mclk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8};

	return __cpm_get_pllout() / div[__cpm_get_mdiv()];
}

/* APB peripheral bus clock */
static __inline__ unsigned int __cpm_get_pclk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8};

	return __cpm_get_pllout() / div[__cpm_get_pdiv()];
}

/* AHB1 module clock */
static __inline__ unsigned int __cpm_get_h2clk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8};

	return __cpm_get_pllout() / div[__cpm_get_h2div()];
}

/* LCD pixel clock */
static __inline__ unsigned int __cpm_get_pixclk(void)
{
	return __cpm_get_pllout2() / (__cpm_get_pixdiv() + 1);
}

/* I2S clock */
static __inline__ unsigned int __cpm_get_i2sclk(void)
{
	if (REG_CPM_I2SCDR & I2SCDR_I2CS) {
		return __cpm_get_pllout2() / (__cpm_get_i2sdiv() + 1);
	}
	else {
		return JZ_EXTAL;
	}
}

/* USB clock */
/*
static __inline__ unsigned int __cpm_get_usbclk(void)
{
	if (REG_CPM_CPCCR & CPCCR_UCS) {
		return __cpm_get_pllout2() / (__cpm_get_udiv() + 1);
	}
	else {
		return JZ_EXTAL;
	}
}
*/
/* EXTAL clock for UART,I2C,SSI,TCU,USB-PHY */
static __inline__ unsigned int __cpm_get_extalclk(void)
{
	return JZ_EXTAL;
}

/* RTC clock for CPM,INTC,RTC,TCU,WDT */
static __inline__ unsigned int __cpm_get_rtcclk(void)
{
	return JZ_EXTAL2;
}

/*
 * Output 24MHz for SD and 16MHz for MMC.
 */
static inline void __cpm_select_msc_clk(int sd)
{
	unsigned int pllout2 = __cpm_get_pllout2();
	unsigned int div = 0;

	if (sd) {
		div = pllout2 / 24000000;
	}
	else {
		div = pllout2 / 16000000;
	}

	REG_CPM_MSCCDR = (div - 1)|(1<<31);
	REG_CPM_CPCCR |= CPCCR_CE;
}

#endif /* __MIPS_ASSEMBLER */

#define	DDRC_BASE	0xB3020000

/*************************************************************************
 * DDRC (DDR Controller)
 *************************************************************************/
#define DDRC_ST			(DDRC_BASE + 0x0) /* DDR Status Register */
#define DDRC_CFG		(DDRC_BASE + 0x4) /* DDR Configure Register */
#define DDRC_CTRL		(DDRC_BASE + 0x8) /* DDR Control Register */
#define DDRC_LMR		(DDRC_BASE + 0xc) /* DDR Load-Mode-Register */
#define DDRC_TIMING1		(DDRC_BASE + 0x10) /* DDR Timing Config Register 1 */
#define DDRC_TIMING2		(DDRC_BASE + 0x14) /* DDR Timing Config Register 2 */
#define DDRC_REFCNT		(DDRC_BASE + 0x18) /* DDR  Auto-Refresh Counter */
#define DDRC_DQS		(DDRC_BASE + 0x1c) /* DDR DQS Delay Control Register */
#define DDRC_DQS_ADJ		(DDRC_BASE + 0x20) /* DDR DQS Delay Adjust Register */
#define DDRC_MMAP0		(DDRC_BASE + 0x24) /* DDR Memory Map Config Register */
#define DDRC_MMAP1		(DDRC_BASE + 0x28) /* DDR Memory Map Config Register */
#define DDRC_DDELAYCTRL1	(DDRC_BASE + 0x2c)
#define DDRC_DDELAYCTRL2	(DDRC_BASE + 0x30)
#define DDRC_DSTRB		(DDRC_BASE + 0x34)
#define DDRC_PMEMBS0		(DDRC_BASE + 0x50)
#define DDRC_PMEMBS1		(DDRC_BASE + 0x54)
#define DDRC_PMEMOSEL		(DDRC_BASE + 0x58)
#define DDRC_PMEMOEN		(DDRC_BASE + 0x5c)

/* DDRC Register */
#define REG_DDRC_ST		REG32(DDRC_ST)
#define REG_DDRC_CFG		REG32(DDRC_CFG)
#define REG_DDRC_CTRL		REG32(DDRC_CTRL)
#define REG_DDRC_LMR		REG32(DDRC_LMR)
#define REG_DDRC_TIMING1	REG32(DDRC_TIMING1)
#define REG_DDRC_TIMING2	REG32(DDRC_TIMING2)
#define REG_DDRC_REFCNT		REG32(DDRC_REFCNT)
#define REG_DDRC_DQS		REG32(DDRC_DQS)
#define REG_DDRC_DQS_ADJ	REG32(DDRC_DQS_ADJ)
#define REG_DDRC_MMAP0		REG32(DDRC_MMAP0)
#define REG_DDRC_MMAP1		REG32(DDRC_MMAP1)
#define REG_DDRC_DDELAYCTRL1	REG32(DDRC_DDELAYCTRL1)
#define REG_DDRC_DDELAYCTRL2	REG32(DDRC_DDELAYCTRL2)
#define REG_DDRC_DSTRB		REG32(DDRC_DSTRB)
#define REG_DDRC_PMEMBS0	REG32(DDRC_PMEMBS0)
#define REG_DDRC_PMEMBS1	REG32(DDRC_PMEMBS1)
#define REG_DDRC_PMEMOSEL	REG32(DDRC_PMEMOSEL)
#define REG_DDRC_PMEMOEN	REG32(DDRC_PMEMOEN)

/* DDRC Status Register */
#define DDRC_ST_ENDIAN		(1 << 7) /* 0 Little data endian
					    1 Big data endian */
#define DDRC_ST_MISS		(1 << 6) 

#define DDRC_ST_DPDN		(1 << 5) /* 0 DDR memory is NOT in deep-power-down state
					    1 DDR memory is in deep-power-down state */
#define DDRC_ST_PDN		(1 << 4) /* 0 DDR memory is NOT in power-down state
					    1 DDR memory is in power-down state */
#define DDRC_ST_AREF		(1 << 3) /* 0 DDR memory is NOT in auto-refresh state
					    1 DDR memory is in auto-refresh state */
#define DDRC_ST_SREF		(1 << 2) /* 0 DDR memory is NOT in self-refresh state
					    1 DDR memory is in self-refresh state */
#define DDRC_ST_CKE1		(1 << 1) /* 0 CKE1 Pin is low
					    1 CKE1 Pin is high */
#define DDRC_ST_CKE0		(1 << 0) /* 0 CKE0 Pin is low
					    1 CKE0 Pin is high */

/* DDRC Configure Register */
#define DDRC_CFG_RDPRI		(1 << 29)
#define DDRC_CFG_ROW1_BIT	27 /* Row Address width. */
#define DDRC_CFG_COL1_BIT	25 /* Row Address width. */
#define DDRC_CFG_BA1		(1 << 24)
#define DDRC_CFG_IMBA		(1 << 23)
#define DDRC_CFG_DQSMD		(1 << 22)
#define DDRC_CFG_BTRUN		(1 << 21)

#define DDRC_CFG_MISPE		(1 << 15)

#define DDRC_CFG_TYPE_BIT	12
#define DDRC_CFG_TYPE_MASK	(0x7 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR1	(2 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_MDDR	(3 << DDRC_CFG_TYPE_BIT)
#define DDRC_CFG_TYPE_DDR2	(4 << DDRC_CFG_TYPE_BIT)

#define DDRC_CFG_ROW_BIT	10 /* Row Address width. */
#define DDRC_CFG_ROW_MASK	(0x3 << DDRC_CFG_ROW_BIT)
#define DDRC_CFG_ROW_12		(0 << DDRC_CFG_ROW_BIT) /* 12-bit row address is used */
#define DDRC_CFG_ROW_13		(1 << DDRC_CFG_ROW_BIT) /* 13-bit row address is used */
#define DDRC_CFG_ROW_14		(2 << DDRC_CFG_ROW_BIT) /* 14-bit row address is used */

#define DDRC_CFG_COL_BIT	8 /* Column Address width.
				     Specify the Column address width of external DDR. */
#define DDRC_CFG_COL_MASK	(0x3 << DDRC_CFG_COL_BIT)
#define DDRC_CFG_COL_8		(0 << DDRC_CFG_COL_BIT) /* 8-bit Column address is used */
#define DDRC_CFG_COL_9		(1 << DDRC_CFG_COL_BIT) /* 9-bit Column address is used */
#define DDRC_CFG_COL_10		(2 << DDRC_CFG_COL_BIT) /* 10-bit Column address is used */
#define DDRC_CFG_COL_11		(3 << DDRC_CFG_COL_BIT) /* 11-bit Column address is used */

#define DDRC_CFG_CS1EN		(1 << 7) /* 0 DDR Pin CS1 un-used
					    1 There're DDR memory connected to CS1 */
#define DDRC_CFG_CS0EN		(1 << 6) /* 0 DDR Pin CS0 un-used
					    1 There're DDR memory connected to CS0 */

#define DDRC_CFG_CL_BIT		2 /* CAS Latency */
#define DDRC_CFG_CL_MASK	(0xf << DDRC_CFG_CL_BIT)
#define DDRC_CFG_CL_3		(0x0a << DDRC_CFG_CL_BIT) /* CL = 3 tCK */
#define DDRC_CFG_CL_4		(0x0b << DDRC_CFG_CL_BIT) /* CL = 4 tCK */
#define DDRC_CFG_CL_5		(0x0c << DDRC_CFG_CL_BIT) /* CL = 5 tCK */
#define DDRC_CFG_CL_6		(0x0d << DDRC_CFG_CL_BIT) /* CL = 6 tCK */
#define DDRC_CFG_CL_7           (0x0e << DDRC_CFG_CL_BIT) /* CL = 7 tCK */

#define DDRC_CFG_BA		(1 << 1) /* 0 4 bank device, Pin ba[1:0] valid, ba[2] un-used
					    1 8 bank device, Pin ba[2:0] valid*/
#define DDRC_CFG_DW		(1 << 0) /*0 External memory data width is 16-bit
					   1 External memory data width is 32-bit */

/* DDRC Control Register */
#define DDRC_CTRL_ACTPD		(1 << 15) /* 0 Precharge all banks before entering power-down
					     1 Do not precharge banks before entering power-down */
#define DDRC_CTRL_PDT_BIT	12 /* Power-Down Timer */
#define DDRC_CTRL_PDT_MASK	(0x7 << DDRC_CTRL_PDT_BIT)
#define DDRC_CTRL_PDT_DIS	(0 << DDRC_CTRL_PDT_BIT) /* power-down disabled */
#define DDRC_CTRL_PDT_8		(1 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 8 tCK idle */
#define DDRC_CTRL_PDT_16	(2 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 16 tCK idle */
#define DDRC_CTRL_PDT_32	(3 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 32 tCK idle */
#define DDRC_CTRL_PDT_64	(4 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 64 tCK idle */
#define DDRC_CTRL_PDT_128	(5 << DDRC_CTRL_PDT_BIT) /* Enter power-down after 128 tCK idle */

#define DDRC_CTRL_PRET_BIT	8 /* Precharge Timer */
#define DDRC_CTRL_PRET_MASK	(0x7 << DDRC_CTRL_PRET_BIT) /*  */
  #define DDRC_CTRL_PRET_DIS	(0 << DDRC_CTRL_PRET_BIT) /* PRET function Disabled */
  #define DDRC_CTRL_PRET_8	(1 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 8 tCK idle */
  #define DDRC_CTRL_PRET_16	(2 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 16 tCK idle */
  #define DDRC_CTRL_PRET_32	(3 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 32 tCK idle */
  #define DDRC_CTRL_PRET_64	(4 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 64 tCK idle */
  #define DDRC_CTRL_PRET_128	(5 << DDRC_CTRL_PRET_BIT) /* Precharge active bank after 128 tCK idle */

#define DDRC_CTRL_SR		(1 << 5) /* 1 Drive external DDR device entering self-refresh mode
					    0 Drive external DDR device exiting self-refresh mode */
#define DDRC_CTRL_UNALIGN	(1 << 4) /* 0 Disable unaligned transfer on AXI BUS
					    1 Enable unaligned transfer on AXI BUS */
#define DDRC_CTRL_ALH		(1 << 3) /* Advanced Latency Hiding:
					    0 Disable ALH
					    1 Enable ALH */
#define DDRC_CTRL_CKE		(1 << 1) /* 0 Not set CKE Pin High
					    1 Set CKE Pin HIGH */
#define DDRC_CTRL_RESET		(1 << 0) /* 0 End resetting ddrc_controller
					    1 Resetting ddrc_controller */

/* DDRC Load-Mode-Register */
#define DDRC_LMR_DDR_ADDR_BIT	16 /* When performing a DDR command, DDRC_ADDR[13:0]
				      corresponding to external DDR address Pin A[13:0] */
#define DDRC_LMR_DDR_ADDR_MASK	(0x3fff << DDRC_LMR_DDR_ADDR_BIT)

#define DDRC_LMR_BA_BIT		8 /* When performing a DDR command, BA[2:0]
				     corresponding to external DDR address Pin BA[2:0]. */
#define DDRC_LMR_BA_MASK	(0x7 << DDRC_LMR_BA_BIT)
/* For DDR2 */
#define DDRC_LMR_BA_MRS		(0 << DDRC_LMR_BA_BIT) /* Mode Register set */
#define DDRC_LMR_BA_EMRS1	(1 << DDRC_LMR_BA_BIT) /* Extended Mode Register1 set */
#define DDRC_LMR_BA_EMRS2	(2 << DDRC_LMR_BA_BIT) /* Extended Mode Register2 set */
#define DDRC_LMR_BA_EMRS3	(3 << DDRC_LMR_BA_BIT) /* Extended Mode Register3 set */
/* For mobile DDR */
#define DDRC_LMR_BA_M_MRS	(0 << DDRC_LMR_BA_BIT) /* Mode Register set */
#define DDRC_LMR_BA_M_EMRS	(2 << DDRC_LMR_BA_BIT) /* Extended Mode Register set */
#define DDRC_LMR_BA_M_SR	(1 << DDRC_LMR_BA_BIT) /* Status Register set */

#define DDRC_LMR_CMD_BIT	4
#define DDRC_LMR_CMD_MASK	(0x3 << DDRC_LMR_CMD_BIT)
#define DDRC_LMR_CMD_PREC	(0 << DDRC_LMR_CMD_BIT)/* Precharge one bank/All banks */
#define DDRC_LMR_CMD_AUREF	(1 << DDRC_LMR_CMD_BIT)/* Auto-Refresh */
#define DDRC_LMR_CMD_LMR	(2 << DDRC_LMR_CMD_BIT)/* Load Mode Register */

#define DDRC_LMR_START		(1 << 0) /* 0 No command is performed
					    1 On the posedge of START, perform a command
					    defined by CMD field */
/* DDRC Mode Register Set */
#define DDR_MRS_PD_BIT		(1 << 10) /* Active power down exit time */
#define DDR_MRS_PD_MASK		(1 << DDR_MRS_PD_BIT) 
#define DDR_MRS_PD_FAST_EXIT	(0 << 10)
#define DDR_MRS_PD_SLOW_EXIT	(1 << 10)
#define DDR_MRS_WR_BIT		(1 << 9) /* Write Recovery for autoprecharge */
#define DDR_MRS_WR_MASK		(7 << DDR_MRS_WR_BIT)
#define DDR_MRS_DLL_RST		(1 << 8) /* DLL Reset */
#define DDR_MRS_TM_BIT		7        /* Operating Mode */
#define DDR_MRS_TM_MASK		(1 << DDR_MRS_OM_BIT) 
#define DDR_MRS_TM_NORMAL	(0 << DDR_MRS_OM_BIT)
#define DDR_MRS_TM_TEST	(1 << DDR_MRS_OM_BIT)
#define DDR_MRS_CAS_BIT		4        /* CAS Latency */
#define DDR_MRS_CAS_MASK	(7 << DDR_MRS_CAS_BIT)
#define DDR_MRS_BT_BIT		3        /* Burst Type */
#define DDR_MRS_BT_MASK		(1 << DDR_MRS_BT_BIT)
#define DDR_MRS_BT_SEQ	(0 << DDR_MRS_BT_BIT) /* Sequential */
#define DDR_MRS_BT_INT	(1 << DDR_MRS_BT_BIT) /* Interleave */
#define DDR_MRS_BL_BIT		0        /* Burst Length */
#define DDR_MRS_BL_MASK		(7 << DDR_MRS_BL_BIT)
#define DDR_MRS_BL_4		(2 << DDR_MRS_BL_BIT)
#define DDR_MRS_BL_8		(3 << DDR_MRS_BL_BIT)

/* DDRC Extended Mode Register1 Set */
#define DDR_EMRS1_QOFF		(1<<12) /* 0 Output buffer enabled
					   1 Output buffer disabled */
#define DDR_EMRS1_RDQS_EN	(1<<11) /* 0 Disable
					   1 Enable */
#define DDR_EMRS1_DQS_DIS	(1<<10) /* 0 Enable
					   1 Disable */
#define DDR_EMRS1_OCD_BIT	7 /* Additive Latency 0 -> 6 */
#define DDR_EMRS1_OCD_MASK	(0x7 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_OCD_EXIT		(0 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_OCD_D0		(1 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_OCD_D1		(2 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_OCD_ADJ		(4 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_OCD_DFLT		(7 << DDR_EMRS1_OCD_BIT)
#define DDR_EMRS1_AL_BIT	3 /* Additive Latency 0 -> 6 */
#define DDR_EMRS1_AL_MASK	(7 << DDR_EMRS1_AL_BIT)
#define DDR_EMRS1_RTT_BIT	2 /*  */
#define DDR_EMRS1_RTT_MASK	(0x11 << DDR_EMRS1_DIC_BIT) /* Bit 6, Bit 2 */
#define DDR_EMRS1_DIC_BIT	1        /* Output Driver Impedence Control */
#define DDR_EMRS1_DIC_MASK	(1 << DDR_EMRS1_DIC_BIT) /* 100% */
#define DDR_EMRS1_DIC_NORMAL	(0 << DDR_EMRS1_DIC_BIT) /* 60% */
#define DDR_EMRS1_DIC_HALF	(1 << DDR_EMRS1_DIC_BIT)
#define DDR_EMRS1_DLL_BIT	0        /* DLL Enable  */
#define DDR_EMRS1_DLL_MASK	(1 << DDR_EMRS1_DLL_BIT)
#define DDR_EMRS1_DLL_EN	(0 << DDR_EMRS1_DLL_BIT)
#define DDR_EMRS1_DLL_DIS	(1 << DDR_EMRS1_DLL_BIT)

/* Mobile SDRAM Extended Mode Register */
#define DDR_EMRS_DS_BIT		5	/* Driver strength */
#define DDR_EMRS_DS_MASK	(7 << DDR_EMRS_DS_BIT)
#define DDR_EMRS_DS_FULL	(0 << DDR_EMRS_DS_BIT)	/*Full*/
#define DDR_EMRS_DS_HALF	(1 << DDR_EMRS_DS_BIT)	/*1/2 Strength*/
#define DDR_EMRS_DS_QUTR	(2 << DDR_EMRS_DS_BIT)	/*1/4 Strength*/
#define DDR_EMRS_DS_OCTANT	(3 << DDR_EMRS_DS_BIT)	/*1/8 Strength*/
#define DDR_EMRS_DS_QUTR3	(4 << DDR_EMRS_DS_BIT)	/*3/4 Strength*/

#define DDR_EMRS_PRSR_BIT	0	/* Partial Array Self Refresh */
#define DDR_EMRS_PRSR_MASK	(7 << DDR_EMRS_PRSR_BIT)
#define DDR_EMRS_PRSR_ALL	(0 << DDR_EMRS_PRSR_BIT) /*All Banks*/
#define DDR_EMRS_PRSR_HALF_TL	(1 << DDR_EMRS_PRSR_BIT) /*Half of Total Bank*/
#define DDR_EMRS_PRSR_QUTR_TL	(2 << DDR_EMRS_PRSR_BIT) /*Quarter of Total Bank*/
#define DDR_EMRS_PRSR_HALF_B0	(5 << DDR_EMRS_PRSR_BIT) /*Half of Bank0*/
#define DDR_EMRS_PRSR_QUTR_B0	(6 << DDR_EMRS_PRSR_BIT) /*Quarter of Bank0*/

/* DDRC Timing Config Register 1 */
#define DDRC_TIMING1_TRAS_BIT 	28 /* ACTIVE to PRECHARGE command period (2 * tRAS + 1) */
#define DDRC_TIMING1_TRAS_MASK 	(0xf << DDRC_TIMING1_TRAS_BIT)

#define DDRC_TIMING1_TRTP_BIT		24 /* READ to PRECHARGE command period. */
#define DDRC_TIMING1_TRTP_MASK	(0x3 << DDRC_TIMING1_TRTP_BIT)

#define DDRC_TIMING1_TRP_BIT		20 /* PRECHARGE command period. */
#define DDRC_TIMING1_TRP_MASK 	(0x7 << DDRC_TIMING1_TRP_BIT)

#define DDRC_TIMING1_TRCD_BIT		16 /* ACTIVE to READ or WRITE command period. */
#define DDRC_TIMING1_TRCD_MASK	(0x7 << DDRC_TIMING1_TRCD_BIT)

#define DDRC_TIMING1_TRC_BIT 		12 /* ACTIVE to ACTIVE command period. */
#define DDRC_TIMING1_TRC_MASK 	(0xf << DDRC_TIMING1_TRC_BIT)

#define DDRC_TIMING1_TRRD_BIT		8 /* ACTIVE bank A to ACTIVE bank B command period. */
#define DDRC_TIMING1_TRRD_MASK	(0x3 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_DISABLE	(0 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_2		(1 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_3		(2 << DDRC_TIMING1_TRRD_BIT)
#define DDRC_TIMING1_TRRD_4		(3 << DDRC_TIMING1_TRRD_BIT)

#define DDRC_TIMING1_TWR_BIT 		4 /* WRITE Recovery Time defined by register MR of DDR2 memory */
#define DDRC_TIMING1_TWR_MASK		(0x7 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_1		(0 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_2		(1 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_3		(2 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_4		(3 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_5		(4 << DDRC_TIMING1_TWR_BIT)
#define DDRC_TIMING1_TWR_6		(5 << DDRC_TIMING1_TWR_BIT)

#define DDRC_TIMING1_TWTR_BIT		0 /* WRITE to READ command delay. */
#define DDRC_TIMING1_TWTR_MASK	(0x3 << DDRC_TIMING1_TWTR_BIT)
#define DDRC_TIMING1_TWTR_1		(0 << DDRC_TIMING1_TWTR_BIT)
#define DDRC_TIMING1_TWTR_2		(1 << DDRC_TIMING1_TWTR_BIT)
#define DDRC_TIMING1_TWTR_3		(2 << DDRC_TIMING1_TWTR_BIT)
#define DDRC_TIMING1_TWTR_4		(3 << DDRC_TIMING1_TWTR_BIT)

/* DDRC Timing Config Register 2 */
#define DDRC_TIMING2_TRFC_BIT         24 /* AUTO-REFRESH command period. */
#define DDRC_TIMING2_TRFC_MASK        (0xf << DDRC_TIMING2_TRFC_BIT)
#define DDRC_TIMING2_RWCOV_BIT        19 /* Equal to Tsel of MDELAY. */
#define DDRC_TIMING2_RWCOV_MASK       (0x3 << DDRC_TIMING2_RWCOV_BIT)
#define DDRC_TIMING2_TCKE_BIT         16
#define DDRC_TIMING2_TCKE_MASK        (0x7 << DDRC_TIMING2_TCKE_BIT)
#define DDRC_TIMING2_TMINSR_BIT       8  /* Minimum Self-Refresh / Deep-Power-Down time */
#define DDRC_TIMING2_TMINSR_MASK      (0xf << DDRC_TIMING2_TMINSR_BIT)
#define DDRC_TIMING2_TXP_BIT          4  /* EXIT-POWER-DOWN to next valid command period. */
#define DDRC_TIMING2_TXP_MASK         (0x7 << DDRC_TIMING2_TXP_BIT)
#define DDRC_TIMING2_TMRD_BIT         0  /* Load-Mode-Register to next valid command period. */
#define DDRC_TIMING2_TMRD_MASK        (0x3 << DDRC_TIMING2_TMRD_BIT)

/* DDRC  Auto-Refresh Counter */
#define DDRC_REFCNT_CON_BIT           16 /* Constant value used to compare with CNT value. */
#define DDRC_REFCNT_CON_MASK          (0xff << DDRC_REFCNT_CON_BIT)
#define DDRC_REFCNT_CNT_BIT           8  /* 8-bit counter */
#define DDRC_REFCNT_CNT_MASK          (0xff << DDRC_REFCNT_CNT_BIT)
#define DDRC_REFCNT_CLKDIV_BIT        1  /* Clock Divider for auto-refresh counter. */
#define DDRC_REFCNT_CLKDIV_MASK       (0x7 << DDRC_REFCNT_CLKDIV_BIT)
#define DDRC_REFCNT_REF_EN            (1 << 0) /* Enable Refresh Counter */

/* DDRC DQS Delay Control Register */
#define DDRC_DQS_ERROR                (1 << 29) /* ahb_clk Delay Detect ERROR, read-only. */
#define DDRC_DQS_READY                (1 << 28) /* ahb_clk Delay Detect READY, read-only. */
#define DDRC_DQS_SRDET                (1 << 25)
#define DDRC_DQS_DET                  (1 << 24) /* Start delay detecting. */
#define DDRC_DQS_AUTO                 (1 << 23) /* Hardware auto-detect & set delay line */
#define DDRC_DQS_CLKD_BIT             16 /* CLKD is reference value for setting WDQS and RDQS.*/
#define DDRC_DQS_CLKD_MASK            (0x3f << DDRC_DQS_CLKD_BIT) 
#define DDRC_DQS_WDQS_BIT             8  /* Set delay element number to write DQS delay-line. */
#define DDRC_DQS_WDQS_MASK            (0x3f << DDRC_DQS_WDQS_BIT) 
#define DDRC_DQS_RDQS_BIT             0  /* Set delay element number to read DQS delay-line. */
#define DDRC_DQS_RDQS_MASK            (0x3f << DDRC_DQS_RDQS_BIT) 

/* DDRC DQS Delay Adjust Register */
#define DDRC_DQS_ADJDQSCON_BIT        16
#define DDRC_DQS_ADJDQSCON_MASK       (0xffff << DDRC_DQS_ADJDQSCON_BIT)
#define DDRC_DQS_ADJWSIGN             (1 << 13)
#define DDRC_DQS_ADJWDQS_BIT          8 /* The adjust value for WRITE DQS delay */
#define DDRC_DQS_ADJWDQS_MASK         (0x1f << DDRC_DQS_ADJWDQS_BIT)
#define DDRC_DQS_ADJRSIGN             (1 << 5)
#define DDRC_DQS_ADJRDQS_BIT          0 /* The adjust value for READ DQS delay */
#define DDRC_DQS_ADJRDQS_MASK         (0x1f << DDRC_DQS_ADJRDQS_BIT)

/* DDRC Memory Map Config Register */
#define DDRC_MMAP_BASE_BIT            8 /* base address */
#define DDRC_MMAP_BASE_MASK           (0xff << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP_MASK_BIT            0 /* address mask */
#define DDRC_MMAP_MASK_MASK           (0xff << DDRC_MMAP_MASK_BIT)         

#define DDRC_MMAP0_BASE		     (0x20 << DDRC_MMAP_BASE_BIT)
#define DDRC_MMAP1_BASE_64M	(0x24 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_128M	(0x28 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/
#define DDRC_MMAP1_BASE_256M	(0x30 << DDRC_MMAP_BASE_BIT) /*when bank0 is 128M*/

#define DDRC_MMAP_MASK_64_64	(0xfc << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_128_128	(0xf8 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/
#define DDRC_MMAP_MASK_256_256	(0xf0 << DDRC_MMAP_MASK_BIT)  /*mask for two 128M SDRAM*/

/* DDRC Timing Configure Register 1 */
#define DDRC_DDELAYCTRL1_TSEL_BIT			18
#define DDRC_DDELAYCTRL1_TSEL_MASK			(0x3 << DDRC_DDELAYCTRL1_TSEL_BIT)
#define DDRC_DDELAYCTRL1_MSEL_BIT			16
#define DDRC_DDELAYCTRL1_MSEL_MASK			(0x3 << DDRC_DDELAYCTRL1_MSEL_BIT)
#define DDRC_DDELAYCTRL1_HL				(1 << 15)
#define DDRC_DDELAYCTRL1_QUAR				(1 << 14)
#define DDRC_DDELAYCTRL1_MAUTO				(1 << 6)
#define DDRC_DDELAYCTRL1_MSIGN				(1 << 5)
#define DDRC_DDELAYCTRL1_MASK_DELAY_SEL_ADJ_BIT		0
#define DDRC_DDELAYCTRL1_MASK_DELAY_SEL_ADJ_MASK	(0x1f << DDRC_DDELAYCTRL1_MASK_DELAY_SEL_ADJ_BIT)

/* DDRC Timing Configure Register 2 */
#define DDRC_DDELAYCTRL2_MASK_DELAY_SEL_BIT		0
#define DDRC_DDELAYCTRL2_MASK_DELAY_SEL_MASK		(0x3f << DDRC_DDELAYCTRL2_MASK_DELAY_SEL_BIT)

/* DDRC Multi-media stride Register */
#define DDRC_DSTRB_STRB0_BIT		16
#define DDRC_DSTRB_STRB0_MASK		(0x1fff << DDRC_DSTRB_STRB0_BIT)
#define DDRC_DSTRB_STRB1_BIT		0
#define DDRC_DSTRB_STRB1_MASK		(0x1fff << DDRC_DSTRB_STRB1_BIT)
/* DDRC IO pad control Register */
#define DDRC_PMEMBS0_PDDQS3			(1 << 31)
#define DDRC_PMEMBS0_PDDQS2			(1 << 30)
#define DDRC_PMEMBS0_PDDQS1			(1 << 29)
#define DDRC_PMEMBS0_PDDQS0			(1 << 28)
#define DDRC_PMEMBS0_PDDQ3			(1 << 27)
#define DDRC_PMEMBS0_PDDQ2			(1 << 26)
#define DDRC_PMEMBS0_PDDQ1			(1 << 25)
#define DDRC_PMEMBS0_PDDQ0			(1 << 24)
#define DDRC_PMEMBS0_STDQS3			(1 << 23)
#define DDRC_PMEMBS0_STDQS2			(1 << 22)
#define DDRC_PMEMBS0_STDQS1			(1 << 21)
#define DDRC_PMEMBS0_STDQS0			(1 << 20)
#define DDRC_PMEMBS0_STDQ3			(1 << 19)
#define DDRC_PMEMBS0_STDQ2			(1 << 18)
#define DDRC_PMEMBS0_STDQ1			(1 << 17)
#define DDRC_PMEMBS0_STDQ0			(1 << 16)
#define DDRC_PMEMBS0_PEDQS3			(1 << 15)
#define DDRC_PMEMBS0_PEDQS2 			(1 << 14)
#define DDRC_PMEMBS0_PEDQS1 			(1 << 13)
#define DDRC_PMEMBS0_PEDQS0 			(1 << 12)
#define DDRC_PMEMBS0_PEDQ3			(1 << 11)
#define DDRC_PMEMBS0_PEDQ2			(1 << 10)
#define DDRC_PMEMBS0_PEDQ1			(1 << 9)
#define DDRC_PMEMBS0_PEDQ0			(1 << 8)
#define DDRC_PMEMBS0_PSDQS3			(1 << 7)
#define DDRC_PMEMBS0_PSDQS2			(1 << 6)
#define DDRC_PMEMBS0_PSDQS1			(1 << 5)
#define DDRC_PMEMBS0_PSDQS0			(1 << 4)
#define DDRC_PMEMBS0_PSDQ3			(1 << 3)
#define DDRC_PMEMBS0_PSDQ2			(1 << 2)
#define DDRC_PMEMBS0_PSDQ1			(1 << 1)
#define DDRC_PMEMBS0_PSDQ0			(1 << 0)
/* DDRC IO pad control Register */
#define DDRC_PMEMBS1_IENDQS3			(1 << 31)
#define DDRC_PMEMBS1_IENDQS2			(1 << 30)
#define DDRC_PMEMBS1_IENDQS1			(1 << 29)
#define DDRC_PMEMBS1_IENDQS0			(1 << 28)
#define DDRC_PMEMBS1_IENDQ3			(1 << 27)
#define DDRC_PMEMBS1_IENDQ2			(1 << 26)
#define DDRC_PMEMBS1_IENDQ1			(1 << 25)
#define DDRC_PMEMBS1_IENDQ0			(1 << 24)
#define DDRC_PMEMBS1_SSTL			(1 << 16)

#define DDRC_PMEMBS1_SSELDQS3_BIT		14
#define DDRC_PMEMBS1_SSELDQS3_MASK		(0x3 << DDRC_PMEMBS1_SSELDQS3_BIT)

#define DDRC_PMEMBS1_SSELDQS2_BIT		12
#define DDRC_PMEMBS1_SSELDQS2_MASK		(0x3 << DDRC_PMEMBS1_SSELDQS2_BIT)

#define DDRC_PMEMBS1_SSELDQS1_BIT		10
#define DDRC_PMEMBS1_SSELDQS1_MASK		(0x3 << DDRC_PMEMBS1_SSELDQS1_BIT)

#define DDRC_PMEMBS1_SSELDQS0_BIT		8
#define DDRC_PMEMBS1_SSELDQS0_MASK		(0x3 << DDRC_PMEMBS1_SSELDQS0_BIT)

#define DDRC_PMEMBS1_SSELDQ3_BIT		6
#define DDRC_PMEMBS1_SSELDQ3_MASK		(0x3 << DDRC_PMEMBS1_SSELDQ3_BIT)

#define DDRC_PMEMBS1_SSELDQ2_BIT		4
#define DDRC_PMEMBS1_SSELDQ2_MASK		(0x3 << DDRC_PMEMBS1_SSELDQ2_BIT)

#define DDRC_PMEMBS1_SSELDQ1_BIT		2
#define DDRC_PMEMBS1_SSELDQ1_MASK		(0x3 << DDRC_PMEMBS1_SSELDQ1_BIT)

#define DDRC_PMEMBS1_SSELDQ0_BIT		0
#define DDRC_PMEMBS1_SSELDQ0_MASK		(0x3 << DDRC_PMEMBS1_SSELDQ0_BIT)

/* DDRC IO pad control Register */
#define DDRC_PMEMOSEL_CKSSEL_BIT		18
#define DDRC_PMEMOSEL_CKSSEL_MASK		(0x3 << DDRC_PMEMOSEL_CKSSEL_BIT)

#define DDRC_PMEMOSEL_CKESSEL_BIT		16
#define DDRC_PMEMOSEL_CKESSEL_MASK		(0x3 << DDRC_PMEMOSEL_CKESSEL_BIT)

#define DDRC_PMEMOSEL_ADDRSSEL_BIT		14
#define DDRC_PMEMOSEL_ADDRSSEL_MASK		(0x3 << DDRC_PMEMOSEL_ADDRSSEL_BIT)

#define DDRC_PMEMOSEL_DMSSEL3_BIT		12
#define DDRC_PMEMOSEL_DMSSEL3_MASK		(0x3 << DDRC_PMEMOSEL_DMSSEL3_BIT)

#define DDRC_PMEMOSEL_DMSSEL2_BIT		10
#define DDRC_PMEMOSEL_DMSSEL2_MASK		(0x3 << DDRC_PMEMOSEL_DMSSEL2_BIT)

#define DDRC_PMEMOSEL_DMSSEL1_BIT		8
#define DDRC_PMEMOSEL_DMSSEL1_MASK		(0x3 << DDRC_PMEMOSEL_DMSSEL1_BIT)

#define DDRC_PMEMOSEL_DMSSEL0_BIT		6
#define DDRC_PMEMOSEL_DMSSEL0_MASK		(0x3 << DDRC_PMEMOSEL_DMSSEL0_BIT)

#define DDRC_PMEMOSEL_CMDSSEL_BIT		4
#define DDRC_PMEMOSEL_CMDSSEL_MASK		(0x3 << DDRC_PMEMOSEL_CMDSSEL_BIT)

#define DDRC_PMEMOSEL_CSSSEL1_BIT		2
#define DDRC_PMEMOSEL_CSSSEL1_MASK		(0x3 << DDRC_PMEMOSEL_CSSSEL1_BIT)

#define DDRC_PMEMOSEL_CSSSEL0_BIT		0
#define DDRC_PMEMOSEL_CSSSEL0_MASK		(0x3 << DDRC_PMEMOSEL_CSSSEL0_BIT)

/* DDRC IO pad control Register */
#define DDRC_PMEMOEN_CKOEN			(1 << 14)
#define DDRC_PMEMOEN_BAOEN2			(1 << 13)
#define DDRC_PMEMOEN_BAOEN1			(1 << 12)
#define DDRC_PMEMOEN_BAOEN0			(1 << 11)
#define DDRC_PMEMOEN_AOEN13			(1 << 10)
#define DDRC_PMEMOEN_AOEN12			(1 << 9)
#define DDRC_PMEMOEN_AOEN11_0			(1 << 8)
#define DDRC_PMEMOEN_DMOEN3			(1 << 7)
#define DDRC_PMEMOEN_DMOEN2			(1 << 6)
#define DDRC_PMEMOEN_DMOEN1			(1 << 5)
#define DDRC_PMEMOEN_DMOEN0			(1 << 4)
#define DDRC_PMEMOEN_CMDOEN			(1 << 3)
#define DDRC_PMEMOEN_CSOEN1			(1 << 2)
#define DDRC_PMEMOEN_CSOEN0			(1 << 1)
#define DDRC_PMEMOEN_CKEOEN			(1 << 0)

#ifndef __MIPS_ASSEMBLER

#define DDR_GET_VALUE(x, y)                           \
({                                                    \
        unsigned long value, tmp;                     \
        tmp = x * 1000;                               \
        value = (tmp % y == 0) ? (tmp / y) : (tmp / y + 1); \
                value;                                \
})

#endif /* __MIPS_ASSEMBLER */

#define	EMC_BASE	0xB3410000

/*************************************************************************
 * EMC (External Memory Controller)
 *************************************************************************/
#define EMC_BCR    	(EMC_BASE + 0x00)  /* Bus Control Register */
#define EMC_PMEMBS1     (EMC_BASE + 0x6004) 
#define EMC_PMEMBS0     (EMC_BASE + 0x6008) 
#define EMC_SMCR0	(EMC_BASE + 0x10)  /* Static Memory Control Register 0 ??? */
#define EMC_SMCR1	(EMC_BASE + 0x14)  /* Static Memory Control Register 1 */
#define EMC_SMCR2	(EMC_BASE + 0x18)  /* Static Memory Control Register 2 */
#define EMC_SMCR3	(EMC_BASE + 0x1c)  /* Static Memory Control Register 3 */
#define EMC_SMCR4	(EMC_BASE + 0x20)  /* Static Memory Control Register 4 */
#define EMC_SMCR5	(EMC_BASE + 0x24)  /* Static Memory Control Register 5 */
#define EMC_SMCR6	(EMC_BASE + 0x28)  /* Static Memory Control Register 6 */
#define EMC_SACR0	(EMC_BASE + 0x30)  /* Static Memory Bank 0 Addr Config Reg */
#define EMC_SACR1	(EMC_BASE + 0x34)  /* Static Memory Bank 1 Addr Config Reg */
#define EMC_SACR2	(EMC_BASE + 0x38)  /* Static Memory Bank 2 Addr Config Reg */
#define EMC_SACR3	(EMC_BASE + 0x3c)  /* Static Memory Bank 3 Addr Config Reg */
#define EMC_SACR4	(EMC_BASE + 0x40)  /* Static Memory Bank 4 Addr Config Reg */
#define EMC_SACR5	(EMC_BASE + 0x44)  /* Static Memory Bank 5 Addr Config Reg */
#define EMC_SACR6	(EMC_BASE + 0x48)  /* Static Memory Bank 6 Addr Config Reg */
#define EMC_NFCSR	(EMC_BASE + 0x50)  /* NAND Flash Control/Status Register */
#define EMC_DMCR	(EMC_BASE + 0x80)  /* DRAM Control Register */
#define EMC_RTCSR	(EMC_BASE + 0x84)  /* Refresh Time Control/Status Register */
#define EMC_RTCNT	(EMC_BASE + 0x88)  /* Refresh Timer Counter */
#define EMC_RTCOR	(EMC_BASE + 0x8c)  /* Refresh Time Constant Register */
#define EMC_DMAR0	(EMC_BASE + 0x90)  /* SDRAM Bank 0 Addr Config Register */
#define EMC_DMAR1	(EMC_BASE + 0x94)  /* SDRAM Bank 1 Addr Config Register */
#define EMC_SDMR0	(EMC_BASE + 0xa000) /* Mode Register of SDRAM bank 0 */

#define REG_EMC_BCR 	REG32(EMC_BCR)
#define REG_EMC_PMEMBS1 REG32(EMC_PMEMBS1)
#define REG_EMC_PMEMBS0	REG32(EMC_PMEMBS0)
#define REG_EMC_SMCR0	REG32(EMC_SMCR0) // ???
#define REG_EMC_SMCR1	REG32(EMC_SMCR1)
#define REG_EMC_SMCR2	REG32(EMC_SMCR2)
#define REG_EMC_SMCR3	REG32(EMC_SMCR3)
#define REG_EMC_SMCR4	REG32(EMC_SMCR4)
#define REG_EMC_SMCR5	REG32(EMC_SMCR5)
#define REG_EMC_SMCR6	REG32(EMC_SMCR6)
#define REG_EMC_SACR0	REG32(EMC_SACR0)
#define REG_EMC_SACR1	REG32(EMC_SACR1)
#define REG_EMC_SACR2	REG32(EMC_SACR2)
#define REG_EMC_SACR3	REG32(EMC_SACR3)
#define REG_EMC_SACR4	REG32(EMC_SACR4)

#define REG_EMC_NFCSR	REG32(EMC_NFCSR)

#define REG_EMC_DMCR	REG32(EMC_DMCR)
#define REG_EMC_RTCSR	REG16(EMC_RTCSR)
#define REG_EMC_RTCNT	REG16(EMC_RTCNT)
#define REG_EMC_RTCOR	REG16(EMC_RTCOR)
#define REG_EMC_DMAR0	REG32(EMC_DMAR0)
#define REG_EMC_DMAR1	REG32(EMC_DMAR1)

/* Bus Control Register */
#define EMC_BCR_BT_SEL_BIT      30
#define EMC_BCR_BT_SEL_MASK     (0x3 << EMC_BCR_BT_SEL_BIT)
#define EMC_BCR_PK_SEL          (1 << 24)
#define EMC_BCR_BSR_MASK          (1 << 2)  /* Nand and SDRAM Bus Share Select: 0, share; 1, unshare */
  #define EMC_BCR_BSR_SHARE       (0 << 2)
  #define EMC_BCR_BSR_UNSHARE     (1 << 2)
#define EMC_BCR_BRE             (1 << 1)
#define EMC_BCR_ENDIAN          (1 << 0)

/* Static Memory Control Register */
#define EMC_SMCR_STRV_BIT	24
#define EMC_SMCR_STRV_MASK	(0x1f << EMC_SMCR_STRV_BIT)
#define EMC_SMCR_TAW_BIT	20
#define EMC_SMCR_TAW_MASK	(0x0f << EMC_SMCR_TAW_BIT)
#define EMC_SMCR_TBP_BIT	16
#define EMC_SMCR_TBP_MASK	(0x0f << EMC_SMCR_TBP_BIT)
#define EMC_SMCR_TAH_BIT	12
#define EMC_SMCR_TAH_MASK	(0x07 << EMC_SMCR_TAH_BIT)
#define EMC_SMCR_TAS_BIT	8
#define EMC_SMCR_TAS_MASK	(0x07 << EMC_SMCR_TAS_BIT)
#define EMC_SMCR_BW_BIT		6
#define EMC_SMCR_BW_MASK	(0x03 << EMC_SMCR_BW_BIT)
  #define EMC_SMCR_BW_8BIT	(0 << EMC_SMCR_BW_BIT)
  #define EMC_SMCR_BW_16BIT	(1 << EMC_SMCR_BW_BIT)
  #define EMC_SMCR_BW_32BIT	(2 << EMC_SMCR_BW_BIT)
#define EMC_SMCR_BCM		(1 << 3)
#define EMC_SMCR_BL_BIT		1
#define EMC_SMCR_BL_MASK	(0x03 << EMC_SMCR_BL_BIT)
  #define EMC_SMCR_BL_4		(0 << EMC_SMCR_BL_BIT)
  #define EMC_SMCR_BL_8		(1 << EMC_SMCR_BL_BIT)
  #define EMC_SMCR_BL_16	(2 << EMC_SMCR_BL_BIT)
  #define EMC_SMCR_BL_32	(3 << EMC_SMCR_BL_BIT)
#define EMC_SMCR_SMT		(1 << 0)

/* Static Memory Bank Addr Config Reg */
#define EMC_SACR_BASE_BIT	8
#define EMC_SACR_BASE_MASK	(0xff << EMC_SACR_BASE_BIT)
#define EMC_SACR_MASK_BIT	0
#define EMC_SACR_MASK_MASK	(0xff << EMC_SACR_MASK_BIT)

/* NAND Flash Control/Status Register */
#define EMC_NFCSR_NFCE4		(1 << 7) /* NAND Flash Enable */
#define EMC_NFCSR_NFE4		(1 << 6) /* NAND Flash FCE# Assertion Enable */
#define EMC_NFCSR_NFCE3		(1 << 5)
#define EMC_NFCSR_NFE3		(1 << 4)
#define EMC_NFCSR_NFCE2		(1 << 3)
#define EMC_NFCSR_NFE2		(1 << 2)
#define EMC_NFCSR_NFCE1		(1 << 1)
#define EMC_NFCSR_NFE1		(1 << 0)
#define EMC_NFCSR_NFE(n)	(1 << (((n)-1)*2))
#define EMC_NFCSR_NFCE(n)	(1 << (((n)*2)-1))

/* DRAM Control Register */
#define EMC_DMCR_BW_BIT		31
#define EMC_DMCR_BW		(1 << EMC_DMCR_BW_BIT)
#define EMC_DMCR_CA_BIT		26
#define EMC_DMCR_CA_MASK	(0x07 << EMC_DMCR_CA_BIT)
  #define EMC_DMCR_CA_8		(0 << EMC_DMCR_CA_BIT)
  #define EMC_DMCR_CA_9		(1 << EMC_DMCR_CA_BIT)
  #define EMC_DMCR_CA_10	(2 << EMC_DMCR_CA_BIT)
  #define EMC_DMCR_CA_11	(3 << EMC_DMCR_CA_BIT)
  #define EMC_DMCR_CA_12	(4 << EMC_DMCR_CA_BIT)
#define EMC_DMCR_RMODE		(1 << 25)
#define EMC_DMCR_RFSH		(1 << 24)
#define EMC_DMCR_MRSET		(1 << 23)
#define EMC_DMCR_RA_BIT		20
#define EMC_DMCR_RA_MASK	(0x03 << EMC_DMCR_RA_BIT)
  #define EMC_DMCR_RA_11	(0 << EMC_DMCR_RA_BIT)
  #define EMC_DMCR_RA_12	(1 << EMC_DMCR_RA_BIT)
  #define EMC_DMCR_RA_13	(2 << EMC_DMCR_RA_BIT)
#define EMC_DMCR_BA_BIT		19
#define EMC_DMCR_BA		(1 << EMC_DMCR_BA_BIT)
#define EMC_DMCR_PDM		(1 << 18)
#define EMC_DMCR_EPIN		(1 << 17)
#define EMC_DMCR_MBSEL		(1 << 16)
#define EMC_DMCR_TRAS_BIT	13
#define EMC_DMCR_TRAS_MASK	(0x07 << EMC_DMCR_TRAS_BIT)
#define EMC_DMCR_RCD_BIT	11
#define EMC_DMCR_RCD_MASK	(0x03 << EMC_DMCR_RCD_BIT)
#define EMC_DMCR_TPC_BIT	8
#define EMC_DMCR_TPC_MASK	(0x07 << EMC_DMCR_TPC_BIT)
#define EMC_DMCR_TRWL_BIT	5
#define EMC_DMCR_TRWL_MASK	(0x03 << EMC_DMCR_TRWL_BIT)
#define EMC_DMCR_TRC_BIT	2
#define EMC_DMCR_TRC_MASK	(0x07 << EMC_DMCR_TRC_BIT)
#define EMC_DMCR_TCL_BIT	0
#define EMC_DMCR_TCL_MASK	(0x03 << EMC_DMCR_TCL_BIT)

/* Refresh Time Control/Status Register */
#define EMC_RTCSR_SFR		(1 << 8)    /* self refresh flag */
#define EMC_RTCSR_CMF		(1 << 7)
#define EMC_RTCSR_CKS_BIT	0
#define EMC_RTCSR_CKS_MASK	(0x07 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_DISABLE	(0 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_4	(1 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_16	(2 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_64	(3 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_256	(4 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_1024	(5 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_2048	(6 << EMC_RTCSR_CKS_BIT)
  #define EMC_RTCSR_CKS_4096	(7 << EMC_RTCSR_CKS_BIT)

/* SDRAM Bank Address Configuration Register */
#define EMC_DMAR_BASE_BIT	8
#define EMC_DMAR_BASE_MASK	(0xff << EMC_DMAR_BASE_BIT)
#define EMC_DMAR_MASK_BIT	0
#define EMC_DMAR_MASK_MASK	(0xff << EMC_DMAR_MASK_BIT)

/* Mode Register of SDRAM bank 0 */
#define EMC_SDMR_BM		(1 << 9) /* Write Burst Mode */
#define EMC_SDMR_OM_BIT		7        /* Operating Mode */
#define EMC_SDMR_OM_MASK	(3 << EMC_SDMR_OM_BIT)
  #define EMC_SDMR_OM_NORMAL	(0 << EMC_SDMR_OM_BIT)
#define EMC_SDMR_CAS_BIT	4        /* CAS Latency */
#define EMC_SDMR_CAS_MASK	(7 << EMC_SDMR_CAS_BIT)
  #define EMC_SDMR_CAS_1	(1 << EMC_SDMR_CAS_BIT)
  #define EMC_SDMR_CAS_2	(2 << EMC_SDMR_CAS_BIT)
  #define EMC_SDMR_CAS_3	(3 << EMC_SDMR_CAS_BIT)
#define EMC_SDMR_BT_BIT		3        /* Burst Type */
#define EMC_SDMR_BT_MASK	(1 << EMC_SDMR_BT_BIT)
  #define EMC_SDMR_BT_SEQ	(0 << EMC_SDMR_BT_BIT) /* Sequential */
  #define EMC_SDMR_BT_INT	(1 << EMC_SDMR_BT_BIT) /* Interleave */
#define EMC_SDMR_BL_BIT		0        /* Burst Length */
#define EMC_SDMR_BL_MASK	(7 << EMC_SDMR_BL_BIT)
  #define EMC_SDMR_BL_1		(0 << EMC_SDMR_BL_BIT)
  #define EMC_SDMR_BL_2		(1 << EMC_SDMR_BL_BIT)
  #define EMC_SDMR_BL_4		(2 << EMC_SDMR_BL_BIT)
  #define EMC_SDMR_BL_8		(3 << EMC_SDMR_BL_BIT)

#define EMC_SDMR_CAS2_16BIT \
  (EMC_SDMR_CAS_2 | EMC_SDMR_BT_SEQ | EMC_SDMR_BL_2)
#define EMC_SDMR_CAS2_32BIT \
  (EMC_SDMR_CAS_2 | EMC_SDMR_BT_SEQ | EMC_SDMR_BL_4)
#define EMC_SDMR_CAS3_16BIT \
  (EMC_SDMR_CAS_3 | EMC_SDMR_BT_SEQ | EMC_SDMR_BL_2)
#define EMC_SDMR_CAS3_32BIT \
  (EMC_SDMR_CAS_3 | EMC_SDMR_BT_SEQ | EMC_SDMR_BL_4)

#define	I2C0_BASE	0xB0050000
#define	I2C1_BASE	0xB0051000

/*************************************************************************
 * I2C
 *************************************************************************/
#define	I2C_CTRL(n)		(I2C0_BASE + (n)*0x1000 + 0x00)
#define	I2C_TAR(n)     		(I2C0_BASE + (n)*0x1000 + 0x04)
#define	I2C_SAR(n)     		(I2C0_BASE + (n)*0x1000 + 0x08)
#define	I2C_DC(n)      		(I2C0_BASE + (n)*0x1000 + 0x10)
#define	I2C_SHCNT(n)		(I2C0_BASE + (n)*0x1000 + 0x14)
#define	I2C_SLCNT(n)		(I2C0_BASE + (n)*0x1000 + 0x18)
#define	I2C_FHCNT(n)		(I2C0_BASE + (n)*0x1000 + 0x1C)
#define	I2C_FLCNT(n)		(I2C0_BASE + (n)*0x1000 + 0x20)
#define	I2C_INTST(n)		(I2C0_BASE + (n)*0x1000 + 0x2C)
#define	I2C_INTM(n)		(I2C0_BASE + (n)*0x1000 + 0x30)
#define I2C_RXTL(n)		(I2C0_BASE + (n)*0x1000 + 0x38)
#define I2C_TXTL(n)		(I2C0_BASE + (n)*0x1000 + 0x3c)
#define	I2C_CINTR(n)		(I2C0_BASE + (n)*0x1000 + 0x40)
#define	I2C_CRXUF(n)		(I2C0_BASE + (n)*0x1000 + 0x44)
#define	I2C_CRXOF(n)		(I2C0_BASE + (n)*0x1000 + 0x48)
#define	I2C_CTXOF(n)		(I2C0_BASE + (n)*0x1000 + 0x4C)
#define	I2C_CRXREQ(n)		(I2C0_BASE + (n)*0x1000 + 0x50)
#define	I2C_CTXABRT(n)		(I2C0_BASE + (n)*0x1000 + 0x54)
#define	I2C_CRXDONE(n)		(I2C0_BASE + (n)*0x1000 + 0x58)
#define	I2C_CACT(n)		(I2C0_BASE + (n)*0x1000 + 0x5C)
#define	I2C_CSTP(n)		(I2C0_BASE + (n)*0x1000 + 0x60)
#define	I2C_CSTT(n)		(I2C0_BASE + (n)*0x1000 + 0x64)
#define	I2C_CGC(n)    		(I2C0_BASE + (n)*0x1000 + 0x68)
#define	I2C_ENB(n)     		(I2C0_BASE + (n)*0x1000 + 0x6C)
#define	I2C_STA(n)     		(I2C0_BASE + (n)*0x1000 + 0x70)
#define	I2C_TXABRT(n)		(I2C0_BASE + (n)*0x1000 + 0x80)
#define I2C_DMACR(n)            (I2C0_BASE + (n)*0x1000 + 0x88)
#define I2C_DMATDLR(n)          (I2C0_BASE + (n)*0x1000 + 0x8c)
#define I2C_DMARDLR(n)          (I2C0_BASE + (n)*0x1000 + 0x90)
#define	I2C_SDASU(n)		(I2C0_BASE + (n)*0x1000 + 0x94)
#define	I2C_ACKGC(n)		(I2C0_BASE + (n)*0x1000 + 0x98)
#define	I2C_ENSTA(n)		(I2C0_BASE + (n)*0x1000 + 0x9C)

#define	REG_I2C_CTRL(n)		REG8(I2C_CTRL(n)) /* I2C Control Register (I2C_CTRL) */
#define	REG_I2C_TAR(n)		REG16(I2C_TAR(n)) /* I2C target address (I2C_TAR) */
#define REG_I2C_SAR(n)		REG16(I2C_SAR(n))
#define REG_I2C_DC(n)		REG16(I2C_DC(n))
#define REG_I2C_SHCNT(n)       	REG16(I2C_SHCNT(n))
#define REG_I2C_SLCNT(n)       	REG16(I2C_SLCNT(n))
#define REG_I2C_FHCNT(n)       	REG16(I2C_FHCNT(n))
#define REG_I2C_FLCNT(n)       	REG16(I2C_FLCNT(n))
#define REG_I2C_INTST(n)       	REG16(I2C_INTST(n)) /* i2c interrupt status (I2C_INTST) */
#define REG_I2C_INTM(n)		REG16(I2C_INTM(n)) /* i2c interrupt mask status (I2C_INTM) */
#define REG_I2C_RXTL(n)		REG8(I2C_RXTL(n))
#define REG_I2C_TXTL(n)		REG8(I2C_TXTL(n))
#define REG_I2C_CINTR(n)       	REG8(I2C_CINTR(n))
#define REG_I2C_CRXUF(n)       	REG8(I2C_CRXUF(n))
#define REG_I2C_CRXOF(n)       	REG8(I2C_CRXOF(n))
#define REG_I2C_CTXOF(n)       	REG8(I2C_CTXOF(n))
#define REG_I2C_CRXREQ(n)      	REG8(I2C_CRXREQ(n))
#define REG_I2C_CTXABRT(n)     	REG8(I2C_CTXABRT(n))
#define REG_I2C_CRXDONE(n)     	REG8(I2C_CRXDONE(n))
#define REG_I2C_CACT(n)		REG8(I2C_CACT(n))
#define REG_I2C_CSTP(n)		REG8(I2C_CSTP(n))
#define REG_I2C_CSTT(n)		REG16(I2C_CSTT(n))
#define REG_I2C_CGC(n)		REG8(I2C_CGC(n))
#define REG_I2C_ENB(n)		REG8(I2C_ENB(n))
#define REG_I2C_STA(n)		REG8(I2C_STA(n))
#define REG_I2C_TXABRT(n)      	REG16(I2C_TXABRT(n))
#define REG_I2C_DMACR(n)        REG8(I2C_DMACR(n))
#define REG_I2C_DMATDLR(n)      REG8(I2C_DMATDLR(n))
#define REG_I2C_DMARDLR(n)      REG8(I2C_DMARDLR(n))
#define REG_I2C_SDASU(n)       	REG8(I2C_SDASU(n))
#define REG_I2C_ACKGC(n)       	REG8(I2C_ACKGC(n))
#define REG_I2C_ENSTA(n)       	REG8(I2C_ENSTA(n))

/* I2C Control Register (I2C_CTRL) */

#define I2C_CTRL_STPHLD		(1 << 7) /* Stop Hold Enable bit: when tx fifo empty, 0: send stop 1: never send stop*/
#define I2C_CTRL_SLVDIS		(1 << 6) /* after reset slave is disabled*/
#define I2C_CTRL_REST		(1 << 5)	
#define I2C_CTRL_MATP		(1 << 4) /* 1: 10bit address 0: 7bit addressing*/
#define I2C_CTRL_SATP		(1 << 3) /* 1: 10bit address 0: 7bit address*/
#define I2C_CTRL_SPDF		(2 << 1) /* fast mode 400kbps */
#define I2C_CTRL_SPDS		(1 << 1) /* standard mode 100kbps */
#define I2C_CTRL_MD		(1 << 0) /* master enabled*/

/* I2C target address (I2C_TAR) */

#define I2C_TAR_MATP		(1 << 12)
#define I2C_TAR_SPECIAL		(1 << 11)
#define I2C_TAR_GC_OR_START	(1 << 10)
#define I2C_TAR_I2CTAR_BIT	0
#define I2C_TAR_I2CTAR_MASK	(0x3ff << I2C_TAR_I2CTAR_BIT)

/* I2C slave address  */
#define I2C_SAR_I2CSAR_BIT	0
#define I2C_SAR_I2CSAR_MASK	(0x3ff << I2C_SAR_I2CSAR_BIT)

/* I2C data buffer and command (I2C_DC) */

#define I2C_DC_CMD			(1 << 8) /* 1 read 0  write*/
#define I2C_DC_DAT_BIT		0
#define I2C_DC_DAT_MASK		(0xff << I2C_DC_DAT_BIT) /* 1 read 0  write*/

/* i2c interrupt status (I2C_INTST) */

#define I2C_INTST_IGC			(1 << 11) /* */
#define I2C_INTST_ISTT			(1 << 10)
#define I2C_INTST_ISTP			(1 << 9)
#define I2C_INTST_IACT			(1 << 8)
#define I2C_INTST_RXDN			(1 << 7)
#define I2C_INTST_TXABT			(1 << 6) 
#define I2C_INTST_RDREQ			(1 << 5)
#define I2C_INTST_TXEMP			(1 << 4)
#define I2C_INTST_TXOF			(1 << 3)
#define I2C_INTST_RXFL			(1 << 2)
#define I2C_INTST_RXOF			(1 << 1)
#define I2C_INTST_RXUF			(1 << 0)

/* i2c interrupt mask status (I2C_INTM) */

#define I2C_INTM_MIGC			(1 << 11) /* */
#define I2C_INTM_MISTT			(1 << 10)
#define I2C_INTM_MISTP			(1 << 9)
#define I2C_INTM_MIACT			(1 << 8)
#define I2C_INTM_MRXDN			(1 << 7)
#define I2C_INTM_MTXABT			(1 << 6)
#define I2C_INTM_MRDREQ			(1 << 5)
#define I2C_INTM_MTXEMP			(1 << 4)
#define I2C_INTM_MTXOF			(1 << 3)
#define I2C_INTM_MRXFL			(1 << 2)
#define I2C_INTM_MRXOF			(1 << 1)
#define I2C_INTM_MRXUF			(1 << 0)

/* I2C Clear Combined and Individual Interrupts (I2C_CINTR) */

#define I2C_CINTR_CINT 			(1 << 0)

/* I2C Clear TX_OVER Interrupt */
/* I2C Clear RDREQ Interrupt */
/* I2C Clear TX_ABRT Interrupt */
/* I2C Clear RX_DONE Interrupt */
/* I2C Clear ACTIVITY Interrupt */
/* I2C Clear STOP Interrupts */
/* I2C Clear START Interrupts */
/* I2C Clear GEN_CALL Interrupts */

/* I2C Enable (I2C_ENB) */

#define I2C_ENB_I2CENB 		(1 << 0) /* Enable the i2c */	

/* I2C Status Register (I2C_STA) */

#define I2C_STA_SLVACT		(1 << 6) /* Slave FSM is not in IDLE state */
#define I2C_STA_MSTACT		(1 << 5) /* Master FSM is not in IDLE state */
#define I2C_STA_RFF		(1 << 4) /* RFIFO if full */
#define I2C_STA_RFNE		(1 << 3) /* RFIFO is not empty */
#define I2C_STA_TFE		(1 << 2) /* TFIFO is empty */
#define I2C_STA_TFNF		(1 << 1) /* TFIFO is not full  */
#define I2C_STA_ACT		(1 << 0) /* I2C Activity Status */

/* I2C Transmit Abort Status Register (I2C_TXABRT) */

#define I2C_TXABRT_SLVRD_INTX		(1 << 15)
#define I2C_TXABRT_SLV_ARBLOST		(1 << 14)
#define I2C_TXABRT_SLVFLUSH_TXFIFO	(1 << 13)
#define I2C_TXABRT_ARB_LOST		(1 << 12)
#define I2C_TXABRT_ABRT_MASTER_DIS	(1 << 11)
#define I2C_TXABRT_ABRT_10B_RD_NORSTRT	(1 << 10)
#define I2C_TXABRT_SBYTE_NORSTRT	(1 << 9)
#define I2C_TXABRT_ABRT_HS_NORSTRT	(1 << 8)
#define I2C_TXABRT_SBYTE_ACKDET		(1 << 7)
#define I2C_TXABRT_ABRT_HS_ACKD		(1 << 6)
#define I2C_TXABRT_ABRT_GCALL_READ	(1 << 5)
#define I2C_TXABRT_ABRT_GCALL_NOACK	(1 << 4)
#define I2C_TXABRT_ABRT_XDATA_NOACK	(1 << 3)
#define I2C_TXABRT_ABRT_10ADDR2_NOACK	(1 << 2)
#define I2C_TXABRT_ABRT_10ADDR1_NOACK	(1 << 1)
#define I2C_TXABRT_ABRT_7B_ADDR_NOACK	(1 << 0)

/* */
#define I2C_DMACR_TDEN			(1 << 1)
#define I2C_DMACR_RDEN			(1 << 0)

/* */
#define I2C_DMATDLR_TDLR_BIT		0
#define I2C_DMATDLR_TDLR_MASK		(0x1f << I2C_DMATDLR_TDLR_BIT)

/* */
#define I2C_DMARDLR_RDLR_BIT		0
#define I2C_DMARDLR_RDLR_MASK		(0x1f << I2C_DMARDLR_RDLR_BIT)

/* I2C Enable Status Register (I2C_ENSTA) */

#define I2C_ENSTA_SLVRDLST		(1 << 2)
#define I2C_ENSTA_SLVDISB 		(1 << 1)
#define I2C_ENSTA_I2CEN 		(1 << 0) /* when read as 1, i2c is deemed to be in an enabled state
						    when read as 0, i2c is deemed completely inactive. The cpu can 
						 safely read this bit anytime .When this bit is read as 0 ,the cpu can 
						 safely read SLVRDLST and SLVDISB */

/* I2C standard mode high count register(I2CSHCNT) */
#define I2CSHCNT_ADJUST(n)	(((n) - 8) < 6 ? 6 : ((n) - 8))

/* I2C standard mode low count register(I2CSLCNT) */
#define I2CSLCNT_ADJUST(n)	(((n) - 1) < 8 ? 8 : ((n) - 1))

/* I2C fast mode high count register(I2CFHCNT) */
#define I2CFHCNT_ADJUST(n)	(((n) - 8) < 6 ? 6 : ((n) - 8))

/* I2C fast mode low count register(I2CFLCNT) */
#define I2CFLCNT_ADJUST(n)	(((n) - 1) < 8 ? 8 : ((n) - 1))

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * I2C
 ***************************************************************************/

#define __i2c_enable(n)		( REG_I2C_ENB(n) = 1 )
#define __i2c_disable(n)       	( REG_I2C_ENB(n) = 0 )

#define __i2c_is_enable(n)       ( REG_I2C_ENSTA(n) & I2C_ENB_I2CENB )
#define __i2c_is_disable(n)      ( !(REG_I2C_ENSTA(n) & I2C_ENB_I2CENB) )

#define __i2c_abrt(n)            ( REG_I2C_TXABRT(n) != 0 )
#define __i2c_abrt_intr(n)       (REG_I2C_INTST(n) & I2C_INTST_TXABT)
#define __i2c_master_active(n)   ( REG_I2C_STA(n) & I2C_STA_MSTACT )
#define __i2c_abrt_7b_addr_nack(n)  ( REG_I2C_TXABRT(n) & I2C_TXABRT_ABRT_7B_ADDR_NOACK )
#define __i2c_txfifo_is_empty(n)     ( REG_I2C_STA(n) & I2C_STA_TFE )
#define __i2c_clear_interrupts(ret,n)    ( ret = REG_I2C_CINTR(n) )

#define __i2c_dma_rd_enable(n)        SETREG8(I2C_DMACR(n),1 << 0)
#define __i2c_dma_rd_disable(n)       CLRREG8(I2C_DMACR(n),1 << 0)
#define __i2c_dma_td_enable(n)        SETREG8(I2C_DMACR(n),1 << 1)
#define __i2c_dma_td_disable(n)       CLRREG8(I2C_DMACR(n),1 << 1)

#define __i2c_send_stop(n)           CLRREG8(I2C_CTRL(n), I2C_CTRL_STPHLD)
#define __i2c_nsend_stop(n)          SETREG8(I2C_CTRL(n), I2C_CTRL_STPHLD)

#define __i2c_set_dma_td_level(n,data) OUTREG8(I2C_DMATDLR(n),data)
#define __i2c_set_dma_rd_level(n,data) OUTREG8(I2C_DMARDLR(n),data)   

/*
#define __i2c_set_clk(dev_clk, i2c_clk) \
  ( REG_I2C_GR = (dev_clk) / (16*(i2c_clk)) - 1 )
*/

#define __i2c_read(n)		( REG_I2C_DC(n) & 0xff )
#define __i2c_write(val,n)	( REG_I2C_DC(n) = (val) )

#endif /* __MIPS_ASSEMBLER */

#define	IPU_BASE	0xB3080000

/*************************************************************************
 * IPU (Image Processing Unit)
 *************************************************************************/
#define IPU_V_BASE		0xB3080000
#define IPU_P_BASE		0x13080000

/* Register offset */
#define REG_CTRL		0x0  /* IPU Control Register */
#define REG_STATUS		0x4  /* IPU Status Register */
#define REG_D_FMT		0x8  /* Data Format Register */
#define REG_Y_ADDR		0xc  /* Input Y or YUV422 Packaged Data Address Register */
#define REG_U_ADDR		0x10 /* Input U Data Address Register */
#define REG_V_ADDR		0x14 /* Input V Data Address Register */
#define REG_IN_FM_GS		0x18 /* Input Geometric Size Register */
#define REG_Y_STRIDE		0x1c /* Input Y Data Line Stride Register */
#define REG_UV_STRIDE		0x20 /* Input UV Data Line Stride Register */
#define REG_OUT_ADDR		0x24 /* Output Frame Start Address Register */
#define REG_OUT_GS		0x28 /* Output Geometric Size Register */
#define REG_OUT_STRIDE		0x2c /* Output Data Line Stride Register */
#define REG_RSZ_COEF_INDEX	0x30 /* Resize Coefficients Table Index Register */
#define REG_CSC_CO_COEF		0x34 /* CSC C0 Coefficient Register */
#define REG_CSC_C1_COEF		0x38 /* CSC C1 Coefficient Register */
#define REG_CSC_C2_COEF 	0x3c /* CSC C2 Coefficient Register */
#define REG_CSC_C3_COEF 	0x40 /* CSC C3 Coefficient Register */
#define REG_CSC_C4_COEF 	0x44 /* CSC C4 Coefficient Register */
#define HRSZ_LUT_BASE 		0x48 /* Horizontal Resize Coefficients Look Up Table Register group */
#define VRSZ_LUT_BASE 		0x4c /* Virtical Resize Coefficients Look Up Table Register group */
#define REG_CSC_OFSET_PARA	0x50 /* CSC Offset Parameter Register */
#define REG_Y_PHY_T_ADDR	0x54 /* Input Y Physical Table Address Register */
#define REG_U_PHY_T_ADDR	0x58 /* Input U Physical Table Address Register */
#define REG_V_PHY_T_ADDR	0x5c /* Input V Physical Table Address Register */
#define REG_OUT_PHY_T_ADDR	0x60 /* Output Physical Table Address Register */

/* REG_CTRL: IPU Control Register */
#define IPU_CE_SFT	0x0
#define IPU_CE_MSK	0x1
#define IPU_RUN_SFT	0x1
#define IPU_RUN_MSK	0x1
#define HRSZ_EN_SFT	0x2
#define HRSZ_EN_MSK	0x1
#define VRSZ_EN_SFT	0x3
#define VRSZ_EN_MSK	0x1
#define CSC_EN_SFT	0x4
#define CSC_EN_MSK	0x1
#define FM_IRQ_EN_SFT	0x5
#define FM_IRQ_EN_MSK	0x1
#define IPU_RST_SFT	0x6
#define IPU_RST_MSK	0x1
#define H_SCALE_SFT	0x8
#define H_SCALE_MSK	0x1
#define V_SCALE_SFT	0x9
#define V_SCALE_MSK	0x1
#define PKG_SEL_SFT	0xA
#define PKG_SEL_MSK	0x1
#define LCDC_SEL_SFT	0xB
#define LCDC_SEL_MSK	0x1
#define SPAGE_MAP_SFT	0xC
#define SPAGE_MAP_MSK	0x1
#define DPAGE_SEL_SFT	0xD
#define DPAGE_SEL_MSK	0x1
#define DISP_SEL_SFT	0xE
#define DISP_SEL_MSK	0x1
#define FIELD_CONF_EN_SFT 15
#define FIELD_CONF_EN_MSK 1
#define FIELD_SEL_SFT	16
#define FIELD_SEL_MSK	1
#define DFIX_SEL_SFT	17
#define DFIX_SEL_MSK	1

/* REG_STATUS: IPU Status Register */
#define OUT_END_SFT	0x0
#define OUT_END_MSK	0x1
#define FMT_ERR_SFT	0x1
#define FMT_ERR_MSK	0x1
#define SIZE_ERR_SFT	0x2
#define SIZE_ERR_MSK	0x1

/* D_FMT: Data Format Register */
#define IN_FMT_SFT	0x0
#define IN_FMT_MSK 	0x3
#define IN_OFT_SFT 	0x2
#define IN_OFT_MSK 	0x3
#define YUV_PKG_OUT_SFT	0x10
#define YUV_PKG_OUT_MSK	0x7
#define OUT_FMT_SFT 	0x13
#define OUT_FMT_MSK 	0x3
#define RGB_OUT_OFT_SFT	0x15
#define RGB_OUT_OFT_MSK	0x7
#define RGB888_FMT_SFT	0x18
#define RGB888_FMT_MSK	0x1

/* IN_FM_GS: Input Geometric Size Register */
#define IN_FM_H_SFT	0x0
#define IN_FM_H_MSK	0xFFF
#define IN_FM_W_SFT	0x10
#define IN_FM_W_MSK	0xFFF

/* Y_STRIDE: Input Y Data Line Stride Register */
#define Y_S_SFT		0x0
#define Y_S_MSK		0x3FFF

/* UV_STRIDE: Input UV Data Line Stride Register */
#define V_S_SFT		0x0
#define V_S_MSK		0x1FFF
#define U_S_SFT 	0x10
#define U_S_MSK		0x1FFF

/* OUT_GS: Output Geometric Size Register */
#define OUT_FM_H_SFT	0x0
#define OUT_FM_H_MSK	0x1FFF
#define OUT_FM_W_SFT	0x10
#define OUT_FM_W_MSK	0x7FFF

/* OUT_STRIDE: Output Data Line Stride Register */
#define OUT_S_SFT	0x0
#define OUT_S_MSK	0xFFFF

/* RSZ_COEF_INDEX: Resize Coefficients Table Index Register */
#define VE_IDX_SFT	0x0
#define VE_IDX_MSK	0x1F
#define HE_IDX_SFT	0x10
#define HE_IDX_MSK	0x1F

/* CSC_CX_COEF: CSC CX Coefficient Register */
#define CX_COEF_SFT	0x0
#define CX_COEF_MSK	0xFFF

/* HRSZ_LUT_BASE, VRSZ_LUT_BASE: Resize Coefficients Look Up Table Register group */
#define LUT_LEN		20

#define OUT_N_SFT	0x0
#define OUT_N_MSK	0x1
#define IN_N_SFT	0x1
#define IN_N_MSK	0x1
#define W_COEF_SFT	0x2
#define W_COEF_MSK	0x3FF

/* CSC_OFSET_PARA: CSC Offset Parameter Register */
#define CHROM_OF_SFT	0x10
#define CHROM_OF_MSK	0xFF
#define LUMA_OF_SFT	0x00
#define LUMA_OF_MSK	0xFF

#ifndef __MIPS_ASSEMBLER

#if 0
/*************************************************************************
 * IPU (Image Processing Unit)
 *************************************************************************/
#define u32 volatile unsigned long

#define write_reg(reg, val)	\
do {				\
	*(u32 *)(reg) = (val);	\
} while(0)

#define read_reg(reg, off)	(*(u32 *)((reg)+(off)))

#define set_ipu_fmt(rgb_888_out_fmt, rgb_out_oft, out_fmt, yuv_pkg_out, in_oft, in_fmt ) \
({ write_reg( (IPU_V_BASE + REG_D_FMT), ((in_fmt) & IN_FMT_MSK)<<IN_FMT_SFT \
| ((in_oft) & IN_OFT_MSK)<< IN_OFT_SFT \
| ((out_fmt) & OUT_FMT_MSK)<<OUT_FMT_SFT \
| ((yuv_pkg_out) & YUV_PKG_OUT_MSK ) << YUV_PKG_OUT_SFT \
| ((rgb_888_out_fmt) & RGB888_FMT_MSK ) << RGB888_FMT_SFT \
| ((rgb_out_oft) & RGB_OUT_OFT_MSK ) << RGB_OUT_OFT_SFT); \
})
#define set_y_addr(y_addr) \
({ write_reg( (IPU_V_BASE + REG_Y_ADDR), y_addr); \
})
#define set_u_addr(u_addr) \
({ write_reg( (IPU_V_BASE + REG_U_ADDR), u_addr); \
})

#define set_v_addr(v_addr) \
({ write_reg( (IPU_V_BASE + REG_V_ADDR), v_addr); \
})

#define set_y_phy_t_addr(y_phy_t_addr) \
({ write_reg( (IPU_V_BASE + REG_Y_PHY_T_ADDR), y_phy_t_addr); \
})

#define set_u_phy_t_addr(u_phy_t_addr) \
({ write_reg( (IPU_V_BASE + REG_U_PHY_T_ADDR), u_phy_t_addr); \
})

#define set_v_phy_t_addr(v_phy_t_addr) \
({ write_reg( (IPU_V_BASE + REG_V_PHY_T_ADDR), v_phy_t_addr); \
})

#define set_out_phy_t_addr(out_phy_t_addr) \
({ write_reg( (IPU_V_BASE + REG_OUT_PHY_T_ADDR), out_phy_t_addr); \
})

#define set_inframe_gsize(width, height, y_stride, u_stride, v_stride) \
({ write_reg( (IPU_V_BASE + REG_IN_FM_GS), ((width) & IN_FM_W_MSK)<<IN_FM_W_SFT \
| ((height) & IN_FM_H_MSK)<<IN_FM_H_SFT); \
 write_reg( (IPU_V_BASE + REG_Y_STRIDE), ((y_stride) & Y_S_MSK)<<Y_S_SFT); \
 write_reg( (IPU_V_BASE + REG_UV_STRIDE), ((u_stride) & U_S_MSK)<<U_S_SFT \
| ((v_stride) & V_S_MSK)<<V_S_SFT); \
})
#define set_out_addr(out_addr) \
({ write_reg( (IPU_V_BASE + REG_OUT_ADDR), out_addr); \
})
#define set_outframe_gsize(width, height, o_stride) \
({ write_reg( (IPU_V_BASE + REG_OUT_GS), ((width) & OUT_FM_W_MSK)<<OUT_FM_W_SFT \
| ((height) & OUT_FM_H_MSK)<<OUT_FM_H_SFT); \
 write_reg( (IPU_V_BASE + REG_OUT_STRIDE), ((o_stride) & OUT_S_MSK)<<OUT_S_SFT); \
})
#define set_rsz_lut_end(h_end, v_end) \
({ write_reg( (IPU_V_BASE + REG_RSZ_COEF_INDEX), ((h_end) & HE_IDX_MSK)<<HE_IDX_SFT \
| ((v_end) & VE_IDX_MSK)<<VE_IDX_SFT); \
})
#define set_csc_c0(c0_coeff) \
({ write_reg( (IPU_V_BASE + REG_CSC_CO_COEF), ((c0_coeff) & CX_COEF_MSK)<<CX_COEF_SFT); \
})
#define set_csc_c1(c1_coeff) \
({ write_reg( (IPU_V_BASE + REG_CSC_C1_COEF), ((c1_coeff) & CX_COEF_MSK)<<CX_COEF_SFT); \
})
#define set_csc_c2(c2_coeff) \
({ write_reg( (IPU_V_BASE + REG_CSC_C2_COEF), ((c2_coeff) & CX_COEF_MSK)<<CX_COEF_SFT); \
})
#define set_csc_c3(c3_coeff) \
({ write_reg( (IPU_V_BASE + REG_CSC_C3_COEF), ((c3_coeff) & CX_COEF_MSK)<<CX_COEF_SFT); \
})
#define set_csc_c4(c4_coeff) \
({ write_reg( (IPU_V_BASE + REG_CSC_C4_COEF), ((c4_coeff) & CX_COEF_MSK)<<CX_COEF_SFT); \
})
#define set_hrsz_lut_coef(coef, in_n, out_n) \
({ write_reg( (IPU_V_BASE + HRSZ_LUT_BASE ), ((coef) & W_COEF_MSK)<<W_COEF_SFT \
| ((in_n) & IN_N_MSK)<<IN_N_SFT | ((out_n) & OUT_N_MSK)<<OUT_N_SFT); \
})
#define set_vrsz_lut_coef(coef, in_n, out_n) \
({ write_reg( (IPU_V_BASE + VRSZ_LUT_BASE), ((coef) & W_COEF_MSK)<<W_COEF_SFT \
| ((in_n) & IN_N_MSK)<<IN_N_SFT | ((out_n) & OUT_N_MSK)<<OUT_N_SFT); \
})

#define set_primary_ctrl(vrsz_en, hrsz_en,csc_en, irq_en) \
({ write_reg( (IPU_V_BASE + REG_CTRL), ((irq_en) & FM_IRQ_EN_MSK)<<FM_IRQ_EN_SFT \
| ((vrsz_en) & VRSZ_EN_MSK)<<VRSZ_EN_SFT \
| ((hrsz_en) & HRSZ_EN_MSK)<<HRSZ_EN_SFT \
| ((csc_en) & CSC_EN_MSK)<<CSC_EN_SFT \
| (read_reg(IPU_V_BASE, REG_CTRL)) \
& ~(CSC_EN_MSK<<CSC_EN_SFT | FM_IRQ_EN_MSK<<FM_IRQ_EN_SFT | VRSZ_EN_MSK<<VRSZ_EN_SFT | HRSZ_EN_MSK<<HRSZ_EN_SFT ) ); \
})

#define set_source_ctrl(pkg_sel, spage_sel) \
({ write_reg( (IPU_V_BASE + REG_CTRL), ((pkg_sel) & PKG_SEL_MSK  )<< PKG_SEL_SFT \
| ((spage_sel) & SPAGE_MAP_MSK )<< SPAGE_MAP_SFT \
| (read_reg(IPU_V_BASE, REG_CTRL)) \
& ~(SPAGE_MAP_MSK << SPAGE_MAP_SFT | PKG_SEL_MSK << PKG_SEL_SFT ) ) ; \
})

#define set_out_ctrl(lcdc_sel, dpage_sel, disp_sel) \
({ write_reg( (IPU_V_BASE + REG_CTRL), ((lcdc_sel) & LCDC_SEL_MSK  )<< LCDC_SEL_SFT \
| ((dpage_sel) & DPAGE_SEL_MSK )<< DPAGE_SEL_SFT \
| ((disp_sel) & DISP_SEL_MSK )<< DISP_SEL_SFT \
| (read_reg(IPU_V_BASE, REG_CTRL)) \
& ~(LCDC_SEL_MSK<< LCDC_SEL_SFT | DPAGE_SEL_MSK << DPAGE_SEL_SFT | DISP_SEL_MSK << DISP_SEL_SFT ) ); \
})

#define set_scale_ctrl(v_scal, h_scal) \
({ write_reg( (IPU_V_BASE + REG_CTRL), ((v_scal) & V_SCALE_MSK)<<V_SCALE_SFT \
| ((h_scal) & H_SCALE_MSK)<<H_SCALE_SFT \
| (read_reg(IPU_V_BASE, REG_CTRL)) & ~(V_SCALE_MSK<<V_SCALE_SFT | H_SCALE_MSK<<H_SCALE_SFT ) ); \
})

#define set_csc_ofset_para(chrom_oft, luma_oft) \
({ write_reg( (IPU_V_BASE + REG_CSC_OFSET_PARA ), ((chrom_oft) & CHROM_OF_MSK ) << CHROM_OF_SFT \
| ((luma_oft) & LUMA_OF_MSK ) << LUMA_OF_SFT ) ; \
})

#define sw_reset_ipu() \
({ write_reg( (IPU_V_BASE + REG_CTRL), (read_reg(IPU_V_BASE, REG_CTRL)) \
| IPU_RST_MSK<<IPU_RST_SFT); \
})
#define enable_ipu() \
({ write_reg( (IPU_V_BASE + REG_CTRL), (read_reg(IPU_V_BASE, REG_CTRL)) | 0x1); \
})
#define disable_ipu() \
({ write_reg( (IPU_V_BASE + REG_CTRL), (read_reg(IPU_V_BASE, REG_CTRL)) & ~0x1); \
})
#define run_ipu() \
({ write_reg( (IPU_V_BASE + REG_CTRL), (read_reg(IPU_V_BASE, REG_CTRL)) | 0x2); \
})
#define stop_ipu() \
({ write_reg( (IPU_V_BASE + REG_CTRL), (read_reg(IPU_V_BASE, REG_CTRL)) & ~0x2); \
})

#define polling_end_flag() \
({ (read_reg(IPU_V_BASE, REG_STATUS)) & 0x01; \
})

#define start_vlut_coef_write() \
({ write_reg( (IPU_V_BASE + VRSZ_LUT_BASE), ( 0x1<<12 ) ); \
})

#define start_hlut_coef_write() \
({ write_reg( (IPU_V_BASE + HRSZ_LUT_BASE), ( 0x01<<12 ) ); \
})

#define clear_end_flag() \
({ write_reg( (IPU_V_BASE + REG_STATUS), 0); \
})
#endif /* #if 0 */

#endif /* __MIPS_ASSEMBLER */

#define	LCD_BASE	0xB3050000
#define	SLCD_BASE	0xB3050000

/*************************************************************************
 * SLCD (Smart LCD Controller)
 *************************************************************************/

#define SLCD_CFG	(SLCD_BASE + 0xA0)  /* SLCD Configure Register */
#define SLCD_CTRL	(SLCD_BASE + 0xA4)  /* SLCD Control Register */
#define SLCD_STATE	(SLCD_BASE + 0xA8)  /* SLCD Status Register */
#define SLCD_DATA	(SLCD_BASE + 0xAC)  /* SLCD Data Register */

#define REG_SLCD_CFG	REG32(SLCD_CFG)
#define REG_SLCD_CTRL	REG8(SLCD_CTRL)
#define REG_SLCD_STATE	REG8(SLCD_STATE)
#define REG_SLCD_DATA	REG32(SLCD_DATA)

/* SLCD Configure Register */
#define SLCD_CFG_DWIDTH_BIT	10
#define SLCD_CFG_DWIDTH_MASK	(0x7 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_18BIT	(0 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_16BIT	(1 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_8BIT_x3	(2 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_8BIT_x2	(3 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_8BIT_x1	(4 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_24BIT	(5 << SLCD_CFG_DWIDTH_BIT)
  #define SLCD_CFG_DWIDTH_9BIT_x2	(7 << SLCD_CFG_DWIDTH_BIT)
#define SLCD_CFG_CWIDTH_BIT	(8)
#define SLCD_CFG_CWIDTH_MASK	(0x3 << SLCD_CFG_CWIDTH_BIT)
#define SLCD_CFG_CWIDTH_16BIT	(0 << SLCD_CFG_CWIDTH_BIT)
#define SLCD_CFG_CWIDTH_8BIT	(1 << SLCD_CFG_CWIDTH_BIT)
#define SLCD_CFG_CWIDTH_18BIT	(2 << SLCD_CFG_CWIDTH_BIT)
#define SLCD_CFG_CWIDTH_24BIT	(3 << SLCD_CFG_CWIDTH_BIT)
#define SLCD_CFG_CS_ACTIVE_LOW	(0 << 4)
#define SLCD_CFG_CS_ACTIVE_HIGH	(1 << 4)
#define SLCD_CFG_RS_CMD_LOW	(0 << 3)
#define SLCD_CFG_RS_CMD_HIGH	(1 << 3)
#define SLCD_CFG_CLK_ACTIVE_FALLING	(0 << 1)
#define SLCD_CFG_CLK_ACTIVE_RISING	(1 << 1)
#define SLCD_CFG_TYPE_PARALLEL	(0 << 0)
#define SLCD_CFG_TYPE_SERIAL	(1 << 0)

/* SLCD Control Register */
#define SLCD_CTRL_DMA_MODE	(1 << 2)
#define SLCD_CTRL_DMA_START	(1 << 1)
#define SLCD_CTRL_DMA_EN	(1 << 0)

/* SLCD Status Register */
#define SLCD_STATE_BUSY		(1 << 0)

/* SLCD Data Register */
#define SLCD_DATA_RS_DATA	(0 << 31)
#define SLCD_DATA_RS_COMMAND	(1 << 31)

/*************************************************************************
 * LCD (LCD Controller)
 *************************************************************************/
#define LCD_CFG		(LCD_BASE + 0x00) /* LCD Configure Register */
#define LCD_CTRL	(LCD_BASE + 0x30) /* LCD Control Register */
#define LCD_STATE	(LCD_BASE + 0x34) /* LCD Status Register */

#define LCD_OSDC	(LCD_BASE + 0x100) /* LCD OSD Configure Register */
#define LCD_OSDCTRL	(LCD_BASE + 0x104) /* LCD OSD Control Register */
#define LCD_OSDS	(LCD_BASE + 0x108) /* LCD OSD Status Register */
#define LCD_BGC		(LCD_BASE + 0x10C) /* LCD Background Color Register */
#define LCD_KEY0	(LCD_BASE + 0x110) /* LCD Foreground Color Key Register 0 */
#define LCD_KEY1	(LCD_BASE + 0x114) /* LCD Foreground Color Key Register 1 */
#define LCD_ALPHA	(LCD_BASE + 0x118) /* LCD ALPHA Register */
#define LCD_IPUR	(LCD_BASE + 0x11C) /* LCD IPU Restart Register */
#define LCD_RGBC	(LCD_BASE + 0x90) /* RGB Controll Register */

#define LCD_VAT		(LCD_BASE + 0x0c) /* Virtual Area Setting Register */
#define LCD_DAH		(LCD_BASE + 0x10) /* Display Area Horizontal Start/End Point */
#define LCD_DAV		(LCD_BASE + 0x14) /* Display Area Vertical Start/End Point */

#define LCD_XYP0	(LCD_BASE + 0x120) /* Foreground 0 XY Position Register */
#define LCD_XYP0_PART2	(LCD_BASE + 0x1F0) /* Foreground 0 PART2 XY Position Register */
#define LCD_XYP1	(LCD_BASE + 0x124) /* Foreground 1 XY Position Register */
#define LCD_SIZE0	(LCD_BASE + 0x128) /* Foreground 0 Size Register */
#define LCD_SIZE0_PART2	(LCD_BASE + 0x1F4) /*Foreground 0 PART2 Size Register */
#define LCD_SIZE1	(LCD_BASE + 0x12C) /* Foreground 1 Size Register */

#define LCD_VSYNC	(LCD_BASE + 0x04) /* Vertical Synchronize Register */
#define LCD_HSYNC	(LCD_BASE + 0x08) /* Horizontal Synchronize Register */
#define LCD_PS		(LCD_BASE + 0x18) /* PS Signal Setting */
#define LCD_CLS		(LCD_BASE + 0x1c) /* CLS Signal Setting */
#define LCD_SPL		(LCD_BASE + 0x20) /* SPL Signal Setting */
#define LCD_REV		(LCD_BASE + 0x24) /* REV Signal Setting */
#define LCD_IID		(LCD_BASE + 0x38) /* Interrupt ID Register */
#define LCD_DA0		(LCD_BASE + 0x40) /* Descriptor Address Register 0 */
#define LCD_SA0		(LCD_BASE + 0x44) /* Source Address Register 0 */
#define LCD_FID0	(LCD_BASE + 0x48) /* Frame ID Register 0 */
#define LCD_CMD0	(LCD_BASE + 0x4c) /* DMA Command Register 0 */

#define LCD_OFFS0	(LCD_BASE + 0x60) /* DMA Offsize Register 0 */
#define LCD_PW0		(LCD_BASE + 0x64) /* DMA Page Width Register 0 */
#define LCD_CNUM0	(LCD_BASE + 0x68) /* DMA Command Counter Register 0 */
#define LCD_DESSIZE0	(LCD_BASE + 0x6C) /* Foreground Size in Descriptor 0 Register*/

#define LCD_DA1		(LCD_BASE + 0x50) /* Descriptor Address Register 1 */
#define LCD_SA1		(LCD_BASE + 0x54) /* Source Address Register 1 */
#define LCD_FID1	(LCD_BASE + 0x58) /* Frame ID Register 1 */
#define LCD_CMD1	(LCD_BASE + 0x5c) /* DMA Command Register 1 */
#define LCD_OFFS1	(LCD_BASE + 0x70) /* DMA Offsize Register 1 */
#define LCD_PW1		(LCD_BASE + 0x74) /* DMA Page Width Register 1 */
#define LCD_CNUM1	(LCD_BASE + 0x78) /* DMA Command Counter Register 1 */
#define LCD_DESSIZE1	(LCD_BASE + 0x7C) /* Foreground Size in Descriptor 1 Register*/

#define LCD_DA0_PART2	(LCD_BASE + 0x1C0) /* Descriptor Address Register PART2 */
#define LCD_SA0_PART2	(LCD_BASE + 0x1C4) /* Source Address Register PART2 */
#define LCD_FID0_PART2	(LCD_BASE + 0x1C8) /* Frame ID Register PART2 */
#define LCD_CMD0_PART2	(LCD_BASE + 0x1CC) /* DMA Command Register PART2 */
#define LCD_OFFS0_PART2	(LCD_BASE + 0x1E0) /* DMA Offsize Register PART2 */
#define LCD_PW0_PART2	(LCD_BASE + 0x1E4) /* DMA Command Counter Register PART2 */
#define LCD_CNUM0_PART2	(LCD_BASE + 0x1E8) /* Foreground Size in Descriptor PART2 Register */
#define LCD_DESSIZE0_PART2	(LCD_BASE + 0x1EC) /*  */
#define LCD_PCFG	(LCD_BASE + 0x2C0)

#define REG_LCD_CFG	REG32(LCD_CFG)
#define REG_LCD_CTRL	REG32(LCD_CTRL)
#define REG_LCD_STATE	REG32(LCD_STATE)

#define REG_LCD_OSDC	REG16(LCD_OSDC)
#define REG_LCD_OSDCTRL	REG16(LCD_OSDCTRL)
#define REG_LCD_OSDS	REG16(LCD_OSDS)
#define REG_LCD_BGC	REG32(LCD_BGC)
#define REG_LCD_KEY0	REG32(LCD_KEY0)
#define REG_LCD_KEY1	REG32(LCD_KEY1)
#define REG_LCD_ALPHA	REG8(LCD_ALPHA)
#define REG_LCD_IPUR	REG32(LCD_IPUR)

#define REG_LCD_VAT	REG32(LCD_VAT)
#define REG_LCD_DAH	REG32(LCD_DAH)
#define REG_LCD_DAV	REG32(LCD_DAV)

#define REG_LCD_XYP0		REG32(LCD_XYP0)
#define REG_LCD_XYP0_PART2	REG32(LCD_XYP0_PART2)
#define REG_LCD_XYP1		REG32(LCD_XYP1)
#define REG_LCD_SIZE0		REG32(LCD_SIZE0)
#define REG_LCD_SIZE0_PART2	REG32(LCD_SIZE0_PART2)
#define REG_LCD_SIZE1		REG32(LCD_SIZE1)
			
#define REG_LCD_RGBC		REG16(LCD_RGBC)
			
#define REG_LCD_VSYNC		REG32(LCD_VSYNC)
#define REG_LCD_HSYNC		REG32(LCD_HSYNC)
#define REG_LCD_PS		REG32(LCD_PS)
#define REG_LCD_CLS		REG32(LCD_CLS)
#define REG_LCD_SPL		REG32(LCD_SPL)
#define REG_LCD_REV		REG32(LCD_REV)
#define REG_LCD_IID		REG32(LCD_IID)
#define REG_LCD_DA0		REG32(LCD_DA0)
#define REG_LCD_SA0		REG32(LCD_SA0)
#define REG_LCD_FID0		REG32(LCD_FID0)
#define REG_LCD_CMD0		REG32(LCD_CMD0)

#define REG_LCD_OFFS0		REG32(LCD_OFFS0)
#define REG_LCD_PW0		REG32(LCD_PW0)
#define REG_LCD_CNUM0		REG32(LCD_CNUM0)
#define REG_LCD_DESSIZE0	REG32(LCD_DESSIZE0)

#define REG_LCD_DA0_PART2	REG32(LCD_DA0_PART2)
#define REG_LCD_SA0_PART2	REG32(LCD_SA0_PART2)
#define REG_LCD_FID0_PART2	REG32(LCD_FID0_PART2)
#define REG_LCD_CMD0_PART2	REG32(LCD_CMD0_PART2)
#define REG_LCD_OFFS0_PART2	REG32(LCD_OFFS0_PART2)
#define REG_LCD_PW0_PART2	REG32(LCD_PW0_PART2)
#define REG_LCD_CNUM0_PART2	REG32(LCD_CNUM0_PART2)
#define REG_LCD_DESSIZE0_PART2	REG32(LCD_DESSIZE0_PART2)

#define REG_LCD_DA1		REG32(LCD_DA1)
#define REG_LCD_SA1		REG32(LCD_SA1)
#define REG_LCD_FID1		REG32(LCD_FID1)
#define REG_LCD_CMD1		REG32(LCD_CMD1)
#define REG_LCD_OFFS1		REG32(LCD_OFFS1)
#define REG_LCD_PW1		REG32(LCD_PW1)
#define REG_LCD_CNUM1		REG32(LCD_CNUM1)
#define REG_LCD_DESSIZE1	REG32(LCD_DESSIZE1)
#define REG_LCD_PCFG		REG32(LCD_PCFG)

/* LCD Configure Register */
#define LCD_CFG_LCDPIN_BIT	31  /* LCD pins selection */
#define LCD_CFG_LCDPIN_MASK	(0x1 << LCD_CFG_LCDPIN_BIT)
  #define LCD_CFG_LCDPIN_LCD	(0x0 << LCD_CFG_LCDPIN_BIT)
  #define LCD_CFG_LCDPIN_SLCD	(0x1 << LCD_CFG_LCDPIN_BIT)
#define LCD_CFG_TVEPEH		(1 << 30) /* TVE PAL enable extra halfline signal */
//#define LCD_CFG_FUHOLD		(1 << 29) /* hold pixel clock when outFIFO underrun */
#define LCD_CFG_NEWDES		(1 << 28) /* use new descripter. old: 4words, new:8words */
#define LCD_CFG_PALBP		(1 << 27) /* bypass data format and alpha blending */
#define LCD_CFG_TVEN		(1 << 26) /* indicate the terminal is lcd or tv */
#define LCD_CFG_RECOVER		(1 << 25) /* Auto recover when output fifo underrun */
#define LCD_CFG_DITHER		(1 << 24) /* Dither function */
#define LCD_CFG_PSM		(1 << 23) /* PS signal mode */
#define LCD_CFG_CLSM		(1 << 22) /* CLS signal mode */
#define LCD_CFG_SPLM		(1 << 21) /* SPL signal mode */
#define LCD_CFG_REVM		(1 << 20) /* REV signal mode */
#define LCD_CFG_HSYNM		(1 << 19) /* HSYNC signal mode */
#define LCD_CFG_PCLKM		(1 << 18) /* PCLK signal mode */
#define LCD_CFG_INVDAT		(1 << 17) /* Inverse output data */
#define LCD_CFG_SYNDIR_IN	(1 << 16) /* VSYNC&HSYNC direction */
#define LCD_CFG_PSP		(1 << 15) /* PS pin reset state */
#define LCD_CFG_CLSP		(1 << 14) /* CLS pin reset state */
#define LCD_CFG_SPLP		(1 << 13) /* SPL pin reset state */
#define LCD_CFG_REVP		(1 << 12) /* REV pin reset state */
#define LCD_CFG_HSP		(1 << 11) /* HSYNC polarity:0-active high,1-active low */
#define LCD_CFG_PCP		(1 << 10) /* PCLK polarity:0-rising,1-falling */
#define LCD_CFG_DEP		(1 << 9)  /* DE polarity:0-active high,1-active low */
#define LCD_CFG_VSP		(1 << 8)  /* VSYNC polarity:0-rising,1-falling */
#define LCD_CFG_MODE_TFT_18BIT 	(1 << 7)  /* 18bit TFT */
#define LCD_CFG_MODE_TFT_16BIT 	(0 << 7)  /* 16bit TFT */
#define LCD_CFG_MODE_TFT_24BIT 	(1 << 6)  /* 24bit TFT */
#define LCD_CFG_PDW_BIT		4  /* STN pins utilization */
#define LCD_CFG_PDW_MASK	(0x3 << LCD_CFG_PDW_BIT)
#define LCD_CFG_PDW_1		(0 << LCD_CFG_PDW_BIT) /* LCD_D[0] */
  #define LCD_CFG_PDW_2		(1 << LCD_CFG_PDW_BIT) /* LCD_D[0:1] */
  #define LCD_CFG_PDW_4		(2 << LCD_CFG_PDW_BIT) /* LCD_D[0:3]/LCD_D[8:11] */
  #define LCD_CFG_PDW_8		(3 << LCD_CFG_PDW_BIT) /* LCD_D[0:7]/LCD_D[8:15] */
#define LCD_CFG_MODE_BIT	0  /* Display Device Mode Select */
#define LCD_CFG_MODE_MASK	(0x0f << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_GENERIC_TFT	(0 << LCD_CFG_MODE_BIT) /* 16,18 bit TFT */
  #define LCD_CFG_MODE_SPECIAL_TFT_1	(1 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SPECIAL_TFT_2	(2 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SPECIAL_TFT_3	(3 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_NONINTER_CCIR656	(4 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_INTER_CCIR656	(6 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SINGLE_CSTN	(8 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SINGLE_MSTN	(9 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_DUAL_CSTN	(10 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_DUAL_MSTN	(11 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SERIAL_TFT	(12 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_LCM  		(13 << LCD_CFG_MODE_BIT)
  #define LCD_CFG_MODE_SLCD  		LCD_CFG_MODE_LCM

/* LCD Control Register */
#define LCD_CTRL_PINMD		(1 << 31) /* This register set Pin distribution in 16-bit parallel mode
					    0: 16-bit data correspond with LCD_D[15:0]
					    1: 16-bit data correspond with LCD_D[17:10], LCD_D[8:1] */
#define LCD_CTRL_BST_BIT	28  /* Burst Length Selection */
#define LCD_CTRL_BST_MASK	(0x7 << LCD_CTRL_BST_BIT)
  #define LCD_CTRL_BST_4	(0 << LCD_CTRL_BST_BIT) /* 4-word */
  #define LCD_CTRL_BST_8	(1 << LCD_CTRL_BST_BIT) /* 8-word */
  #define LCD_CTRL_BST_16	(2 << LCD_CTRL_BST_BIT) /* 16-word */
  #define LCD_CTRL_BST_32	(3 << LCD_CTRL_BST_BIT) /* 32-word */
  #define LCD_CTRL_BST_C16	(5 << LCD_CTRL_BST_BIT) /* 32-word */
  #define LCD_CTRL_BST_64	(4 << LCD_CTRL_BST_BIT) /* 32-word */
#define LCD_CTRL_RGB565		(0 << 27) /* RGB565 mode(foreground 0 in OSD mode) */
#define LCD_CTRL_RGB555		(1 << 27) /* RGB555 mode(foreground 0 in OSD mode) */
#define LCD_CTRL_OFUP		(1 << 26) /* Output FIFO underrun protection enable */
#define LCD_CTRL_FRC_BIT	24  /* STN FRC Algorithm Selection */
#define LCD_CTRL_FRC_MASK	(0x03 << LCD_CTRL_FRC_BIT)
  #define LCD_CTRL_FRC_16	(0 << LCD_CTRL_FRC_BIT) /* 16 grayscale */
  #define LCD_CTRL_FRC_4	(1 << LCD_CTRL_FRC_BIT) /* 4 grayscale */
  #define LCD_CTRL_FRC_2	(2 << LCD_CTRL_FRC_BIT) /* 2 grayscale */
#define LCD_CTRL_PDD_BIT	16  /* Load Palette Delay Counter */
#define LCD_CTRL_PDD_MASK	(0xff << LCD_CTRL_PDD_BIT)
//#define LCD_CTRL_VGA		(1 << 15) /* VGA interface enable */
#define LCD_CTRL_DACTE		(1 << 14) /* DAC loop back test */
#define LCD_CTRL_EOFM		(1 << 13) /* EOF interrupt mask */
#define LCD_CTRL_SOFM		(1 << 12) /* SOF interrupt mask */
#define LCD_CTRL_OFUM		(1 << 11) /* Output FIFO underrun interrupt mask */
#define LCD_CTRL_IFUM0		(1 << 10) /* Input FIFO 0 underrun interrupt mask */
#define LCD_CTRL_IFUM1		(1 << 9)  /* Input FIFO 1 underrun interrupt mask */
#define LCD_CTRL_LDDM		(1 << 8)  /* LCD disable done interrupt mask */
#define LCD_CTRL_QDM		(1 << 7)  /* LCD quick disable done interrupt mask */
#define LCD_CTRL_BEDN		(1 << 6)  /* Endian selection */
#define LCD_CTRL_PEDN		(1 << 5)  /* Endian in byte:0-msb first, 1-lsb first */
#define LCD_CTRL_DIS		(1 << 4)  /* Disable indicate bit */
#define LCD_CTRL_ENA		(1 << 3)  /* LCD enable bit */
#define LCD_CTRL_BPP_BIT	0  /* Bits Per Pixel */
#define LCD_CTRL_BPP_MASK	(0x07 << LCD_CTRL_BPP_BIT)
  #define LCD_CTRL_BPP_1	(0 << LCD_CTRL_BPP_BIT) /* 1 bpp */
  #define LCD_CTRL_BPP_2	(1 << LCD_CTRL_BPP_BIT) /* 2 bpp */
  #define LCD_CTRL_BPP_4	(2 << LCD_CTRL_BPP_BIT) /* 4 bpp */
  #define LCD_CTRL_BPP_8	(3 << LCD_CTRL_BPP_BIT) /* 8 bpp */
  #define LCD_CTRL_BPP_16	(4 << LCD_CTRL_BPP_BIT) /* 15/16 bpp */
  #define LCD_CTRL_BPP_18_24	(5 << LCD_CTRL_BPP_BIT) /* 18/24/32 bpp */
  #define LCD_CTRL_BPP_CMPS_24	(6 << LCD_CTRL_BPP_BIT) /* 24 compress bpp */
  #define LCD_CTRL_BPP_30	(7 << LCD_CTRL_BPP_BIT) /* 30 bpp */

/* LCD Status Register */
#define LCD_STATE_QD		(1 << 7) /* Quick Disable Done */
#define LCD_STATE_EOF		(1 << 5) /* EOF Flag */
#define LCD_STATE_SOF		(1 << 4) /* SOF Flag */
#define LCD_STATE_OFU		(1 << 3) /* Output FIFO Underrun */
#define LCD_STATE_IFU0		(1 << 2) /* Input FIFO 0 Underrun */
#define LCD_STATE_IFU1		(1 << 1) /* Input FIFO 1 Underrun */
#define LCD_STATE_LDD		(1 << 0) /* LCD Disabled */

/* OSD Configure Register */
#define LCD_OSDC_SOFM1		(1 << 15) /* Start of frame interrupt mask for foreground 1 */
#define LCD_OSDC_EOFM1		(1 << 14) /* End of frame interrupt mask for foreground 1 */
#define LCD_OSDC_OSDIV		(1 << 12)
#define LCD_OSDC_SOFM0		(1 << 11) /* Start of frame interrupt mask for foreground 0 */
#define LCD_OSDC_EOFM0		(1 << 10) /* End of frame interrupt mask for foreground 0 */
#if 0
#define LCD_OSDC_ENDM		(1 << 9) /* End of frame interrupt mask for panel. */
#define LCD_OSDC_F0DIVMD	(1 << 8) /* Divide Foreground 0 into 2 parts.
					  * 0: Foreground 0 only has one part. */
#define LCD_OSDC_F0P1EN		(1 << 7) /* 1: Foreground 0 PART1 is enabled.
					  * 0: Foreground 0 PART1 is disabled. */
#define LCD_OSDC_F0P2MD		(1 << 6) /* 1: PART 1&2 same level and same heighth
					  * 0: PART 1&2 have no same line */
#define LCD_OSDC_F0P2EN		(1 << 5) /* 1: Foreground 0 PART2 is enabled.
					  * 0: Foreground 0 PART2 is disabled.*/
#endif
#define LCD_OSDC_F1EN		(1 << 4) /* enable foreground 1 */
#define LCD_OSDC_F0EN		(1 << 3) /* enable foreground 0 */
#define LCD_OSDC_ALPHAEN		(1 << 2) /* enable alpha blending */
#define LCD_OSDC_ALPHAMD		(1 << 1) /* alpha blending mode */
#define LCD_OSDC_OSDEN		(1 << 0) /* OSD mode enable */

/* OSD Controll Register */
#define LCD_OSDCTRL_IPU		(1 << 15) /* input data from IPU */
#define LCD_OSDCTRL_RGB565	(0 << 4) /* foreground 1, 16bpp, 0-RGB565, 1-RGB555 */
#define LCD_OSDCTRL_RGB555	(1 << 4) /* foreground 1, 16bpp, 0-RGB565, 1-RGB555 */
#define LCD_OSDCTRL_CHANGES	(1 << 3) /* Change size flag */
#define LCD_OSDCTRL_OSDBPP_BIT	0 	 /* Bits Per Pixel of OSD Channel 1 */
#define LCD_OSDCTRL_OSDBPP_MASK	(0x7<<LCD_OSDCTRL_OSDBPP_BIT) 	 /* Bits Per Pixel of OSD Channel 1's MASK */
  #define LCD_OSDCTRL_OSDBPP_16	(4 << LCD_OSDCTRL_OSDBPP_BIT) /* RGB 15,16 bit*/
  #define LCD_OSDCTRL_OSDBPP_15_16	(4 << LCD_OSDCTRL_OSDBPP_BIT) /* RGB 15,16 bit*/
  #define LCD_OSDCTRL_OSDBPP_18_24	(5 << LCD_OSDCTRL_OSDBPP_BIT) /* RGB 18,24 bit*/
  #define LCD_OSDCTRL_OSDBPP_CMPS_24	(6 << LCD_OSDCTRL_OSDBPP_BIT) /* RGB compress 24 bit*/
  #define LCD_OSDCTRL_OSDBPP_30		(7 << LCD_OSDCTRL_OSDBPP_BIT) /* RGB 30 bit*/

/* OSD State Register */
#define LCD_OSDS_SOF1		(1 << 15) /* Start of frame flag for foreground 1 */
#define LCD_OSDS_EOF1		(1 << 14) /* End of frame flag for foreground 1 */
#define LCD_OSDS_SOF0		(1 << 11) /* Start of frame flag for foreground 0 */
#define LCD_OSDS_EOF0		(1 << 10) /* End of frame flag for foreground 0 */
#define LCD_OSDS_READY		(1 << 0)  /* Read for accept the change */

/* Background Color Register */
#define LCD_BGC_RED_OFFSET	(1 << 16)  /* Red color offset */
#define LCD_BGC_RED_MASK	(0xFF<<LCD_BGC_RED_OFFSET)
#define LCD_BGC_GREEN_OFFSET	(1 << 8)   /* Green color offset */
#define LCD_BGC_GREEN_MASK	(0xFF<<LCD_BGC_GREEN_OFFSET)
#define LCD_BGC_BLUE_OFFSET	(1 << 0)   /* Blue color offset */
#define LCD_BGC_BLUE_MASK	(0xFF<<LCD_BGC_BLUE_OFFSET)

/* Foreground Color Key Register 0,1(foreground 0, foreground 1) */
#define LCD_KEY_KEYEN		(1 << 31)   /* enable color key */
#define LCD_KEY_KEYMD		(1 << 30)   /* color key mode */
#define LCD_KEY_RED_OFFSET	16  /* Red color offset */
#define LCD_KEY_RED_MASK	(0xFF<<LCD_KEY_RED_OFFSET)
#define LCD_KEY_GREEN_OFFSET	8   /* Green color offset */
#define LCD_KEY_GREEN_MASK	(0xFF<<LCD_KEY_GREEN_OFFSET)
#define LCD_KEY_BLUE_OFFSET	0   /* Blue color offset */
#define LCD_KEY_BLUE_MASK	(0xFF<<LCD_KEY_BLUE_OFFSET)
#define LCD_KEY_MASK		(LCD_KEY_RED_MASK|LCD_KEY_GREEN_MASK|LCD_KEY_BLUE_MASK)

/* IPU Restart Register */
#define LCD_IPUR_IPUREN		(1 << 31)   /* IPU restart function enable*/
#define LCD_IPUR_IPURMASK	(0xFFFFFF)   /* IPU restart value mask*/

/* RGB Control Register */
#define LCD_RGBC_RGBDM		(1 << 15)   /* enable RGB Dummy data */
#define LCD_RGBC_DMM		(1 << 14)   /* RGB Dummy mode */
#define LCD_RGBC_YCC		(1 << 8)    /* RGB to YCC */
#define LCD_RGBC_ODDRGB_BIT	4	/* odd line serial RGB data arrangement */
#define LCD_RGBC_ODDRGB_MASK	(0x7<<LCD_RGBC_ODDRGB_BIT)
  #define LCD_RGBC_ODD_RGB	0
  #define LCD_RGBC_ODD_RBG	1
  #define LCD_RGBC_ODD_GRB	2
  #define LCD_RGBC_ODD_GBR	3
  #define LCD_RGBC_ODD_BRG	4
  #define LCD_RGBC_ODD_BGR	5
#define LCD_RGBC_EVENRGB_BIT	0	/* even line serial RGB data arrangement */
#define LCD_RGBC_EVENRGB_MASK	(0x7<<LCD_RGBC_EVENRGB_BIT)
  #define LCD_RGBC_EVEN_RGB	0
  #define LCD_RGBC_EVEN_RBG	1
  #define LCD_RGBC_EVEN_GRB	2
  #define LCD_RGBC_EVEN_GBR	3
  #define LCD_RGBC_EVEN_BRG	4
  #define LCD_RGBC_EVEN_BGR	5

/* Vertical Synchronize Register */
#define LCD_VSYNC_VPS_BIT	16  /* VSYNC pulse start in line clock, fixed to 0 */
#define LCD_VSYNC_VPS_MASK	(0xffff << LCD_VSYNC_VPS_BIT)
#define LCD_VSYNC_VPE_BIT	0   /* VSYNC pulse end in line clock */
#define LCD_VSYNC_VPE_MASK	(0xffff << LCD_VSYNC_VPS_BIT)

/* Horizontal Synchronize Register */
#define LCD_HSYNC_HPS_BIT	16  /* HSYNC pulse start position in dot clock */
#define LCD_HSYNC_HPS_MASK	(0xffff << LCD_HSYNC_HPS_BIT)
#define LCD_HSYNC_HPE_BIT	0   /* HSYNC pulse end position in dot clock */
#define LCD_HSYNC_HPE_MASK	(0xffff << LCD_HSYNC_HPE_BIT)

/* Virtual Area Setting Register */
#define LCD_VAT_HT_BIT		16  /* Horizontal Total size in dot clock */
#define LCD_VAT_HT_MASK		(0xffff << LCD_VAT_HT_BIT)
#define LCD_VAT_VT_BIT		0   /* Vertical Total size in dot clock */
#define LCD_VAT_VT_MASK		(0xffff << LCD_VAT_VT_BIT)

/* Display Area Horizontal Start/End Point Register */
#define LCD_DAH_HDS_BIT		16  /* Horizontal display area start in dot clock */
#define LCD_DAH_HDS_MASK	(0xffff << LCD_DAH_HDS_BIT)
#define LCD_DAH_HDE_BIT		0   /* Horizontal display area end in dot clock */
#define LCD_DAH_HDE_MASK	(0xffff << LCD_DAH_HDE_BIT)

/* Display Area Vertical Start/End Point Register */
#define LCD_DAV_VDS_BIT		16  /* Vertical display area start in line clock */
#define LCD_DAV_VDS_MASK	(0xffff << LCD_DAV_VDS_BIT)
#define LCD_DAV_VDE_BIT		0   /* Vertical display area end in line clock */
#define LCD_DAV_VDE_MASK	(0xffff << LCD_DAV_VDE_BIT)

/* Foreground XY Position Register */
#define LCD_XYP_YPOS_BIT	16  /* Y position bit of foreground 0 or 1 */
#define LCD_XYP_YPOS_MASK	(0xffff << LCD_XYP_YPOS_BIT)
#define LCD_XYP_XPOS_BIT	0   /* X position bit of foreground 0 or 1 */
#define LCD_XYP_XPOS_MASK	(0xffff << LCD_XYP_XPOS_BIT)

/* PS Signal Setting */
#define LCD_PS_PSS_BIT		16  /* PS signal start position in dot clock */
#define LCD_PS_PSS_MASK		(0xffff << LCD_PS_PSS_BIT)
#define LCD_PS_PSE_BIT		0   /* PS signal end position in dot clock */
#define LCD_PS_PSE_MASK		(0xffff << LCD_PS_PSE_BIT)

/* CLS Signal Setting */
#define LCD_CLS_CLSS_BIT	16  /* CLS signal start position in dot clock */
#define LCD_CLS_CLSS_MASK	(0xffff << LCD_CLS_CLSS_BIT)
#define LCD_CLS_CLSE_BIT	0   /* CLS signal end position in dot clock */
#define LCD_CLS_CLSE_MASK	(0xffff << LCD_CLS_CLSE_BIT)

/* SPL Signal Setting */
#define LCD_SPL_SPLS_BIT	16  /* SPL signal start position in dot clock */
#define LCD_SPL_SPLS_MASK	(0xffff << LCD_SPL_SPLS_BIT)
#define LCD_SPL_SPLE_BIT	0   /* SPL signal end position in dot clock */
#define LCD_SPL_SPLE_MASK	(0xffff << LCD_SPL_SPLE_BIT)

/* REV Signal Setting */
#define LCD_REV_REVS_BIT	16  /* REV signal start position in dot clock */
#define LCD_REV_REVS_MASK	(0xffff << LCD_REV_REVS_BIT)

/* DMA Command Register */
#define LCD_CMD_SOFINT		(1 << 31)
#define LCD_CMD_EOFINT		(1 << 30)
#define LCD_CMD_CMD		(1 << 29) /* indicate command in slcd mode */
#define LCD_CMD_PAL		(1 << 28)
#define LCD_CMD_UNCOMP_EN	(1 << 27)
#define LCD_CMD_UNCOMPRESS_WITHOUT_ALPHA	(1 << 26)
#define LCD_CMD_LEN_BIT		0
#define LCD_CMD_LEN_MASK	(0xffffff << LCD_CMD_LEN_BIT)

/* DMA Offsize Register 0,1 */

/* DMA Page Width Register 0,1 */

/* DMA Command Counter Register 0,1 */

/* Foreground 0,1 Size Register */
#define LCD_DESSIZE_HEIGHT_BIT	16  /* height of foreground 1 */
#define LCD_DESSIZE_HEIGHT_MASK	(0xffff << LCD_DESSIZE_HEIGHT_BIT)
#define LCD_DESSIZE_WIDTH_BIT	0  /* width of foreground 1 */
#define LCD_DESSIZE_WIDTH_MASK	(0xffff << LCD_DESSIZE_WIDTH_BIT)

/* Priority level threshold configure Register */
#define LCD_PCFG_LCD_PRI_MD	(1 << 31)
#define LCD_PCFG_HP_BST_BIT	28
#define LCD_PCFG_HP_BST_MASK	(0x7 << LCD_PCFG_HP_BST_BIT)
#define LCD_PCFG_PCFG2_BIT	8
#define LCD_PCFG_PCFG2_MASK	(0xf << LCD_PCFG_PCFG2_BIT)
#define LCD_PCFG_PCFG1_BIT	4
#define LCD_PCFG_PCFG1_MASK	(0xf << LCD_PCFG_PCFG1_BIT)
#define LCD_PCFG_PCFG0_BIT	0
#define LCD_PCFG_PCFG0_MASK	(0xf << LCD_PCFG_PCFG0_BIT)

#ifndef __MIPS_ASSEMBLER

/*************************************************************************
 * SLCD (Smart LCD Controller)
 *************************************************************************/
#define __slcd_set_data_18bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_18BIT )
#define __slcd_set_data_16bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_16BIT )
#define __slcd_set_data_8bit_x3() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_8BIT_x3 )
#define __slcd_set_data_8bit_x2() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_8BIT_x2 )
#define __slcd_set_data_8bit_x1() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_8BIT_x1 )
#define __slcd_set_data_24bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_24BIT )
#define __slcd_set_data_9bit_x2() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_DWIDTH_MASK) | SLCD_CFG_DWIDTH_9BIT_x2 )

#define __slcd_set_cmd_16bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_CWIDTH_MASK) | SLCD_CFG_CWIDTH_16BIT )
#define __slcd_set_cmd_8bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_CWIDTH_MASK) | SLCD_CFG_CWIDTH_8BIT )
#define __slcd_set_cmd_18bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_CWIDTH_MASK) | SLCD_CFG_CWIDTH_18BIT )
#define __slcd_set_cmd_24bit() \
  ( REG_SLCD_CFG = (REG_SLCD_CFG & ~SLCD_CFG_CWIDTH_MASK) | SLCD_CFG_CWIDTH_24BIT )

#define __slcd_set_cs_high()        ( REG_SLCD_CFG |= SLCD_CFG_CS_ACTIVE_HIGH )
#define __slcd_set_cs_low()         ( REG_SLCD_CFG &= ~SLCD_CFG_CS_ACTIVE_HIGH )

#define __slcd_set_rs_high()        ( REG_SLCD_CFG |= SLCD_CFG_RS_CMD_HIGH )
#define __slcd_set_rs_low()         ( REG_SLCD_CFG &= ~SLCD_CFG_RS_CMD_HIGH )

#define __slcd_set_clk_falling()    ( REG_SLCD_CFG &= ~SLCD_CFG_CLK_ACTIVE_RISING )
#define __slcd_set_clk_rising()     ( REG_SLCD_CFG |= SLCD_CFG_CLK_ACTIVE_RISING )

#define __slcd_set_parallel_type()  ( REG_SLCD_CFG &= ~SLCD_CFG_TYPE_SERIAL )
#define __slcd_set_serial_type()    ( REG_SLCD_CFG |= SLCD_CFG_TYPE_SERIAL )

/* SLCD Control Register */
#define __slcd_enable_dma()         ( REG_SLCD_CTRL |= SLCD_CTRL_DMA_EN )
#define __slcd_disable_dma()        ( REG_SLCD_CTRL &= ~SLCD_CTRL_DMA_EN )

/* SLCD Status Register */
#define __slcd_is_busy()            ( REG_SLCD_STATE & SLCD_STATE_BUSY )

/* SLCD Data Register */
#define __slcd_set_cmd_rs()         ( REG_SLCD_DATA |= SLCD_DATA_RS_COMMAND)
#define __slcd_set_data_rs()        ( REG_SLCD_DATA &= ~SLCD_DATA_RS_COMMAND)

/***************************************************************************
 * LCD
 ***************************************************************************/
#define __lcd_as_smart_lcd() 		( REG_LCD_CFG |= ( LCD_CFG_LCDPIN_SLCD | LCD_CFG_MODE_SLCD))
#define __lcd_as_general_lcd() 		( REG_LCD_CFG &= ~( LCD_CFG_LCDPIN_SLCD | LCD_CFG_MODE_SLCD))

#define __lcd_enable_tvepeh() 		( REG_LCD_CFG |= LCD_CFG_TVEPEH )
#define __lcd_disable_tvepeh() 		( REG_LCD_CFG &= ~LCD_CFG_TVEPEH )

#define __lcd_enable_fuhold() 		( REG_LCD_CFG |= LCD_CFG_FUHOLD )
#define __lcd_disable_fuhold() 		( REG_LCD_CFG &= ~LCD_CFG_FUHOLD )

#define __lcd_des_8word() 		( REG_LCD_CFG |= LCD_CFG_NEWDES )
#define __lcd_des_4word() 		( REG_LCD_CFG &= ~LCD_CFG_NEWDES )

#define __lcd_enable_bypass_pal() 	( REG_LCD_CFG |= LCD_CFG_PALBP )
#define __lcd_disable_bypass_pal() 	( REG_LCD_CFG &= ~LCD_CFG_PALBP )

#define __lcd_set_lcdpnl_term()		( REG_LCD_CFG |= LCD_CFG_TVEN )
#define __lcd_set_tv_term()		( REG_LCD_CFG &= ~LCD_CFG_TVEN )

#define __lcd_enable_auto_recover() 	( REG_LCD_CFG |= LCD_CFG_RECOVER )
#define __lcd_disable_auto_recover() 	( REG_LCD_CFG &= ~LCD_CFG_RECOVER )

#define __lcd_enable_dither() 	        ( REG_LCD_CFG |= LCD_CFG_DITHER )
#define __lcd_disable_dither() 	        ( REG_LCD_CFG &= ~LCD_CFG_DITHER )

#define __lcd_disable_ps_mode()	        ( REG_LCD_CFG |= LCD_CFG_PSM )
#define __lcd_enable_ps_mode()	        ( REG_LCD_CFG &= ~LCD_CFG_PSM )

#define __lcd_disable_cls_mode() 	( REG_LCD_CFG |= LCD_CFG_CLSM )
#define __lcd_enable_cls_mode()	        ( REG_LCD_CFG &= ~LCD_CFG_CLSM )

#define __lcd_disable_spl_mode() 	( REG_LCD_CFG |= LCD_CFG_SPLM )
#define __lcd_enable_spl_mode()	        ( REG_LCD_CFG &= ~LCD_CFG_SPLM )

#define __lcd_disable_rev_mode() 	( REG_LCD_CFG |= LCD_CFG_REVM )
#define __lcd_enable_rev_mode()	        ( REG_LCD_CFG &= ~LCD_CFG_REVM )

#define __lcd_disable_hsync_mode() 	( REG_LCD_CFG |= LCD_CFG_HSYNM )
#define __lcd_enable_hsync_mode()	( REG_LCD_CFG &= ~LCD_CFG_HSYNM )

#define __lcd_disable_pclk_mode() 	( REG_LCD_CFG |= LCD_CFG_PCLKM )
#define __lcd_enable_pclk_mode()	( REG_LCD_CFG &= ~LCD_CFG_PCLKM )

#define __lcd_normal_outdata()          ( REG_LCD_CFG &= ~LCD_CFG_INVDAT )
#define __lcd_inverse_outdata()         ( REG_LCD_CFG |= LCD_CFG_INVDAT )

#define __lcd_sync_input()              ( REG_LCD_CFG |= LCD_CFG_SYNDIR_IN )
#define __lcd_sync_output()             ( REG_LCD_CFG &= ~LCD_CFG_SYNDIR_IN )

#define __lcd_hsync_active_high()       ( REG_LCD_CFG &= ~LCD_CFG_HSP )
#define __lcd_hsync_active_low()        ( REG_LCD_CFG |= LCD_CFG_HSP )

#define __lcd_pclk_rising()             ( REG_LCD_CFG &= ~LCD_CFG_PCP )
#define __lcd_pclk_falling()            ( REG_LCD_CFG |= LCD_CFG_PCP )

#define __lcd_de_active_high()          ( REG_LCD_CFG &= ~LCD_CFG_DEP )
#define __lcd_de_active_low()           ( REG_LCD_CFG |= LCD_CFG_DEP )

#define __lcd_vsync_rising()            ( REG_LCD_CFG &= ~LCD_CFG_VSP )
#define __lcd_vsync_falling()           ( REG_LCD_CFG |= LCD_CFG_VSP )

#define __lcd_set_16_tftpnl() \
  ( REG_LCD_CFG = (REG_LCD_CFG & ~LCD_CFG_MODE_TFT_MASK) | LCD_CFG_MODE_TFT_16BIT )

#define __lcd_set_18_tftpnl() \
  ( REG_LCD_CFG = (REG_LCD_CFG & ~LCD_CFG_MODE_TFT_MASK) | LCD_CFG_MODE_TFT_18BIT )

#define __lcd_set_24_tftpnl()		( REG_LCD_CFG |= LCD_CFG_MODE_TFT_24BIT )

/* 
 * n=1,2,4,8 for single mono-STN 
 * n=4,8 for dual mono-STN
 */
#define __lcd_set_panel_datawidth(n) 		\
do { 						\
	REG_LCD_CFG &= ~LCD_CFG_PDW_MASK; 	\
	REG_LCD_CFG |= LCD_CFG_PDW_n##;		\
} while (0)

/* m = LCD_CFG_MODE_GENERUIC_TFT_xxx */
#define __lcd_set_panel_mode(m) 		\
do {						\
	REG_LCD_CFG &= ~LCD_CFG_MODE_MASK;	\
	REG_LCD_CFG |= (m);			\
} while(0)

/* n=4,8,16 */
#define __lcd_set_burst_length(n) 		\
do {						\
	REG_LCD_CTRL &= ~LCD_CTRL_BST_MASK;	\
	REG_LCD_CTRL |= LCD_CTRL_BST_n##;	\
} while (0)

#define __lcd_select_rgb565()		( REG_LCD_CTRL &= ~LCD_CTRL_RGB555 )
#define __lcd_select_rgb555()		( REG_LCD_CTRL |= LCD_CTRL_RGB555 )

#define __lcd_set_ofup()		( REG_LCD_CTRL |= LCD_CTRL_OFUP )
#define __lcd_clr_ofup()		( REG_LCD_CTRL &= ~LCD_CTRL_OFUP )

/* n=2,4,16 */
#define __lcd_set_stn_frc(n) 			\
do {						\
	REG_LCD_CTRL &= ~LCD_CTRL_FRC_MASK;	\
	REG_LCD_CTRL |= LCD_CTRL_FRC_n##;	\
} while (0)

#define __lcd_enable_eof_intr()		( REG_LCD_CTRL |= LCD_CTRL_EOFM )
#define __lcd_disable_eof_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_EOFM )

#define __lcd_enable_sof_intr()		( REG_LCD_CTRL |= LCD_CTRL_SOFM )
#define __lcd_disable_sof_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_SOFM )

#define __lcd_enable_ofu_intr()		( REG_LCD_CTRL |= LCD_CTRL_OFUM )
#define __lcd_disable_ofu_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_OFUM )

#define __lcd_enable_ifu0_intr()	( REG_LCD_CTRL |= LCD_CTRL_IFUM0 )
#define __lcd_disable_ifu0_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_IFUM0 )

#define __lcd_enable_ifu1_intr()	( REG_LCD_CTRL |= LCD_CTRL_IFUM1 )
#define __lcd_disable_ifu1_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_IFUM1 )

#define __lcd_enable_ldd_intr()		( REG_LCD_CTRL |= LCD_CTRL_LDDM )
#define __lcd_disable_ldd_intr()	( REG_LCD_CTRL &= ~LCD_CTRL_LDDM )

#define __lcd_enable_qd_intr()		( REG_LCD_CTRL |= LCD_CTRL_QDM )
#define __lcd_disable_qd_intr()		( REG_LCD_CTRL &= ~LCD_CTRL_QDM )

#define __lcd_reverse_byte_endian()	( REG_LCD_CTRL |= LCD_CTRL_BEDN )
#define __lcd_normal_byte_endian()	( REG_LCD_CTRL &= ~LCD_CTRL_BEDN )

#define __lcd_pixel_endian_little()	( REG_LCD_CTRL |= LCD_CTRL_PEDN )
#define __lcd_pixel_endian_big()	( REG_LCD_CTRL &= ~LCD_CTRL_PEDN )

#define __lcd_set_dis()			( REG_LCD_CTRL |= LCD_CTRL_DIS )
#define __lcd_clr_dis()			( REG_LCD_CTRL &= ~LCD_CTRL_DIS )

#define __lcd_set_ena()			( REG_LCD_CTRL |= LCD_CTRL_ENA )
#define __lcd_clr_ena()			( REG_LCD_CTRL &= ~LCD_CTRL_ENA )

/* n=1,2,4,8,16 */
#define __lcd_set_bpp(n) \
  ( REG_LCD_CTRL = (REG_LCD_CTRL & ~LCD_CTRL_BPP_MASK) | LCD_CTRL_BPP_##n )

/* LCD status register indication */

#define __lcd_quick_disable_done()	( REG_LCD_STATE & LCD_STATE_QD )
#define __lcd_disable_done()		( REG_LCD_STATE & LCD_STATE_LDD )
#define __lcd_infifo0_underrun()	( REG_LCD_STATE & LCD_STATE_IFU0 )
#define __lcd_infifo1_underrun()	( REG_LCD_STATE & LCD_STATE_IFU1 )
#define __lcd_outfifo_underrun()	( REG_LCD_STATE & LCD_STATE_OFU )
#define __lcd_start_of_frame()		( REG_LCD_STATE & LCD_STATE_SOF )
#define __lcd_end_of_frame()		( REG_LCD_STATE & LCD_STATE_EOF )

#define __lcd_clr_outfifounderrun()	( REG_LCD_STATE &= ~LCD_STATE_OFU )
#define __lcd_clr_sof()			( REG_LCD_STATE &= ~LCD_STATE_SOF )
#define __lcd_clr_eof()			( REG_LCD_STATE &= ~LCD_STATE_EOF )

/* OSD functions */
#define __lcd_enable_osd() 	(REG_LCD_OSDC |= LCD_OSDC_OSDEN)
#define __lcd_enable_f0() 	(REG_LCD_OSDC |= LCD_OSDC_F0EN)
#define __lcd_enable_f1()	(REG_LCD_OSDC |= LCD_OSDC_F1EN)
#define __lcd_enable_alpha() 	(REG_LCD_OSDC |= LCD_OSDC_ALPHAEN)
#define __lcd_enable_alphamd()	(REG_LCD_OSDC |= LCD_OSDC_ALPHAMD)

#define __lcd_disable_osd()	(REG_LCD_OSDC &= ~LCD_OSDC_OSDEN)
#define __lcd_disable_f0() 	(REG_LCD_OSDC &= ~LCD_OSDC_F0EN)
#define __lcd_disable_f1() 	(REG_LCD_OSDC &= ~LCD_OSDC_F1EN)
#define __lcd_disable_alpha()	(REG_LCD_OSDC &= ~LCD_OSDC_ALPHAEN)
#define __lcd_disable_alphamd()	(REG_LCD_OSDC &= ~LCD_OSDC_ALPHAMD)

/* OSD Controll Register */
#define __lcd_fg1_use_ipu() 		(REG_LCD_OSDCTRL |= LCD_OSDCTRL_IPU)
#define __lcd_fg1_use_dma_chan1() 	(REG_LCD_OSDCTRL &= ~LCD_OSDCTRL_IPU)
#define __lcd_fg1_unuse_ipu() 		__lcd_fg1_use_dma_chan1()
#define __lcd_osd_rgb555_mode()         ( REG_LCD_OSDCTRL |= LCD_OSDCTRL_RGB555 )
#define __lcd_osd_rgb565_mode()         ( REG_LCD_OSDCTRL &= ~LCD_OSDCTRL_RGB555 )
#define __lcd_osd_change_size()         ( REG_LCD_OSDCTRL |= LCD_OSDCTRL_CHANGES )
#define __lcd_osd_bpp_15_16() \
  ( REG_LCD_OSDCTRL = (REG_LCD_OSDCTRL & ~LCD_OSDCTRL_OSDBPP_MASK) | LCD_OSDCTRL_OSDBPP_15_16 )
#define __lcd_osd_bpp_18_24() \
  ( REG_LCD_OSDCTRL = (REG_LCD_OSDCTRL & ~LCD_OSDCTRL_OSDBPP_MASK) | LCD_OSDCTRL_OSDBPP_18_24 )

/* OSD State Register */
#define __lcd_start_of_fg1()		( REG_LCD_STATE & LCD_OSDS_SOF1 )
#define __lcd_end_of_fg1()		( REG_LCD_STATE & LCD_OSDS_EOF1 )
#define __lcd_start_of_fg0()		( REG_LCD_STATE & LCD_OSDS_SOF0 )
#define __lcd_end_of_fg0()		( REG_LCD_STATE & LCD_OSDS_EOF0 )
#define __lcd_change_is_rdy()		( REG_LCD_STATE & LCD_OSDS_READY )

/* Foreground Color Key Register 0,1(foreground 0, foreground 1) */
#define __lcd_enable_colorkey0()	(REG_LCD_KEY0 |= LCD_KEY_KEYEN)
#define __lcd_enable_colorkey1()	(REG_LCD_KEY1 |= LCD_KEY_KEYEN)
#define __lcd_enable_colorkey0_md() 	(REG_LCD_KEY0 |= LCD_KEY_KEYMD)
#define __lcd_enable_colorkey1_md() 	(REG_LCD_KEY1 |= LCD_KEY_KEYMD)
#define __lcd_set_colorkey0(key) 	(REG_LCD_KEY0 = (REG_LCD_KEY0&~0xFFFFFF)|(key))
#define __lcd_set_colorkey1(key) 	(REG_LCD_KEY1 = (REG_LCD_KEY1&~0xFFFFFF)|(key))

#define __lcd_disable_colorkey0() 	(REG_LCD_KEY0 &= ~LCD_KEY_KEYEN)
#define __lcd_disable_colorkey1() 	(REG_LCD_KEY1 &= ~LCD_KEY_KEYEN)
#define __lcd_disable_colorkey0_md() 	(REG_LCD_KEY0 &= ~LCD_KEY_KEYMD)
#define __lcd_disable_colorkey1_md() 	(REG_LCD_KEY1 &= ~LCD_KEY_KEYMD)

/* IPU Restart Register */
#define __lcd_enable_ipu_restart() 	(REG_LCD_IPUR |= LCD_IPUR_IPUREN)
#define __lcd_disable_ipu_restart() 	(REG_LCD_IPUR &= ~LCD_IPUR_IPUREN)
#define __lcd_set_ipu_restart_triger(n)	(REG_LCD_IPUR = (REG_LCD_IPUR&(~0xFFFFFF))|(n))

/* RGB Control Register */
#define __lcd_enable_rgb_dummy() 	(REG_LCD_RGBC |= LCD_RGBC_RGBDM)
#define __lcd_disable_rgb_dummy() 	(REG_LCD_RGBC &= ~LCD_RGBC_RGBDM)

#define __lcd_dummy_rgb() 	(REG_LCD_RGBC |= LCD_RGBC_DMM)
#define __lcd_rgb_dummy() 	(REG_LCD_RGBC &= ~LCD_RGBC_DMM)

#define __lcd_rgb2ycc() 	(REG_LCD_RGBC |= LCD_RGBC_YCC)
#define __lcd_notrgb2ycc() 	(REG_LCD_RGBC &= ~LCD_RGBC_YCC)

#define __lcd_odd_mode_rgb() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_RGB )
#define __lcd_odd_mode_rbg() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_RBG )
#define __lcd_odd_mode_grb() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_GRB)

#define __lcd_odd_mode_gbr() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_GBR)
#define __lcd_odd_mode_brg() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_BRG)
#define __lcd_odd_mode_bgr() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_ODDRGB_MASK) | LCD_RGBC_ODD_BGR)

#define __lcd_even_mode_rgb() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_RGB )
#define __lcd_even_mode_rbg() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_RBG )
#define __lcd_even_mode_grb() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_GRB)

#define __lcd_even_mode_gbr() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_GBR)
#define __lcd_even_mode_brg() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_BRG)
#define __lcd_even_mode_bgr() \
  ( REG_LCD_RGBC = (REG_LCD_RGBC & ~LCD_RGBC_EVENRGB_MASK) | LCD_RGBC_EVEN_BGR)

/* Vertical Synchronize Register */
#define __lcd_vsync_get_vps() \
  ( (REG_LCD_VSYNC & LCD_VSYNC_VPS_MASK) >> LCD_VSYNC_VPS_BIT )

#define __lcd_vsync_get_vpe() \
  ( (REG_LCD_VSYNC & LCD_VSYNC_VPE_MASK) >> LCD_VSYNC_VPE_BIT )
#define __lcd_vsync_set_vpe(n) 				\
do {							\
	REG_LCD_VSYNC &= ~LCD_VSYNC_VPE_MASK;		\
	REG_LCD_VSYNC |= (n) << LCD_VSYNC_VPE_BIT;	\
} while (0)

#define __lcd_hsync_get_hps() \
  ( (REG_LCD_HSYNC & LCD_HSYNC_HPS_MASK) >> LCD_HSYNC_HPS_BIT )
#define __lcd_hsync_set_hps(n) 				\
do {							\
	REG_LCD_HSYNC &= ~LCD_HSYNC_HPS_MASK;		\
	REG_LCD_HSYNC |= (n) << LCD_HSYNC_HPS_BIT;	\
} while (0)

#define __lcd_hsync_get_hpe() \
  ( (REG_LCD_HSYNC & LCD_HSYNC_HPE_MASK) >> LCD_VSYNC_HPE_BIT )
#define __lcd_hsync_set_hpe(n) 				\
do {							\
	REG_LCD_HSYNC &= ~LCD_HSYNC_HPE_MASK;		\
	REG_LCD_HSYNC |= (n) << LCD_HSYNC_HPE_BIT;	\
} while (0)

#define __lcd_vat_get_ht() \
  ( (REG_LCD_VAT & LCD_VAT_HT_MASK) >> LCD_VAT_HT_BIT )
#define __lcd_vat_set_ht(n) 				\
do {							\
	REG_LCD_VAT &= ~LCD_VAT_HT_MASK;		\
	REG_LCD_VAT |= (n) << LCD_VAT_HT_BIT;		\
} while (0)

#define __lcd_vat_get_vt() \
  ( (REG_LCD_VAT & LCD_VAT_VT_MASK) >> LCD_VAT_VT_BIT )
#define __lcd_vat_set_vt(n) 				\
do {							\
	REG_LCD_VAT &= ~LCD_VAT_VT_MASK;		\
	REG_LCD_VAT |= (n) << LCD_VAT_VT_BIT;		\
} while (0)

#define __lcd_dah_get_hds() \
  ( (REG_LCD_DAH & LCD_DAH_HDS_MASK) >> LCD_DAH_HDS_BIT )
#define __lcd_dah_set_hds(n) 				\
do {							\
	REG_LCD_DAH &= ~LCD_DAH_HDS_MASK;		\
	REG_LCD_DAH |= (n) << LCD_DAH_HDS_BIT;		\
} while (0)

#define __lcd_dah_get_hde() \
  ( (REG_LCD_DAH & LCD_DAH_HDE_MASK) >> LCD_DAH_HDE_BIT )
#define __lcd_dah_set_hde(n) 				\
do {							\
	REG_LCD_DAH &= ~LCD_DAH_HDE_MASK;		\
	REG_LCD_DAH |= (n) << LCD_DAH_HDE_BIT;		\
} while (0)

#define __lcd_dav_get_vds() \
  ( (REG_LCD_DAV & LCD_DAV_VDS_MASK) >> LCD_DAV_VDS_BIT )
#define __lcd_dav_set_vds(n) 				\
do {							\
	REG_LCD_DAV &= ~LCD_DAV_VDS_MASK;		\
	REG_LCD_DAV |= (n) << LCD_DAV_VDS_BIT;		\
} while (0)

#define __lcd_dav_get_vde() \
  ( (REG_LCD_DAV & LCD_DAV_VDE_MASK) >> LCD_DAV_VDE_BIT )
#define __lcd_dav_set_vde(n) 				\
do {							\
	REG_LCD_DAV &= ~LCD_DAV_VDE_MASK;		\
	REG_LCD_DAV |= (n) << LCD_DAV_VDE_BIT;		\
} while (0)

/* DMA Command Register */
#define __lcd_cmd0_set_sofint()		( REG_LCD_CMD0 |= LCD_CMD_SOFINT )
#define __lcd_cmd0_clr_sofint()		( REG_LCD_CMD0 &= ~LCD_CMD_SOFINT )
#define __lcd_cmd1_set_sofint()		( REG_LCD_CMD1 |= LCD_CMD_SOFINT )
#define __lcd_cmd1_clr_sofint()		( REG_LCD_CMD1 &= ~LCD_CMD_SOFINT )

#define __lcd_cmd0_set_eofint()		( REG_LCD_CMD0 |= LCD_CMD_EOFINT )
#define __lcd_cmd0_clr_eofint()		( REG_LCD_CMD0 &= ~LCD_CMD_EOFINT )
#define __lcd_cmd1_set_eofint()		( REG_LCD_CMD1 |= LCD_CMD_EOFINT )
#define __lcd_cmd1_clr_eofint()		( REG_LCD_CMD1 &= ~LCD_CMD_EOFINT )

#define __lcd_cmd0_set_pal()		( REG_LCD_CMD0 |= LCD_CMD_PAL )
#define __lcd_cmd0_clr_pal()		( REG_LCD_CMD0 &= ~LCD_CMD_PAL )

#define __lcd_cmd0_get_len() \
  ( (REG_LCD_CMD0 & LCD_CMD_LEN_MASK) >> LCD_CMD_LEN_BIT )
#define __lcd_cmd1_get_len() \
  ( (REG_LCD_CMD1 & LCD_CMD_LEN_MASK) >> LCD_CMD_LEN_BIT )

#endif /* __MIPS_ASSEMBLER */

/*
 * Motion compensation module(MC) address definition
 */
#define	MC_BASE		0xb3250000

/*
 * MC registers offset address definition
 */
#define MC_MCCR_OFFSET		(0x00)	/* rw, 32, 0x???????? */
#define MC_MCSR_OFFSET		(0x04)	/* rw, 32, 0x???????? */
#define MC_MCRBAR_OFFSET	(0x08)	/* rw, 32, 0x???????? */
#define MC_MCT1LFCR_OFFSET	(0x0c)	/* rw, 32, 0x???????? */
#define MC_MCT2LFCR_OFFSET	(0x10)	/* rw, 32, 0x???????? */
#define MC_MCCBAR_OFFSET	(0x14)	/* rw, 32, 0x???????? */
#define MC_MCIIR_OFFSET		(0x18)	/* rw, 32, 0x???????? */
#define MC_MCSIR_OFFSET		(0x1c)	/* rw, 32, 0x???????? */
#define MC_MCT1MFCR_OFFSET	(0x20)	/* rw, 32, 0x???????? */
#define MC_MCT2MFCR_OFFSET	(0x24)	/* rw, 32, 0x???????? */
#define MC_MCFGIR_OFFSET	(0x28)	/* rw, 32, 0x???????? */
#define MC_MCFCIR_OFFSET	(0x2c)	/* rw, 32, 0x???????? */
#define MC_MCRNDTR_OFFSET	(0x40)	/* rw, 32, 0x???????? */

#define MC_MC2CR_OFFSET		(0x8000)	/* rw, 32, 0x???????? */
#define MC_MC2SR_OFFSET		(0x8004)	/* rw, 32, 0x???????? */
#define MC_MC2RBAR_OFFSET	(0x8008)	/* rw, 32, 0x???????? */
#define MC_MC2CBAR_OFFSET	(0x800c)	/* rw, 32, 0x???????? */
#define MC_MC2IIR_OFFSET	(0x8010)	/* rw, 32, 0x???????? */
#define MC_MC2TFCR_OFFSET	(0x8014)	/* rw, 32, 0x???????? */
#define MC_MC2SIR_OFFSET	(0x8018)	/* rw, 32, 0x???????? */
#define MC_MC2FCIR_OFFSET	(0x801c)	/* rw, 32, 0x???????? */
#define MC_MC2RNDTR_OFFSET	(0x8040)	/* rw, 32, 0x???????? */

/*
 * MC registers address definition
 */
#define MC_MCCR		(MC_BASE + MC_MCCR_OFFSET)
#define MC_MCSR		(MC_BASE + MC_MCSR_OFFSET)
#define MC_MCRBAR	(MC_BASE + MC_MCRBAR_OFFSET)
#define MC_MCT1LFCR	(MC_BASE + MC_MCT1LFCR_OFFSET)
#define MC_MCT2LFCR	(MC_BASE + MC_MCT2LFCR_OFFSET)
#define MC_MCCBAR	(MC_BASE + MC_MCCBAR_OFFSET)
#define MC_MCIIR	(MC_BASE + MC_MCIIR_OFFSET)
#define MC_MCSIR	(MC_BASE + MC_MCSIR_OFFSET)
#define MC_MCT1MFCR	(MC_BASE + MC_MCT1MFCR_OFFSET)
#define MC_MCT2MFCR	(MC_BASE + MC_MCT2MFCR_OFFSET)
#define MC_MCFGIR	(MC_BASE + MC_MCFGIR_OFFSET)
#define MC_MCFCIR	(MC_BASE + MC_MCFCIR_OFFSET)
#define MC_MCRNDTR	(MC_BASE + MC_MCRNDTR_OFFSET)

#define MC_MC2CR	(MC_BASE + MC_MC2CR_OFFSET)
#define MC_MC2SR	(MC_BASE + MC_MC2SR_OFFSET)
#define MC_MC2RBAR	(MC_BASE + MC_MC2RBAR_OFFSET)
#define MC_MC2CBAR	(MC_BASE + MC_MC2CBAR_OFFSET)
#define MC_MC2IIR	(MC_BASE + MC_MC2IIR_OFFSET)
#define MC_MC2TFCR	(MC_BASE + MC_MC2TFCR_OFFSET)
#define MC_MC2SIR	(MC_BASE + MC_MC2SIR_OFFSET)
#define MC_MC2FCIR	(MC_BASE + MC_MC2FCIR_OFFSET)
#define MC_MC2RNDTR	(MC_BASE + MC_MC2RNDTR_OFFSET)

/*
 * MC registers common define
 */

/* MC Control Register(MCCR) */
#define MCCR_RETE		BIT16
#define MCCR_DIPE		BIT7
#define MCCR_CKGEN		BIT6
#define MCCR_FDDEN		BIT5
#define MCCR_DINSE		BIT3
#define MCCR_FAE		BIT2
#define MCCR_RST		BIT1
#define MCCR_CHEN		BIT0

#define MCCR_FDDPGN_LSB		8
#define MCCR_FDDPGN_MASK	BITS_H2L(15, MCCR_FDDPGN_LSB)

/* MC Status Register(MCSR) */
#define MCSR_DLEND		BIT1
#define MCSR_BKLEND		BIT0

#ifndef __MIPS_ASSEMBLER

#define REG_MC_MCCR		REG32(REG_MC_MCCR)
#define REG_MC_MCSR             REG32(REG_MC_MCSR)
#define REG_MC_MCRBAR           REG32(REG_MC_MCRBAR)
#define REG_MC_MCT1LFCR         REG32(REG_MC_MCT1LFCR)
#define REG_MC_MCT2LFCR         REG32(REG_MC_MCT2LFCR)
#define REG_MC_MCCBAR           REG32(REG_MC_MCCBAR)
#define REG_MC_MCIIR            REG32(REG_MC_MCIIR)
#define REG_MC_MCSIR            REG32(REG_MC_MCSIR)
#define REG_MC_MCT1MFCR         REG32(REG_MC_MCT1MFCR)
#define REG_MC_MCT2MFCR         REG32(REG_MC_MCT2MFCR)
#define REG_MC_MCFGIR           REG32(REG_MC_MCFGIR)
#define REG_MC_MCFCIR           REG32(REG_MC_MCFCIR)
#define REG_MC_MCRNDTR          REG32(REG_MC_MCRNDTR)

#define REG_MC_MC2CR            REG32(REG_MC_MC2CR)
#define REG_MC_MC2SR            REG32(REG_MC_MC2SR)
#define REG_MC_MC2RBAR          REG32(REG_MC_MC2RBAR)
#define REG_MC_MC2CBAR          REG32(REG_MC_MC2CBAR)
#define REG_MC_MC2IIR           REG32(REG_MC_MC2IIR)
#define REG_MC_MC2TFCR          REG32(REG_MC_MC2TFCR)
#define REG_MC_MC2SIR           REG32(REG_MC_MC2SIR)
#define REG_MC_MC2FCIR          REG32(REG_MC_MC2FCIR)
#define REG_MC_MC2RNDTR         REG32(REG_MC_MC2RNDTR)

#endif /* __MIPS_ASSEMBLER */

#define	MDMAC_BASE	0xB3030000 /* Memory Copy DMAC */

/*************************************************************************
 * MDMAC (MEM Copy DMA Controller)
 *************************************************************************/

/* m is the DMA controller index (0, 1), n is the DMA channel index (0 - 11) */

#define MDMAC_DSAR(n)		(MDMAC_BASE + (0x00 + (n) * 0x20)) /* DMA source address */
#define MDMAC_DTAR(n)  		(MDMAC_BASE + (0x04 + (n) * 0x20)) /* DMA target address */
#define MDMAC_DTCR(n)  		(MDMAC_BASE + (0x08 + (n) * 0x20)) /* DMA transfer count */
#define MDMAC_DRSR(n)  		(MDMAC_BASE + (0x0c + (n) * 0x20)) /* DMA request source */
#define MDMAC_DCCSR(n) 		(MDMAC_BASE + (0x10 + (n) * 0x20)) /* DMA control/status */
#define MDMAC_DCMD(n)  		(MDMAC_BASE + (0x14 + (n) * 0x20)) /* DMA command */
#define MDMAC_DDA(n)   		(MDMAC_BASE + (0x18 + (n) * 0x20)) /* DMA descriptor address */
#define MDMAC_DSD(n)   		(MDMAC_BASE + (0x1c + (n) * 0x20)) /* DMA Stride Address */

#define MDMAC_DMACR		(MDMAC_BASE + 0x0300) /* DMA control register */
#define MDMAC_DMAIPR		(MDMAC_BASE + 0x0304) /* DMA interrupt pending */
#define MDMAC_DMADBR		(MDMAC_BASE + 0x0308) /* DMA doorbell */
#define MDMAC_DMADBSR		(MDMAC_BASE + 0x030C) /* DMA doorbell set */
#define MDMAC_DMACKE  		(MDMAC_BASE + 0x0310)
#define MDMAC_DMACKES  		(MDMAC_BASE + 0x0314)
#define MDMAC_DMACKEC  		(MDMAC_BASE + 0x0318)

#define REG_MDMAC_DSAR(n)	REG32(MDMAC_DSAR((n)))
#define REG_MDMAC_DTAR(n)	REG32(MDMAC_DTAR((n)))
#define REG_MDMAC_DTCR(n)	REG32(MDMAC_DTCR((n)))
#define REG_MDMAC_DRSR(n)	REG32(MDMAC_DRSR((n)))
#define REG_MDMAC_DCCSR(n)	REG32(MDMAC_DCCSR((n)))
#define REG_MDMAC_DCMD(n)	REG32(MDMAC_DCMD((n)))
#define REG_MDMAC_DDA(n)	REG32(MDMAC_DDA((n)))
#define REG_MDMAC_DSD(n)        REG32(MDMAC_DSD(n))
#define REG_MDMAC_DMACR		REG32(MDMAC_DMACR)
#define REG_MDMAC_DMAIPR	REG32(MDMAC_DMAIPR)
#define REG_MDMAC_DMADBR	REG32(MDMAC_DMADBR)
#define REG_MDMAC_DMADBSR	REG32(MDMAC_DMADBSR)
#define REG_MDMAC_DMACKE     	REG32(MDMAC_DMACKE)
#define REG_MDMAC_DMACKES     	REG32(MDMAC_DMACKES)
#define REG_MDMAC_DMACKEC     	REG32(MDMAC_DMACKEC)

// DMA control register
#define DMAC_MDMACR_HLT		(1 << 3)  /* DMA halt flag */
#define DMAC_MDMACR_AR		(1 << 2)  /* address error flag */
#define DMAC_MDMACR_DMAE	(1 << 0)  /* DMA enable bit */

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * Mem Copy DMAC
 ***************************************************************************/

#define __mdmac_enable_module() \
	( REG_MDMAC_DMACR |= DMAC_MDMACR_DMAE )
#define __mdmac_disable_module() \
	( REG_MDMAC_DMACR &= ~DMAC_MDMACR_DMAE )

#define __mdmac_test_halt_error ( REG_MDMAC_DMACR & DMAC_MDMACR_HLT )
#define __mdmac_test_addr_error ( REG_MDMAC_DMACR & DMAC_MDMACR_AR )

#define __mdmac_channel_enable_clk \
	REG_MDMAC_DMACKES = 1 << (n);

#define __mdmac_channel_disable_clk \
	REG_MDMAC_DMACKEC = 1 << (n);

#define __mdmac_enable_descriptor(n) \
  ( REG_MDMAC_DCCSR((n)) &= ~DMAC_DCCSR_NDES )
#define __mdmac_disable_descriptor(n) \
  ( REG_MDMAC_DCCSR((n)) |= DMAC_DCCSR_NDES )

#define __mdmac_enable_channel(n)                 \
do {                                             \
	REG_MDMAC_DCCSR((n)) |= DMAC_DCCSR_EN;    \
} while (0)
#define __mdmac_disable_channel(n)                \
do {                                             \
	REG_MDMAC_DCCSR((n)) &= ~DMAC_DCCSR_EN;   \
} while (0)
#define __mdmac_channel_enabled(n) \
  ( REG_MDMAC_DCCSR((n)) & DMAC_DCCSR_EN )

#define __mdmac_channel_enable_irq(n) \
  ( REG_MDMAC_DCMD((n)) |= DMAC_DCMD_TIE )
#define __mdmac_channel_disable_irq(n) \
  ( REG_DMAC_DCMD((n)) &= ~DMAC_DCMD_TIE )

#define __mdmac_channel_transmit_halt_detected(n) \
  (  REG_MDMAC_DCCSR((n)) & DMAC_DCCSR_HLT )
#define __mdmac_channel_transmit_end_detected(n) \
  (  REG_MDMAC_DCCSR((n)) & DMAC_DCCSR_TT )
#define __mdmac_channel_address_error_detected(n) \
  (  REG_DMAC_DCCSR((n)) & DMAC_DCCSR_AR )
#define __mdmac_channel_count_terminated_detected(n) \
  (  REG_MDMAC_DCCSR((n)) & DMAC_DCCSR_CT )
#define __mdmac_channel_descriptor_invalid_detected(n) \
  (  REG_MDMAC_DCCSR((n)) & DMAC_DCCSR_INV )

#define __mdmac_channel_clear_transmit_halt(n)				\
	do {								\
		/* clear both channel halt error and globle halt error */ \
		REG_MDMAC_DCCSR(n) &= ~DMAC_DCCSR_HLT;			\
		REG_MDMAC_DMACR &= ~DMAC_DMACR_HLT;	\
	} while (0)
#define __mdmac_channel_clear_transmit_end(n) \
  (  REG_MDMAC_DCCSR(n) &= ~DMAC_DCCSR_TT )
#define __mdmac_channel_clear_address_error(n)				\
	do {								\
		REG_MDMAC_DDA(n) = 0; /* clear descriptor address register */ \
		REG_MDMAC_DSAR(n) = 0; /* clear source address register */ \
		REG_MDMAC_DTAR(n) = 0; /* clear target address register */ \
		/* clear both channel addr error and globle address error */ \
		REG_MDMAC_DCCSR(n) &= ~DMAC_DCCSR_AR;			\
		REG_MDMAC_DMACR &= ~DMAC_DMACR_AR;	\
	} while (0)
#define __mdmac_channel_clear_count_terminated(n) \
  (  REG_MDMAC_DCCSR((n)) &= ~DMAC_DCCSR_CT )
#define __mdmac_channel_clear_descriptor_invalid(n) \
  (  REG_MDMAC_DCCSR((n)) &= ~DMAC_DCCSR_INV )

#define __mdmac_channel_set_transfer_unit_32bit(n)	\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DS_32BIT;	\
} while (0)

#define __mdmac_channel_set_transfer_unit_16bit(n)	\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DS_16BIT;	\
} while (0)

#define __mdmac_channel_set_transfer_unit_8bit(n)	\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DS_8BIT;	\
} while (0)

#define __mdmac_channel_set_transfer_unit_16byte(n)	\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DS_16BYTE;	\
} while (0)

#define __mdmac_channel_set_transfer_unit_32byte(n)	\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DS_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DS_32BYTE;	\
} while (0)

/* w=8,16,32 */
#define __mdmac_channel_set_dest_port_width(n,w)		\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DWDH_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DWDH_##w;	\
} while (0)

/* w=8,16,32 */
#define __mdmac_channel_set_src_port_width(n,w)		\
do {							\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_SWDH_MASK;	\
	REG_MDMAC_DCMD((n)) |= DMAC_DCMD_SWDH_##w;	\
} while (0)

/* v=0-15 */
#define __mdmac_channel_set_rdil(n,v)				\
do {								\
	REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_RDIL_MASK;		\
	REG_MDMAC_DCMD((n) |= ((v) << DMAC_DCMD_RDIL_BIT);	\
} while (0)

#define __mdmac_channel_dest_addr_fixed(n) \
	(REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_DAI)
#define __mdmac_channel_dest_addr_increment(n) \
	(REG_MDMAC_DCMD((n)) |= DMAC_DCMD_DAI)

#define __mdmac_channel_src_addr_fixed(n) \
	(REG_MDMAC_DCMD((n)) &= ~DMAC_DCMD_SAI)
#define __mdmac_channel_src_addr_increment(n) \
	(REG_MDMAC_DCMD((n)) |= DMAC_DCMD_SAI)

#define __mdmac_channel_set_doorbell(n)	\
	(REG_MDMAC_DMADBSR = (1 << (n)))

#define __mdmac_channel_irq_detected(n)  (REG_MDMAC_DMAIPR & (1 << (n)))
#define __mdmac_channel_ack_irq(n)       (REG_MDMAC_DMAIPR &= ~(1 <<(n)))

static __inline__ int __mdmac_get_irq(void)
{
	int i;
	for (i = 0; i < MAX_MDMA_NUM; i++)
		if (__mdmac_channel_irq_detected(i))
			return i;
	return -1;
}

#endif /* __MIPS_ASSEMBLER */

/*
 * Motion estimation module(ME) address definition
 */
#define	ME_BASE		0xb3260000

/*
 * ME registers offset address definition
 */
#define ME_MECR_OFFSET		(0x00)  /* rw, 32, 0x???????0 */
#define ME_MERBAR_OFFSET	(0x04)  /* rw, 32, 0x???????? */
#define ME_MECBAR_OFFSET	(0x08)  /* rw, 32, 0x???????? */
#define ME_MEDAR_OFFSET		(0x0c)  /* rw, 32, 0x???????? */
#define ME_MERFSR_OFFSET	(0x10)  /* rw, 32, 0x???????? */
#define ME_MECFSR_OFFSET	(0x14)  /* rw, 32, 0x???????? */
#define ME_MEDFSR_OFFSET	(0x18)  /* rw, 32, 0x???????? */
#define ME_MESR_OFFSET		(0x1c)  /* rw, 32, 0x???????? */
#define ME_MEMR_OFFSET		(0x20)  /* rw, 32, 0x???????? */
#define ME_MEFR_OFFSET		(0x24)  /* rw, 32, 0x???????? */

/*
 * ME registers address definition
 */
#define ME_MECR		(ME_BASE + ME_MECR_OFFSET)
#define ME_MERBAR	(ME_BASE + ME_MERBAR_OFFSET)
#define ME_MECBAR       (ME_BASE + ME_MECBAR_OFFSET)
#define ME_MEDAR        (ME_BASE + ME_MEDAR_OFFSET)
#define ME_MERFSR       (ME_BASE + ME_MERFSR_OFFSET)
#define ME_MECFSR       (ME_BASE + ME_MECFSR_OFFSET)
#define ME_MEDFSR       (ME_BASE + ME_MEDFSR_OFFSET)
#define ME_MESR         (ME_BASE + ME_MESR_OFFSET)
#define ME_MEMR         (ME_BASE + ME_MEMR_OFFSET)
#define ME_MEFR         (ME_BASE + ME_MEFR_OFFSET)

/*
 * ME registers common define
 */

/* ME control register(MECR) */
#define MECR_FLUSH		BIT2
#define MECR_RESET		BIT1
#define MECR_ENABLE		BIT0

/* ME settings register(MESR) */
#define MESR_GATE_LSB		16
#define MESR_GATE_MASK		BITS_H2L(31, MESR_GATE_LSB)

#define MESR_NUM_LSB		0
#define MESR_NUM_MASK		BITS_H2L(5, MESR_NUM_LSB)

/* ME MVD register(MEMR) */
#define MEMR_MVDY_LSB		16
#define MESR_MVDY_MASK		BITS_H2L(31, MEMR_MVDY_LSB)

#define MEMR_MVDX_LSB		0
#define MESR_MVDX_MASK		BITS_H2L(15, MEMR_MVDX_LSB)

/* ME flag register(MEFR) */
#define MEFR_INTRA		BIT1
#define MEFR_COMPLETED		BIT0

#ifndef __MIPS_ASSEMBLER

#define REG_ME_MECR	REG32(ME_MECR)
#define REG_ME_MERBAR	REG32(ME_MERBAR)
#define REG_ME_MECBAR	REG32(ME_MECBAR)
#define REG_ME_MEDAR	REG32(ME_MEDAR)
#define REG_ME_MERFSR	REG32(ME_MERFSR)
#define REG_ME_MECFSR	REG32(ME_MECFSR)
#define REG_ME_MEDFSR	REG32(ME_MEDFSR)
#define REG_ME_MESR	REG32(ME_MESR)
#define REG_ME_MEMR	REG32(ME_MEMR)
#define REG_ME_MEFR	REG32(ME_MEFR)

#endif /* __MIPS_ASSEMBLER */

#define	MSC0_BASE	0xB0021000
#define	MSC1_BASE	0xB0022000
#define	MSC2_BASE	0xB0023000

/*************************************************************************
 * MSC
 ************************************************************************/
/* n = 0, 1 (MSC0, MSC1) */
#define	MSC_STRPCL(n)		(MSC0_BASE + (n)*0x1000 + 0x000)
#define	MSC_STAT(n)		(MSC0_BASE + (n)*0x1000 + 0x004)
#define	MSC_CLKRT(n)		(MSC0_BASE + (n)*0x1000 + 0x008)
#define	MSC_CMDAT(n)		(MSC0_BASE + (n)*0x1000 + 0x00C)
#define	MSC_RESTO(n)		(MSC0_BASE + (n)*0x1000 + 0x010)
#define	MSC_RDTO(n)		(MSC0_BASE + (n)*0x1000 + 0x014)
#define	MSC_BLKLEN(n)		(MSC0_BASE + (n)*0x1000 + 0x018)
#define	MSC_NOB(n)		(MSC0_BASE + (n)*0x1000 + 0x01C)
#define	MSC_SNOB(n)		(MSC0_BASE + (n)*0x1000 + 0x020)
#define	MSC_IMASK(n)		(MSC0_BASE + (n)*0x1000 + 0x024)
#define	MSC_IREG(n)		(MSC0_BASE + (n)*0x1000 + 0x028)
#define	MSC_CMD(n)		(MSC0_BASE + (n)*0x1000 + 0x02C)
#define	MSC_ARG(n)		(MSC0_BASE + (n)*0x1000 + 0x030)
#define	MSC_RES(n)		(MSC0_BASE + (n)*0x1000 + 0x034)
#define	MSC_RXFIFO(n)		(MSC0_BASE + (n)*0x1000 + 0x038)
#define	MSC_TXFIFO(n)		(MSC0_BASE + (n)*0x1000 + 0x03C)
#define	MSC_LPM(n)		(MSC0_BASE + (n)*0x1000 + 0x040)

#define	REG_MSC_STRPCL(n)	REG16(MSC_STRPCL(n))
#define	REG_MSC_STAT(n)		REG32(MSC_STAT(n))
#define	REG_MSC_CLKRT(n)	REG16(MSC_CLKRT(n))
#define	REG_MSC_CMDAT(n)	REG32(MSC_CMDAT(n))
#define	REG_MSC_RESTO(n)	REG16(MSC_RESTO(n))
#define	REG_MSC_RDTO(n)		REG32(MSC_RDTO(n))
#define	REG_MSC_BLKLEN(n)	REG16(MSC_BLKLEN(n))
#define	REG_MSC_NOB(n)		REG16(MSC_NOB(n))
#define	REG_MSC_SNOB(n)		REG16(MSC_SNOB(n))
#define	REG_MSC_IMASK(n)	REG32(MSC_IMASK(n))
#define	REG_MSC_IREG(n)		REG16(MSC_IREG(n))
#define	REG_MSC_CMD(n)		REG8(MSC_CMD(n))
#define	REG_MSC_ARG(n)		REG32(MSC_ARG(n))
#define	REG_MSC_RES(n)		REG16(MSC_RES(n))
#define	REG_MSC_RXFIFO(n)	REG32(MSC_RXFIFO(n))
#define	REG_MSC_TXFIFO(n)	REG32(MSC_TXFIFO(n))
#define	REG_MSC_LPM(n)		REG32(MSC_LPM(n))

/* MSC Clock and Control Register (MSC_STRPCL) */
#define MSC_STRPCL_SEND_CCSD		(1 << 15) /*send command completion signal disable to ceata */
#define MSC_STRPCL_SEND_AS_CCSD		(1 << 14) /*send internally generated stop after sending ccsd */
#define MSC_STRPCL_EXIT_MULTIPLE	(1 << 7)
#define MSC_STRPCL_EXIT_TRANSFER	(1 << 6)
#define MSC_STRPCL_START_READWAIT	(1 << 5)
#define MSC_STRPCL_STOP_READWAIT	(1 << 4)
#define MSC_STRPCL_RESET		(1 << 3)
#define MSC_STRPCL_START_OP		(1 << 2)
#define MSC_STRPCL_CLOCK_CONTROL_BIT	0
#define MSC_STRPCL_CLOCK_CONTROL_MASK	(0x3 << MSC_STRPCL_CLOCK_CONTROL_BIT)
  #define MSC_STRPCL_CLOCK_CONTROL_STOP	  (0x1 << MSC_STRPCL_CLOCK_CONTROL_BIT) /* Stop MMC/SD clock */
  #define MSC_STRPCL_CLOCK_CONTROL_START  (0x2 << MSC_STRPCL_CLOCK_CONTROL_BIT) /* Start MMC/SD clock */

/* MSC Status Register (MSC_STAT) */
#define MSC_STAT_AUTO_CMD_DONE		(1 << 31) /*12 is internally generated by controller has finished */
#define MSC_STAT_IS_RESETTING		(1 << 15)
#define MSC_STAT_SDIO_INT_ACTIVE	(1 << 14)
#define MSC_STAT_PRG_DONE		(1 << 13)
#define MSC_STAT_DATA_TRAN_DONE		(1 << 12)
#define MSC_STAT_END_CMD_RES		(1 << 11)
#define MSC_STAT_DATA_FIFO_AFULL	(1 << 10)
#define MSC_STAT_IS_READWAIT		(1 << 9)
#define MSC_STAT_CLK_EN			(1 << 8)
#define MSC_STAT_DATA_FIFO_FULL		(1 << 7)
#define MSC_STAT_DATA_FIFO_EMPTY	(1 << 6)
#define MSC_STAT_CRC_RES_ERR		(1 << 5)
#define MSC_STAT_CRC_READ_ERROR		(1 << 4)
#define MSC_STAT_CRC_WRITE_ERROR_BIT	2
#define MSC_STAT_CRC_WRITE_ERROR_MASK	(0x3 << MSC_STAT_CRC_WRITE_ERROR_BIT)
  #define MSC_STAT_CRC_WRITE_ERROR_NO		(0 << MSC_STAT_CRC_WRITE_ERROR_BIT) /* No error on transmission of data */
  #define MSC_STAT_CRC_WRITE_ERROR		(1 << MSC_STAT_CRC_WRITE_ERROR_BIT) /* Card observed erroneous transmission of data */
  #define MSC_STAT_CRC_WRITE_ERROR_NOSTS	(2 << MSC_STAT_CRC_WRITE_ERROR_BIT) /* No CRC status is sent back */
#define MSC_STAT_TIME_OUT_RES		(1 << 1)
#define MSC_STAT_TIME_OUT_READ		(1 << 0)

/* MSC Bus Clock Control Register (MSC_CLKRT) */
#define	MSC_CLKRT_CLK_DIV_BIT		14
#define	MSC_CLKRT_CLK_DIV_MASK		(0x3 << MSC_CLKRT_CLK_DIV_BIT)
#define MSC_CLKRT_CLK_SRC_DIV_1		(0x0 << MSC_CLKRT_CLK_DIV_BIT) /* CLK_SRC */
#define MSC_CLKRT_CLK_SRC_DIV_2		(0x1 << MSC_CLKRT_CLK_DIV_BIT) /* 1/2 of CLK_SRC */
#define MSC_CLKRT_CLK_SRC_DIV_3		(0x2 << MSC_CLKRT_CLK_DIV_BIT) /* 1/3 of CLK_SRC */
#define MSC_CLKRT_CLK_SRC_DIV_4		(0x3 << MSC_CLKRT_CLK_DIV_BIT) /* 1/4 of CLK_SRC */

#define	MSC_CLKRT_CLK_RATE_BIT		0
#define	MSC_CLKRT_CLK_RATE_MASK		(0x7 << MSC_CLKRT_CLK_RATE_BIT)
#define MSC_CLKRT_CLK_RATE_DIV_1	(0x0 << MSC_CLKRT_CLK_RATE_BIT) /* CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_2	(0x1 << MSC_CLKRT_CLK_RATE_BIT) /* 1/2 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_4	(0x2 << MSC_CLKRT_CLK_RATE_BIT) /* 1/4 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_8	(0x3 << MSC_CLKRT_CLK_RATE_BIT) /* 1/8 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_16	(0x4 << MSC_CLKRT_CLK_RATE_BIT) /* 1/16 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_32	(0x5 << MSC_CLKRT_CLK_RATE_BIT) /* 1/32 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_64	(0x6 << MSC_CLKRT_CLK_RATE_BIT) /* 1/64 of CLK_SRC */
#define MSC_CLKRT_CLK_RATE_DIV_128	(0x7 << MSC_CLKRT_CLK_RATE_BIT) /* 1/128 of CLK_SRC */

/* MSC Command Sequence Control Register (MSC_CMDAT) */
#define	MSC_CMDAT_CCS_EXPECTED		(1 << 31) /* interrupts are enabled in ce-ata */
#define	MSC_CMDAT_READ_CEATA		(1 << 30)
#define	MSC_CMDAT_SDIO_PRDT		(1 << 17) /* exact 2 cycle */
#define	MSC_CMDAT_SEND_AS_STOP		(1 << 16)
#define	MSC_CMDAT_RTRG_BIT		14
#define MSC_CMDAT_RTRG_EQUALT_8	        (0x0 << MSC_CMDAT_RTRG_BIT) /*reset value*/
  #define MSC_CMDAT_RTRG_EQUALT_16	(0x1 << MSC_CMDAT_RTRG_BIT) 
  #define MSC_CMDAT_RTRG_EQUALT_24	(0x2 << MSC_CMDAT_RTRG_BIT)

#define	MSC_CMDAT_TTRG_BIT		12
#define MSC_CMDAT_TTRG_LESS_8		(0x0 << MSC_CMDAT_TTRG_BIT) /*reset value*/
  #define MSC_CMDAT_TTRG_LESS_16	(0x1 << MSC_CMDAT_TTRG_BIT) 
  #define MSC_CMDAT_TTRG_LESS_24	(0x2 << MSC_CMDAT_TTRG_BIT)
#define	MSC_CMDAT_STOP_ABORT		(1 << 11)
#define	MSC_CMDAT_BUS_WIDTH_BIT		9
#define	MSC_CMDAT_BUS_WIDTH_MASK	(0x3 << MSC_CMDAT_BUS_WIDTH_BIT)
  #define MSC_CMDAT_BUS_WIDTH_1BIT	(0x0 << MSC_CMDAT_BUS_WIDTH_BIT) /* 1-bit data bus */
  #define MSC_CMDAT_BUS_WIDTH_4BIT	(0x2 << MSC_CMDAT_BUS_WIDTH_BIT) /* 4-bit data bus */
  #define MSC_CMDAT_BUS_WIDTH_8BIT	(0x3 << MSC_CMDAT_BUS_WIDTH_BIT) /* 8-bit data bus */
#define	MSC_CMDAT_DMA_EN		(1 << 8)
#define	MSC_CMDAT_INIT			(1 << 7)
#define	MSC_CMDAT_BUSY			(1 << 6)
#define	MSC_CMDAT_STREAM_BLOCK		(1 << 5)
#define	MSC_CMDAT_WRITE			(1 << 4)
#define	MSC_CMDAT_READ			(0 << 4)
#define	MSC_CMDAT_DATA_EN		(1 << 3)
#define	MSC_CMDAT_RESPONSE_BIT	0
#define	MSC_CMDAT_RESPONSE_MASK	(0x7 << MSC_CMDAT_RESPONSE_BIT)
  #define MSC_CMDAT_RESPONSE_NONE (0x0 << MSC_CMDAT_RESPONSE_BIT) /* No response */
  #define MSC_CMDAT_RESPONSE_R1	  (0x1 << MSC_CMDAT_RESPONSE_BIT) /* Format R1 and R1b */
  #define MSC_CMDAT_RESPONSE_R2	  (0x2 << MSC_CMDAT_RESPONSE_BIT) /* Format R2 */
  #define MSC_CMDAT_RESPONSE_R3	  (0x3 << MSC_CMDAT_RESPONSE_BIT) /* Format R3 */
  #define MSC_CMDAT_RESPONSE_R4	  (0x4 << MSC_CMDAT_RESPONSE_BIT) /* Format R4 */
  #define MSC_CMDAT_RESPONSE_R5	  (0x5 << MSC_CMDAT_RESPONSE_BIT) /* Format R5 */
  #define MSC_CMDAT_RESPONSE_R6	  (0x6 << MSC_CMDAT_RESPONSE_BIT) /* Format R6 */
  #define MSC_CMDAT_RESRONSE_R7   (0x7 << MSC_CMDAT_RESPONSE_BIT) /* Format R7 */

#define	CMDAT_DMA_EN	(1 << 8)
#define	CMDAT_INIT	(1 << 7)
#define	CMDAT_BUSY	(1 << 6)
#define	CMDAT_STREAM	(1 << 5)
#define	CMDAT_WRITE	(1 << 4)
#define	CMDAT_DATA_EN	(1 << 3)

/* MSC Interrupts Mask Register (MSC_IMASK) */
#define	MSC_IMASK_AUTO_CMD_DONE		(1 << 15)
#define	MSC_IMASK_DATA_FIFO_FULL	(1 << 14)
#define	MSC_IMASK_DATA_FIFO_EMP		(1 << 13)
#define	MSC_IMASK_CRC_RES_ERR		(1 << 12)
#define	MSC_IMASK_CRC_READ_ERR		(1 << 11)
#define	MSC_IMASK_CRC_WRITE_ERR		(1 << 10)
#define	MSC_IMASK_TIME_OUT_RES		(1 << 9)
#define	MSC_IMASK_TIME_OUT_READ		(1 << 8)
#define	MSC_IMASK_SDIO			(1 << 7)
#define	MSC_IMASK_TXFIFO_WR_REQ		(1 << 6)
#define	MSC_IMASK_RXFIFO_RD_REQ		(1 << 5)
#define	MSC_IMASK_END_CMD_RES		(1 << 2)
#define	MSC_IMASK_PRG_DONE		(1 << 1)
#define	MSC_IMASK_DATA_TRAN_DONE	(1 << 0)

/* MSC Interrupts Status Register (MSC_IREG) */
#define	MSC_IREG_AUTO_CMD_DONE		(1 << 15)
#define	MSC_IREG_DATA_FIFO_FULL		(1 << 14)
#define	MSC_IREG_DATA_FIFO_EMP		(1 << 13)
#define	MSC_IREG_CRC_RES_ERR		(1 << 12)
#define	MSC_IREG_CRC_READ_ERR		(1 << 11)
#define	MSC_IREG_CRC_WRITE_ERR		(1 << 10)
#define	MSC_IREG_TIMEOUT_RES		(1 << 9)
#define	MSC_IREG_TIMEOUT_READ		(1 << 8)
#define	MSC_IREG_SDIO			(1 << 7)
#define	MSC_IREG_TXFIFO_WR_REQ		(1 << 6)
#define	MSC_IREG_RXFIFO_RD_REQ		(1 << 5)
#define	MSC_IREG_END_CMD_RES		(1 << 2)
#define	MSC_IREG_PRG_DONE		(1 << 1)
#define	MSC_IREG_DATA_TRAN_DONE		(1 << 0)

/* MSC Low Power Mode Register (MSC_LPM) */
#define	MSC_SET_HISPD			(1 << 31)
#define	MSC_SET_LPM			(1 << 0)

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * MSC
 ***************************************************************************/
/* n = 0, 1 (MSC0, MSC1) */

#define __msc_start_op(n) \
	( REG_MSC_STRPCL(n) = MSC_STRPCL_START_OP | MSC_STRPCL_CLOCK_CONTROL_START )

#define __msc_set_resto(n, to)  	( REG_MSC_RESTO(n) = to )
#define __msc_set_rdto(n, to)   	( REG_MSC_RDTO(n) = to )
#define __msc_set_cmd(n, cmd)   	( REG_MSC_CMD(n) = cmd )
#define __msc_set_arg(n, arg)   	( REG_MSC_ARG(n) = arg )
#define __msc_set_nob(n, nob)      	( REG_MSC_NOB(n) = nob )
#define __msc_get_nob(n)        	( REG_MSC_NOB(n) )
#define __msc_set_blklen(n, len)        ( REG_MSC_BLKLEN(n) = len )
#define __msc_set_cmdat(n, cmdat)   	( REG_MSC_CMDAT(n) = cmdat )

#define __msc_set_cmdat_bus_width1(n) 			\
do { 							\
	REG_MSC_CMDAT(n) &= ~MSC_CMDAT_BUS_WIDTH_MASK; 	\
	REG_MSC_CMDAT(n) |= MSC_CMDAT_BUS_WIDTH_1BIT; 	\
} while(0)

#define __msc_set_cmdat_bus_width4(n) 			\
do { 							\
	REG_MSC_CMDAT(n) &= ~MSC_CMDAT_BUS_WIDTH_MASK; 	\
	REG_MSC_CMDAT(n) |= MSC_CMDAT_BUS_WIDTH_4BIT; 	\
} while(0)

#define __msc_set_cmdat_dma_en(n)       ( REG_MSC_CMDAT(n) |= MSC_CMDAT_DMA_EN )
#define __msc_set_cmdat_init(n) 	( REG_MSC_CMDAT(n) |= MSC_CMDAT_INIT )
#define __msc_set_cmdat_busy(n) 	( REG_MSC_CMDAT(n) |= MSC_CMDAT_BUSY )
#define __msc_set_cmdat_stream(n)       ( REG_MSC_CMDAT(n) |= MSC_CMDAT_STREAM_BLOCK )
#define __msc_set_cmdat_block(n)        ( REG_MSC_CMDAT(n) &= ~MSC_CMDAT_STREAM_BLOCK )
#define __msc_set_cmdat_read(n) 	( REG_MSC_CMDAT(n) &= ~MSC_CMDAT_WRITE )
#define __msc_set_cmdat_write(n)        ( REG_MSC_CMDAT(n) |= MSC_CMDAT_WRITE )
#define __msc_set_cmdat_data_en(n)      ( REG_MSC_CMDAT(n) |= MSC_CMDAT_DATA_EN )

/* r is MSC_CMDAT_RESPONSE_FORMAT_Rx or MSC_CMDAT_RESPONSE_FORMAT_NONE */
#define __msc_set_cmdat_res_format(n, r)				\
do { 								\
	REG_MSC_CMDAT(n) &= ~MSC_CMDAT_RESPONSE_MASK; 	\
	REG_MSC_CMDAT(n) |= (r); 					\
} while(0)

#define __msc_clear_cmdat(n) \
  REG_MSC_CMDAT(n) &= ~( MSC_CMDAT_STOP_ABORT | MSC_CMDAT_DMA_EN | MSC_CMDAT_INIT| \
  MSC_CMDAT_BUSY | MSC_CMDAT_STREAM_BLOCK | MSC_CMDAT_WRITE | \
  MSC_CMDAT_DATA_EN | MSC_CMDAT_RESPONSE_MASK )

#define __msc_get_imask(n) 		( REG_MSC_IMASK(n) )
#define __msc_mask_all_intrs(n) 	( REG_MSC_IMASK(n) = 0xff )
#define __msc_unmask_all_intrs(n) 	( REG_MSC_IMASK(n) = 0x00 )
#define __msc_mask_rd(n) 		( REG_MSC_IMASK(n) |= MSC_IMASK_RXFIFO_RD_REQ )
#define __msc_unmask_rd(n) 		( REG_MSC_IMASK(n) &= ~MSC_IMASK_RXFIFO_RD_REQ )
#define __msc_mask_wr(n) 		( REG_MSC_IMASK(n) |= MSC_IMASK_TXFIFO_WR_REQ )
#define __msc_unmask_wr(n) 		( REG_MSC_IMASK(n) &= ~MSC_IMASK_TXFIFO_WR_REQ )
#define __msc_mask_endcmdres(n) 	( REG_MSC_IMASK(n) |= MSC_IMASK_END_CMD_RES )
#define __msc_unmask_endcmdres(n) 	( REG_MSC_IMASK(n) &= ~MSC_IMASK_END_CMD_RES )
#define __msc_mask_datatrandone(n) 	( REG_MSC_IMASK(n) |= MSC_IMASK_DATA_TRAN_DONE )
#define __msc_unmask_datatrandone(n) 	( REG_MSC_IMASK(n) &= ~MSC_IMASK_DATA_TRAN_DONE )
#define __msc_mask_prgdone(n) 		( REG_MSC_IMASK(n) |= MSC_IMASK_PRG_DONE )
#define __msc_unmask_prgdone(n) 	( REG_MSC_IMASK(n) &= ~MSC_IMASK_PRG_DONE )

/* m=0,1,2,3,4,5,6,7 */
#define __msc_set_clkrt(n, m) 	\
do { 				\
	REG_MSC_CLKRT(n) = m;	\
} while(0)

#define __msc_get_ireg(n) 	        	( REG_MSC_IREG(n) )
#define __msc_ireg_rd(n) 	        	( REG_MSC_IREG(n) & MSC_IREG_RXFIFO_RD_REQ )
#define __msc_ireg_wr(n) 	        	( REG_MSC_IREG(n) & MSC_IREG_TXFIFO_WR_REQ )
#define __msc_ireg_end_cmd_res(n)       	( REG_MSC_IREG(n) & MSC_IREG_END_CMD_RES )
#define __msc_ireg_data_tran_done(n)     	( REG_MSC_IREG(n) & MSC_IREG_DATA_TRAN_DONE )
#define __msc_ireg_prg_done(n) 	        	( REG_MSC_IREG(n) & MSC_IREG_PRG_DONE )
#define __msc_ireg_clear_end_cmd_res(n)         ( REG_MSC_IREG(n) = MSC_IREG_END_CMD_RES )
#define __msc_ireg_clear_data_tran_done(n)      ( REG_MSC_IREG(n) = MSC_IREG_DATA_TRAN_DONE )
#define __msc_ireg_clear_prg_done(n)     	( REG_MSC_IREG(n) = MSC_IREG_PRG_DONE )

#define __msc_get_stat(n) 		( REG_MSC_STAT(n) )
#define __msc_stat_not_end_cmd_res(n) 	( (REG_MSC_STAT(n) & MSC_STAT_END_CMD_RES) == 0)
#define __msc_stat_crc_err(n) \
  ( REG_MSC_STAT(n) & (MSC_STAT_CRC_RES_ERR | MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR_YES) )
#define __msc_stat_res_crc_err(n) 	( REG_MSC_STAT(n) & MSC_STAT_CRC_RES_ERR )
#define __msc_stat_rd_crc_err(n) 	( REG_MSC_STAT(n) & MSC_STAT_CRC_READ_ERROR )
#define __msc_stat_wr_crc_err(n) 	( REG_MSC_STAT(n) & MSC_STAT_CRC_WRITE_ERROR_YES )
#define __msc_stat_resto_err(n) 	( REG_MSC_STAT(n) & MSC_STAT_TIME_OUT_RES )
#define __msc_stat_rdto_err(n) 		( REG_MSC_STAT(n) & MSC_STAT_TIME_OUT_READ )

#define __msc_rd_resfifo(n) 		( REG_MSC_RES(n) )
#define __msc_rd_rxfifo(n)  		( REG_MSC_RXFIFO(n) )
#define __msc_wr_txfifo(n, v)  		( REG_MSC_TXFIFO(n) = v )

#define __msc_reset(n) 						\
do { 								\
	REG_MSC_STRPCL(n) = MSC_STRPCL_RESET;			\
 	while (REG_MSC_STAT(n) & MSC_STAT_IS_RESETTING);		\
} while (0)

#define __msc_start_clk(n) 					\
do { 								\
	REG_MSC_STRPCL(n) = MSC_STRPCL_CLOCK_CONTROL_START;	\
} while (0)

#define __msc_stop_clk(n) 					\
do { 								\
	REG_MSC_STRPCL(n) = MSC_STRPCL_CLOCK_CONTROL_STOP;	\
} while (0)

#define MMC_CLK 19169200
#define SD_CLK  24576000

/* msc_clk should little than pclk and little than clk retrieve from card */
#define __msc_calc_clk_divisor(type,dev_clk,msc_clk,lv)		\
do {								\
	unsigned int rate, pclk, i;				\
	pclk = dev_clk;						\
	rate = type?SD_CLK:MMC_CLK;				\
  	if (msc_clk && msc_clk < pclk)				\
    		pclk = msc_clk;					\
	i = 0;							\
  	while (pclk < rate)					\
    	{							\
      		i ++;						\
      		rate >>= 1;					\
    	}							\
  	lv = i;							\
} while(0)

/* divide rate to little than or equal to 400kHz */
#define __msc_calc_slow_clk_divisor(type, lv)			\
do {								\
	unsigned int rate, i;					\
	rate = (type?SD_CLK:MMC_CLK)/1000/400;			\
	i = 0;							\
	while (rate > 0)					\
    	{							\
      		rate >>= 1;					\
      		i ++;						\
    	}							\
  	lv = i;							\
} while(0)

#endif /* __MIPS_ASSEMBLER */

#define NEMC_BASE	0xB3410000

/*************************************************************************
 * NEMC (External Memory Controller for NAND)
 *************************************************************************/

#define NEMC_NFCSR	(NEMC_BASE + 0x50) /* NAND Flash Control/Status Register */
#define NEMC_SMCR	(NEMC_BASE + 0x14)  /* Static Memory Control Register 1 */
#define NEMC_PNCR 	(NEMC_BASE + 0x100)
#define NEMC_PNDR 	(NEMC_BASE + 0x104)
#define NEMC_BITCNT	(NEMC_BASE + 0x108)

#define REG_NEMC_NFCSR	REG32(NEMC_NFCSR)
#define REG_NEMC_SMCR1	REG32(NEMC_SMCR)
#define REG_NEMC_PNCR 	REG32(NEMC_PNCR)
#define REG_NEMC_PNDR 	REG32(NEMC_PNDR)
#define REG_NEMC_BITCNT	REG32(NEMC_BITCNT)

/* NAND Flash Control/Status Register */
#define NEMC_NFCSR_NFCE4	(1 << 7) /* NAND Flash Enable */
#define NEMC_NFCSR_NFE4		(1 << 6) /* NAND Flash FCE# Assertion Enable */
#define NEMC_NFCSR_NFCE3	(1 << 5)
#define NEMC_NFCSR_NFE3		(1 << 4)
#define NEMC_NFCSR_NFCE2	(1 << 3)
#define NEMC_NFCSR_NFE2		(1 << 2)
#define NEMC_NFCSR_NFCE1	(1 << 1)
#define NEMC_NFCSR_NFE1		(1 << 0)

#define UDC_BASE	0xB3440000

/*************************************************************************
 * USB Device
 *************************************************************************/
#define USB_BASE  UDC_BASE

#define USB_FADDR		(USB_BASE + 0x00) /* Function Address 8-bit */
#define USB_POWER		(USB_BASE + 0x01) /* Power Managemetn 8-bit */
#define USB_INTRIN		(USB_BASE + 0x02) /* Interrupt IN 16-bit */
#define USB_INTROUT		(USB_BASE + 0x04) /* Interrupt OUT 16-bit */
#define USB_INTRINE		(USB_BASE + 0x06) /* Intr IN enable 16-bit */
#define USB_INTROUTE		(USB_BASE + 0x08) /* Intr OUT enable 16-bit */
#define USB_INTRUSB		(USB_BASE + 0x0a) /* Interrupt USB 8-bit */
#define USB_INTRUSBE		(USB_BASE + 0x0b) /* Interrupt USB Enable 8-bit */
#define USB_FRAME		(USB_BASE + 0x0c) /* Frame number 16-bit */
#define USB_INDEX		(USB_BASE + 0x0e) /* Index register 8-bit */
#define USB_TESTMODE		(USB_BASE + 0x0f) /* USB test mode 8-bit */

#define USB_CSR0		(USB_BASE + 0x12) /* EP0 CSR 16-bit */
#define USB_COUNT0		(USB_BASE + 0x18) /* EP0 OUT FIFO count 8-bit */

#define USB_INMAXP		(USB_BASE + 0x10) /* EP1-15 IN Max Pkt Size 16-bit */
#define USB_INCSR		(USB_BASE + 0x12) /* EP1-15 IN CSR LSB 8/16bit */
#define USB_INCSRH		(USB_BASE + 0x13) /* EP1-15 IN CSR MSB 8-bit */
#define USB_OUTMAXP		(USB_BASE + 0x14) /* EP1-15 OUT Max Pkt Size 16-bit */
#define USB_OUTCSR		(USB_BASE + 0x16) /* EP1-15 OUT CSR LSB 8/16bit */
#define USB_OUTCSRH		(USB_BASE + 0x17) /* EP1-15 OUT CSR MSB 8-bit */
#define USB_OUTCOUNT		(USB_BASE + 0x18) /* EP1-15 OUT FIFO count 16-bit */

#define USB_FIFO_EP(n)		(USB_BASE + (n)*4 + 0x20)

#define USB_EPINFO		(USB_BASE + 0x78) /* Endpoint information */
#define USB_RAMINFO		(USB_BASE + 0x79) /* RAM information */

#define USB_INTR		(USB_BASE + 0x200) /* DMA pending interrupts */
#define USB_CNTL(n)		(USB_BASE + (n)*0x10 + 0x204) /* DMA channel n control */
#define USB_ADDR(n)		(USB_BASE + (n)*0x10 + 0x208) /* DMA channel n AHB memory addr */
#define USB_COUNT(n)		(USB_BASE + (n)*0x10 + 0x20c) /* DMA channel n byte count */

/* Power register bit masks */
#define USB_POWER_SUSPENDM	0x01
#define USB_POWER_RESUME	0x04
#define USB_POWER_HSMODE	0x10
#define USB_POWER_HSENAB	0x20
#define USB_POWER_SOFTCONN	0x40

/* Interrupt register bit masks */
#define USB_INTR_SUSPEND	0x01
#define USB_INTR_RESUME		0x02
#define USB_INTR_RESET		0x04

#define USB_INTR_EP(n)		(1 << (n))

/* CSR0 bit masks */
#define USB_CSR0_OUTPKTRDY	0x01
#define USB_CSR0_INPKTRDY	0x02
#define USB_CSR0_SENTSTALL	0x04
#define USB_CSR0_DATAEND	0x08
#define USB_CSR0_SETUPEND	0x10
#define USB_CSR0_SENDSTALL	0x20
#define USB_CSR0_SVDOUTPKTRDY	0x40
#define USB_CSR0_SVDSETUPEND	0x80
#define USB_CSR0_FLUSHFIFO	0x100

/* Endpoint CSR register bits */
#define USB_INCSRH_AUTOSET	0x80
#define USB_INCSRH_ISO		0x40
#define USB_INCSRH_MODE		0x20
#define USB_INCSRH_DMAREQENAB	0x10
#define USB_INCSRH_FRCDATATOG	0x08
#define USB_INCSRH_DMAREQMODE	0x04
#define USB_INCSR_CDT		0x40
#define USB_INCSR_SENTSTALL	0x20
#define USB_INCSR_SENDSTALL	0x10
#define USB_INCSR_FF		0x08
#define USB_INCSR_UNDERRUN	0x04
#define USB_INCSR_FFNOTEMPT	0x02
#define USB_INCSR_INPKTRDY	0x01
#define USB_OUTCSRH_AUTOCLR	0x80
#define USB_OUTCSRH_ISO		0x40
#define USB_OUTCSRH_DMAREQENAB	0x20
#define USB_OUTCSRH_DNYT	0x10
#define USB_OUTCSRH_DMAREQMODE	0x08
#define USB_OUTCSR_CDT		0x80
#define USB_OUTCSR_SENTSTALL	0x40
#define USB_OUTCSR_SENDSTALL	0x20
#define USB_OUTCSR_FF		0x10
#define USB_OUTCSR_DATAERR	0x08
#define USB_OUTCSR_OVERRUN	0x04
#define USB_OUTCSR_FFFULL	0x02
#define USB_OUTCSR_OUTPKTRDY	0x01

/* Testmode register bits */
#define USB_TEST_SE0NAK		0x01
#define USB_TEST_J		0x02
#define USB_TEST_K		0x04
#define USB_TEST_PACKET		0x08
#define USB_TEST_FORCE_HS	0x10
#define USB_TEST_FORCE_FS	0x20
#define USB_TEST_ALL		( USB_TEST_SE0NAK | USB_TEST_J         \
				| USB_TEST_K | USB_TEST_PACKET         \
				| USB_TEST_FORCE_HS | USB_TEST_FORCE_FS)

/* DMA control bits */
#define USB_CNTL_ENA		0x01
#define USB_CNTL_DIR_IN		0x02
#define USB_CNTL_MODE_1		0x04
#define USB_CNTL_INTR_EN	0x08
#define USB_CNTL_EP(n)		((n) << 4)
#define USB_CNTL_BURST_0	(0 << 9)
#define USB_CNTL_BURST_4	(1 << 9)
#define USB_CNTL_BURST_8	(2 << 9)
#define USB_CNTL_BURST_16	(3 << 9)

/* DMA interrupt bits */
#define USB_INTR_DMA_BULKIN	1
#define USB_INTR_DMA_BULKOUT	2

#define REG_USB_FADDR		REG8(USB_FADDR)
#define REG_USB_POWER		REG8(USB_POWER)
#define REG_USB_INTRIN		REG16(USB_INTRIN)
#define REG_USB_INTROUT		REG16(USB_INTROUT)
#define REG_USB_INTRINE		REG16(USB_INTRINE)
#define REG_USB_INTROUTE	REG16(USB_INTROUTE)
#define REG_USB_INTRUSB		REG8(USB_INTRUSB)
#define REG_USB_INTRUSBE	REG8(USB_INTRUSBE)
#define REG_USB_FRAME		REG16(USB_FRAME)
#define REG_USB_INDEX		REG8(USB_INDEX)
#define REG_USB_TESTMODE	REG8(USB_TESTMODE)

#define REG_USB_CSR0		REG16(USB_CSR0)
#define REG_USB_COUNT0		REG8(USB_COUNT0)

#define REG_USB_INMAXP		REG16(USB_INMAXP)
#define REG_USB_INCSR		REG16(USB_INCSR)
#define REG_USB_INCSRH		REG8(USB_INCSRH)
#define REG_USB_OUTMAXP		REG16(USB_OUTMAXP)
#define REG_USB_OUTCSR		REG16(USB_OUTCSR)
#define REG_USB_OUTCSRH		REG8(USB_OUTCSRH)
#define REG_USB_OUTCOUNT	REG16(USB_OUTCOUNT)

#define REG_USB_FIFO_EP(n)	REG32(USB_FIFO_EP(n))

#define REG_USB_INTR		REG8(USB_INTR)
#define REG_USB_CNTL(n)		REG16(USB_CNTL(n))
#define REG_USB_ADDR(n)		REG32(USB_ADDR(n))
#define REG_USB_COUNT(n)	REG32(USB_COUNT(n))

#define REG_USB_EPINFO		REG8(USB_EPINFO)
#define REG_USB_RAMINFO		REG8(USB_RAMINFO)

/*
 * One wire bus interface(OWI) address definition
 */
#define	OWI_BASE	0xb0072000

/*
 * OWI registers offset address definition
 */
#define OWI_OWICFG_OFFSET	(0x00)	/* rw, 8, 0x00 */
#define OWI_OWICTL_OFFSET	(0x04)	/* rw, 8, 0x00 */
#define OWI_OWISTS_OFFSET	(0x08)	/* rw, 8, 0x00 */
#define OWI_OWIDAT_OFFSET	(0x0c)	/* rw, 8, 0x00 */
#define OWI_OWIDIV_OFFSET	(0x10)	/* rw, 8, 0x00 */

/*
 * OWI registers address definition
 */
#define OWI_OWICFG	(OWI_BASE + OWI_OWICFG_OFFSET)
#define OWI_OWICTL	(OWI_BASE + OWI_OWICTL_OFFSET)
#define OWI_OWISTS	(OWI_BASE + OWI_OWISTS_OFFSET)
#define OWI_OWIDAT	(OWI_BASE + OWI_OWIDAT_OFFSET)
#define OWI_OWIDIV	(OWI_BASE + OWI_OWIDIV_OFFSET)

/*
 * OWI registers common define
 */

/* OWI configure register(OWICFG) */
#define OWICFG_MODE	BIT7
#define OWICFG_RDDATA	BIT6
#define OWICFG_WRDATA	BIT5
#define OWICFG_RDST	BIT4
#define OWICFG_WR1RD	BIT3
#define OWICFG_WR0	BIT2
#define OWICFG_RST	BIT1
#define OWICFG_ENA	BIT0

/* OWI control register(OWICTL) */
#define OWICTL_EBYTE	BIT2
#define OWICTL_EBIT	BIT1
#define OWICTL_ERST	BIT0

/* OWI status register(OWISTS) */
#define OWISTS_PST		BIT7
#define OWISTS_BYTE_RDY		BIT2
#define OWISTS_BIT_RDY		BIT1
#define OWISTS_PST_RDY		BIT0

/* OWI clock divide register(OWIDIV) */
#define OWIDIV_CLKDIV_LSB	0
#define OWIDIV_CLKDIV_MASK	BITS_H2L(5, OWIDIV_CLKDIV_LSB)

#ifndef __MIPS_ASSEMBLER

/* Basic ops */
#define REG_OWI_OWICFG  REG8(OWI_OWICFG)
#define REG_OWI_OWICTL  REG8(OWI_OWICTL)
#define REG_OWI_OWISTS  REG8(OWI_OWISTS)
#define REG_OWI_OWIDAT  REG8(OWI_OWIDAT)
#define REG_OWI_OWIDIV  REG8(OWI_OWIDIV)

#endif /* __MIPS_ASSEMBLER */

/*
 * Pulse-code modulation module(PCM) address definition
 */
#define	PCM_BASE        0xb0071000

/*
 * pcm number, jz4760x has only PCM0
 */

#define PCM0		0
#define PCM1		1

/* PCM groups offset */
#define PCM_GOS		0x3000

/*
 * PCM registers offset address definition
 */
#define PCM_PCTL_OFFSET		(0x00)	/* rw, 32, 0x00000000 */
#define PCM_PCFG_OFFSET		(0x04)  /* rw, 32, 0x00000110 */
#define PCM_PDP_OFFSET		(0x08)  /* rw, 32, 0x00000000 */
#define PCM_PINTC_OFFSET	(0x0c)  /* rw, 32, 0x00000000 */
#define PCM_PINTS_OFFSET	(0x10)  /* rw, 32, 0x00000100 */
#define PCM_PDIV_OFFSET		(0x14)  /* rw, 32, 0x00000001 */

/*
 * PCM registers address definition
 */
#define PCM_PCTL(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PCTL_OFFSET)
#define PCM_PCFG(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PCFG_OFFSET)
#define PCM_PDP(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PDP_OFFSET)
#define PCM_PINTC(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PINTC_OFFSET)
#define PCM_PINTS(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PINTS_OFFSET)
#define PCM_PDIV(n)	(PCM_BASE + (n) * PCM_GOS + PCM_PDIV_OFFSET)

/*
 * CPM registers common define
 */

/* PCM controller control register (PCTL) */
#define PCTL_ERDMA	BIT9
#define PCTL_ETDMA	BIT8
#define PCTL_LSMP	BIT7
#define PCTL_ERPL	BIT6
#define PCTL_EREC	BIT5
#define PCTL_FLUSH	BIT4
#define PCTL_RST	BIT3
#define PCTL_CLKEN	BIT1
#define PCTL_PCMEN	BIT0

/* PCM controller configure register (PCFG) */
#define PCFG_ISS_16BIT		BIT12
#define PCFG_OSS_16BIT		BIT11
#define PCFG_IMSBPOS		BIT10
#define PCFG_OMSBPOS		BIT9
#define PCFG_MODE_SLAVE		BIT0

#define PCFG_SLOT_LSB		13
#define PCFG_SLOT_MASK		BITS_H2L(14, PCFG_SLOT_LSB)
#define PCFG_SLOT(val)		((val) << PCFG_SLOT_LSB)

#define	PCFG_RFTH_LSB		5
#define	PCFG_RFTH_MASK		BITS_H2L(8, PCFG_RFTH_LSB)

#define	PCFG_TFTH_LSB		1
#define	PCFG_TFTH_MASK		BITS_H2L(4, PCFG_TFTH_LSB)

/* PCM controller interrupt control register(PINTC) */
#define PINTC_ETFS	BIT3
#define PINTC_ETUR	BIT2
#define PINTC_ERFS	BIT1
#define PINTC_EROR	BIT0

/* PCM controller interrupt status register(PINTS) */
#define PINTS_RSTS	BIT14
#define PINTS_TFS	BIT8
#define PINTS_TUR	BIT7
#define PINTS_RFS	BIT1
#define PINTS_ROR	BIT0

#define PINTS_TFL_LSB		9
#define PINTS_TFL_MASK		BITS_H2L(13, PINTS_TFL_LSB)

#define PINTS_RFL_LSB		2
#define PINTS_RFL_MASK		BITS_H2L(6, PINTS_RFL_LSB)

/* PCM controller clock division register(PDIV) */
#define PDIV_SYNL_LSB		11
#define PDIV_SYNL_MASK		BITS_H2L(16, PDIV_SYNL_LSB)

#define PDIV_SYNDIV_LSB		6
#define PDIV_SYNDIV_MASK	BITS_H2L(10, PDIV_SYNDIV_LSB)

#define PDIV_CLKDIV_LSB		0
#define PDIV_CLKDIV_MASK	BITS_H2L(5, PDIV_CLKDIV_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_PCM_PCTL(n)		REG32(PCM_PCTL(n))
#define REG_PCM_PCFG(n)		REG32(PCM_PCFG(n))
#define REG_PCM_PDP(n)		REG32(PCM_PDP(n))
#define REG_PCM_PINTC(n)	REG32(PCM_PINTC(n))
#define REG_PCM_PINTS(n)	REG32(PCM_PINTS(n))
#define REG_PCM_PDIV(n)		REG32(PCM_PDIV(n))

/*
 * PCM_DIN, PCM_DOUT, PCM_CLK, PCM_SYN
 */
#define __gpio_as_pcm(n) 						\
do {									\
	switch(n) {							\
	case PCM0:		__gpio_as_pcm0();break;			\
	case PCM1:		__gpio_as_pcm1();break;			\
	}								\
									\
} while (0)

#define __pcm_enable(n)			(REG_PCM_PCTL(n) |= PCTL_PCMEN)
#define __pcm_disable(n)		(REG_PCM_PCTL(n) &= ~PCTL_PCMEN)

#define __pcm_clk_enable(n)		(REG_PCM_PCTL(n) |= PCTL_CLKEN)
#define __pcm_clk_disable(n)		(REG_PCM_PCTL(n) &= ~PCTL_CLKEN)

#define __pcm_reset(n)			(REG_PCM_PCTL(n) |= PCTL_RST)
#define __pcm_flush_fifo(n)		(REG_PCM_PCTL(n) |= PCTL_FLUSH)

#define __pcm_enable_record(n)		(REG_PCM_PCTL(n) |= PCTL_EREC)
#define __pcm_disable_record(n)		(REG_PCM_PCTL(n) &= ~PCTL_EREC)
#define __pcm_enable_playback(n)	(REG_PCM_PCTL(n) |= PCTL_ERPL)
#define __pcm_disable_playback(n)	(REG_PCM_PCTL(n) &= ~PCTL_ERPL)

#define __pcm_enable_rxfifo(n)		__pcm_enable_record(n)
#define __pcm_disable_rxfifo(n)		__pcm_disable_record(n)
#define __pcm_enable_txfifo(n)		__pcm_enable_playback(n)
#define __pcm_disable_txfifo(n)		__pcm_disable_playback(n)

#define __pcm_last_sample(n)		(REG_PCM_PCTL(n) |= PCTL_LSMP)
#define __pcm_zero_sample(n)		(REG_PCM_PCTL(n) &= ~PCTL_LSMP)

#define __pcm_enable_transmit_dma(n)	(REG_PCM_PCTL(n) |= PCTL_ETDMA)
#define __pcm_disable_transmit_dma(n)	(REG_PCM_PCTL(n) &= ~PCTL_ETDMA)
#define __pcm_enable_receive_dma(n)	(REG_PCM_PCTL(n) |= PCTL_ERDMA)
#define __pcm_disable_receive_dma(n)	(REG_PCM_PCTL(n) &= ~PCTL_ERDMA)

#define __pcm_as_master(n)		(REG_PCM_PCFG(n) &= ~PCFG_MODE_SLAVE)
#define __pcm_as_slave(n)		(REG_PCM_PCFG(n) |= PCFG_MODE_SLAVE)

#define __pcm_set_transmit_trigger(n, val)		\
do {							\
	REG_PCM_PCFG(n) &= ~PCFG_TFTH_MASK;		\
	REG_PCM_PCFG(n) |= ((val) << PCFG_TFTH_LSB);	\
							\
} while(0)

#define __pcm_set_receive_trigger(n, val)		\
do {							\
	REG_PCM_PCFG(n) &= ~PCFG_RFTH_MASK;		\
	REG_PCM_PCFG(n) |= ((val) << PCFG_RFTH_LSB);	\
							\
} while(0)

#define __pcm_omsb_same_sync(n)   	(REG_PCM_PCFG(n) &= ~PCFG_OMSBPOS)
#define __pcm_omsb_next_sync(n)   	(REG_PCM_PCFG(n) |= PCFG_OMSBPOS)

#define __pcm_imsb_same_sync(n)   	(REG_PCM_PCFG(n) &= ~PCFG_IMSBPOS)
#define __pcm_imsb_next_sync(n)   	(REG_PCM_PCFG(n) |= PCFG_IMSBPOS)

#define __pcm_set_iss(n, val)				\
do {							\
	if ((val) == 16)				\
		REG_PCM_PCFG(n) |= PCFG_ISS_16BIT;	\
	else						\
		REG_PCM_PCFG(n) &= ~PCFG_ISS_16BIT;	\
							\
} while (0)

#define __pcm_set_oss(n, val)				\
do {							\
	if ((val) == 16)				\
		REG_PCM_PCFG(n) |= PCFG_OSS_16BIT;	\
	else						\
		REG_PCM_PCFG(n) &= ~PCFG_OSS_16BIT;	\
							\
} while (0)						\

#define __pcm_set_valid_slot(n, val)					\
	(REG_PCM_PCFG(n) = (REG_PCM_PCFG(n) & ~PCFG_SLOT_MASK) | PCFG_SLOT(val))

#define __pcm_write_data(n, val)	(REG_PCM_PDP(n) = (val))
#define __pcm_read_data(n)		(REG_PCM_PDP(n))

#define __pcm_enable_tfs_intr(n)	(REG_PCM_PINTC(n) |= PINTC_ETFS)
#define __pcm_disable_tfs_intr(n)	(REG_PCM_PINTC(n) &= ~PINTC_ETFS)

#define __pcm_enable_tur_intr(n)	(REG_PCM_PINTC(n) |= PINTC_ETUR)
#define __pcm_disable_tur_intr(n)	(REG_PCM_PINTC(n) &= ~PINTC_ETUR)

#define __pcm_enable_rfs_intr(n)	(REG_PCM_PINTC(n) |= PINTC_ERFS)
#define __pcm_disable_rfs_intr(n)	(REG_PCM_PINTC(n) &= ~PINTC_ERFS)

#define __pcm_enable_ror_intr(n)	(REG_PCM_PINTC(n) |= PINTC_EROR)
#define __pcm_disable_ror_intr(n)	(REG_PCM_PINTC(n) &= ~PINTC_EROR)

#define __pcm_ints_valid_tx(n)		(((REG_PCM_PINTS(n) & PINTS_TFL_MASK) >> PINTS_TFL_LSB))
#define __pcm_ints_valid_rx(n)		(((REG_PCM_PINTS(n) & PINTS_RFL_MASK) >> PINTS_RFL_LSB))

#define __pcm_set_clk_div(n, val)					\
	(REG_PCM_PDIV(n) = (REG_PCM_PDIV(n) & ~PDIV_CLKDIV_MASK) | ((val) << PDIV_CLKDIV_LSB))

#define __pcm_set_clk_rate(n, sysclk, pcmclk)				\
	__pcm_set_clk_div((n), ((sysclk) / (pcmclk) - 1))

#define __pcm_set_sync_div(n, val)					\
	(REG_PCM_PDIV(n) = (REG_PCM_PDIV(n) & ~PDIV_SYNDIV_MASK) | ((val) << PDIV_SYNDIV_LSB))

#define __pcm_set_sync_rate(n, pcmclk, sync)				\
	__pcm_set_sync_div((n), ((pcmclk) / (8 * (sync)) - 1))

#define __pcm_set_sync_len(n, val)					\
	(REG_PCM_PDIV(n) = (REG_PCM_PDIV(n) & ~PDIV_SYNL_MASK) | ((val) << PDIV_SYNL_LSB))

#endif /* __MIPS_ASSEMBLER */

/*
 * Real time clock module(RTC) address definition
 */
#define	RTC_BASE	0xb0003000

/*
 * RTC registers offset address definition
 */
#define RTC_RTCCR_OFFSET	(0x00)	/* rw, 32, 0x00000081 */
#define RTC_RTCSR_OFFSET	(0x04)	/* rw, 32, 0x???????? */
#define RTC_RTCSAR_OFFSET	(0x08)	/* rw, 32, 0x???????? */
#define RTC_RTCGR_OFFSET	(0x0c)	/* rw, 32, 0x0??????? */

#define RTC_HCR_OFFSET		(0x20)  /* rw, 32, 0x00000000 */
#define RTC_HWFCR_OFFSET	(0x24)  /* rw, 32, 0x0000???0 */
#define RTC_HRCR_OFFSET		(0x28)  /* rw, 32, 0x00000??0 */
#define RTC_HWCR_OFFSET		(0x2c)  /* rw, 32, 0x00000008 */
#define RTC_HWRSR_OFFSET	(0x30)  /* rw, 32, 0x00000000 */
#define RTC_HSPR_OFFSET		(0x34)  /* rw, 32, 0x???????? */
#define RTC_WENR_OFFSET		(0x3c)  /* rw, 32, 0x00000000 */

/*
 * RTC registers address definition
 */
#define RTC_RTCCR	(RTC_BASE + RTC_RTCCR_OFFSET)
#define RTC_RTCSR	(RTC_BASE + RTC_RTCSR_OFFSET)
#define RTC_RTCSAR	(RTC_BASE + RTC_RTCSAR_OFFSET)
#define RTC_RTCGR	(RTC_BASE + RTC_RTCGR_OFFSET)

#define RTC_HCR		(RTC_BASE + RTC_HCR_OFFSET)
#define RTC_HWFCR	(RTC_BASE + RTC_HWFCR_OFFSET)
#define RTC_HRCR	(RTC_BASE + RTC_HRCR_OFFSET)
#define RTC_HWCR	(RTC_BASE + RTC_HWCR_OFFSET)
#define RTC_HWRSR	(RTC_BASE + RTC_HWRSR_OFFSET)
#define RTC_HSPR	(RTC_BASE + RTC_HSPR_OFFSET)
#define RTC_WENR	(RTC_BASE + RTC_WENR_OFFSET)

/*
 * RTC registers common define
 */

/* RTC control register(RTCCR) */
#define RTCCR_WRDY		BIT7
#define RTCCR_1HZ		BIT6
#define RTCCR_1HZIE		BIT5
#define RTCCR_AF		BIT4
#define RTCCR_AIE		BIT3
#define RTCCR_AE		BIT2
#define RTCCR_SELEXC		BIT1
#define RTCCR_RTCE		BIT0

/* RTC regulator register(RTCGR) */
#define RTCGR_LOCK		BIT31

#define RTCGR_ADJC_LSB		16
#define RTCGR_ADJC_MASK		BITS_H2L(25, RTCGR_ADJC_LSB)

#define RTCGR_NC1HZ_LSB		0
#define RTCGR_NC1HZ_MASK	BITS_H2L(15, RTCGR_NC1HZ_LSB)

/* Hibernate control register(HCR) */
#define HCR_PD			BIT0

/* Hibernate wakeup filter counter register(HWFCR) */
#define HWFCR_LSB		5
#define HWFCR_MASK		BITS_H2L(15, HWFCR_LSB)
#define HWFCR_WAIT_TIME(ms)	(((ms) << HWFCR_LSB) > HWFCR_MASK ? HWFCR_MASK : ((ms) << HWFCR_LSB))

/* Hibernate reset counter register(HRCR) */
#define HRCR_LSB		5
#define HRCR_MASK		BITS_H2L(11, HRCR_LSB)
#define HRCR_WAIT_TIME(ms)     (((ms) << HRCR_LSB) > HRCR_MASK ? HRCR_MASK : ((ms) << HRCR_LSB))

/* Hibernate wakeup control register(HWCR) */
#define HWCR_EPDET		BIT3
#define HWCR_WKUPVL		BIT2
#define HWCR_EALM		BIT0

/* Hibernate wakeup status register(HWRSR) */
#define HWRSR_APD		BIT8
#define HWRSR_HR		BIT5
#define HWRSR_PPR		BIT4
#define HWRSR_PIN		BIT1
#define HWRSR_ALM		BIT0

/* write enable pattern register(WENR) */
#define WENR_WEN		BIT31

#define WENR_WENPAT_LSB		0
#define WENR_WENPAT_MASK	BITS_H2L(15, WENR_WENPAT_LSB)
#define WENR_WENPAT_WRITABLE	(0xa55a)

/* Hibernate scratch pattern register(HSPR) */
#define HSPR_RTCV               0x52544356      /* The value is 'RTCV', means rtc is valid */ 

#ifndef __MIPS_ASSEMBLER

/* Waiting for the RTC register writing finish */
#define __wait_write_ready()						\
do {									\
	int timeout = 0x1000;						\
	while (!(rtc_read_reg(RTC_RTCCR) & RTCCR_WRDY) && timeout--);	\
}while(0);

/* Waiting for the RTC register writable */
#define __wait_writable()						\
do {									\
	int timeout = 0x1000;						\
	__wait_write_ready();						\
	OUTREG32(RTC_WENR, WENR_WENPAT_WRITABLE);			\
	__wait_write_ready();						\
	while (!(rtc_read_reg(RTC_WENR) & WENR_WEN) && timeout--);	\
}while(0);

/* Basic RTC ops */
#define rtc_read_reg(reg)				\
({							\
	unsigned int data, timeout = 0x10000;		\
	do {						\
		data = INREG32(reg);			\
	} while (INREG32(reg) != data && timeout--);	\
	data;						\
})

#define rtc_write_reg(reg, data)			\
do {							\
	__wait_writable();				\
	OUTREG32(reg, data);				\
	__wait_write_ready();				\
}while(0);

#define rtc_set_reg(reg, data)	rtc_write_reg(reg, rtc_read_reg(reg) | (data))
#define rtc_clr_reg(reg, data)	rtc_write_reg(reg, rtc_read_reg(reg) & ~(data))

#endif /* __MIPS_ASSEMBLER */

/*
 * SAR A/D Controller(SADC) address definition
 */
#define	SADC_BASE		0xb0070000

/*
 * SADC registers offset definition
 */
#define SADC_ADENA_OFFSET	(0x00)	/* rw,  8, 0x00 */
#define SADC_ADCFG_OFFSET       (0x04)  /* rw, 32, 0x0002000c */
#define SADC_ADCTRL_OFFSET      (0x08)  /* rw,  8, 0x3f */
#define SADC_ADSTATE_OFFSET     (0x0c)  /* rw,  8, 0x00 */
#define SADC_ADSAME_OFFSET    	(0x10)  /* rw, 16, 0x0000 */
#define SADC_ADWAIT_OFFSET    	(0x14)  /* rw, 16, 0x0000 */
#define SADC_ADTCH_OFFSET       (0x18)  /* rw, 32, 0x00000000 */
#define SADC_ADVDAT_OFFSET      (0x1c)  /* rw, 16, 0x0000 */
#define SADC_ADADAT_OFFSET      (0x20)  /* rw, 16, 0x0000 */
#define SADC_ADFLT_OFFSET       (0x24)  /* rw, 16, 0x0000 */
#define SADC_ADCLK_OFFSET       (0x28)  /* rw, 32, 0x00000000 */

/*
 * SADC registers address definition
 */
#define SADC_ADENA		(SADC_BASE + SADC_ADENA_OFFSET)	 /* ADC Enable Register */
#define SADC_ADCFG		(SADC_BASE + SADC_ADCFG_OFFSET)	 /* ADC Configure Register */
#define SADC_ADCTRL		(SADC_BASE + SADC_ADCTRL_OFFSET) /* ADC Control Register */
#define SADC_ADSTATE		(SADC_BASE + SADC_ADSTATE_OFFSET)/* ADC Status Register*/
#define SADC_ADSAME		(SADC_BASE + SADC_ADSAME_OFFSET) /* ADC Same Point Time Register */
#define SADC_ADWAIT		(SADC_BASE + SADC_ADWAIT_OFFSET) /* ADC Wait Time Register */
#define SADC_ADTCH		(SADC_BASE + SADC_ADTCH_OFFSET)  /* ADC Touch Screen Data Register */
#define SADC_ADVDAT		(SADC_BASE + SADC_ADVDAT_OFFSET) /* ADC VBAT Data Register */
#define SADC_ADADAT		(SADC_BASE + SADC_ADADAT_OFFSET) /* ADC AUX Data Register */
#define SADC_ADFLT		(SADC_BASE + SADC_ADFLT_OFFSET)  /* ADC Filter Register */
#define SADC_ADCLK		(SADC_BASE + SADC_ADCLK_OFFSET)  /* ADC Clock Divide Register */

/*
 * SADC registers common define
 */

/* ADC Enable Register (ADENA) */
#define ADENA_POWER		BIT7
#define ADENA_SLP_MD		BIT6
#define ADENA_PENDEN		BIT3
#define ADENA_TCHEN		BIT2
#define ADENA_VBATEN		BIT1
#define ADENA_AUXEN		BIT0

/* ADC Configure Register (ADCFG) */
#define ADCFG_SPZZ           	BIT31
#define ADCFG_VBAT_SEL		BIT30
#define ADCFG_DMA_EN		BIT15

#define ADCFG_XYZ_LSB		13
#define ADCFG_XYZ_MASK		BITS_H2L(14, ADCFG_XYZ_LSB)
 #define ADCFG_XYZ_XYS		(0x0 << ADCFG_XYZ_LSB)
 #define ADCFG_XYZ_XYD		(0x1 << ADCFG_XYZ_LSB)
 #define ADCFG_XYZ_XYZ1Z2	(0x2 << ADCFG_XYZ_LSB)

#define ADCFG_SNUM_LSB		10
#define ADCFG_SNUM_MASK		BITS_H2L(12, ADCFG_SNUM_LSB)
 #define ADCFG_SNUM(n)          (((n) <= 6 ? ((n)-1) : ((n)-2)) << ADCFG_SNUM_LSB)

#define ADCFG_CMD_LSB		0
#define ADCFG_CMD_MASK		BITS_H2L(1, ADCFG_CMD_LSB)
 #define ADCFG_CMD_AUX(n)	((n) << ADCFG_CMD_LSB)

/* ADC Control Register (ADCCTRL) */
#define ADCTRL_SLPENDM		BIT5
#define ADCTRL_PENDM		BIT4
#define ADCTRL_PENUM		BIT3
#define ADCTRL_DTCHM		BIT2
#define ADCTRL_VRDYM		BIT1
#define ADCTRL_ARDYM		BIT0
#define ADCTRL_MASK_ALL         (ADCTRL_SLPENDM | ADCTRL_PENDM | ADCTRL_PENUM \
                                | ADCTRL_DTCHM | ADCTRL_VRDYM | ADCTRL_ARDYM)

/*  ADC Status Register  (ADSTATE) */
#define ADSTATE_SLP_RDY		BIT7
#define ADSTATE_SLPEND		BIT5
#define ADSTATE_PEND		BIT4
#define ADSTATE_PENU		BIT3
#define ADSTATE_DTCH		BIT2
#define ADSTATE_VRDY		BIT1
#define ADSTATE_ARDY		BIT0

/* ADC Same Point Time Register (ADSAME) */
#define ADSAME_SCNT_LSB		0
#define ADSAME_SCNT_MASK	BITS_H2L(15, ADSAME_SCNT_LSB)

/* ADC Wait Pen Down Time Register (ADWAIT) */
#define ADWAIT_WCNT_LSB		0
#define ADWAIT_WCNT_MASK	BITS_H2L(15, ADWAIT_WCNT_LSB)

/* ADC Touch Screen Data Register (ADTCH) */
#define ADTCH_TYPE1		BIT31
#define ADTCH_TYPE0		BIT15

#define ADTCH_DATA1_LSB		16
#define ADTCH_DATA1_MASK	BITS_H2L(27, ADTCH_DATA1_LSB)

#define ADTCH_DATA0_LSB		0
#define ADTCH_DATA0_MASK	BITS_H2L(11, ADTCH_DATA0_LSB)

/* ADC VBAT Date Register (ADVDAT) */
#define ADVDAT_VDATA_LSB	0
#define ADVDAT_VDATA_MASK	BITS_H2L(11, ADVDAT_VDATA_LSB)

/* ADC AUX Data Register (ADADAT) */
#define ADADAT_ADATA_LSB	0
#define ADADAT_ADATA_MASK	BITS_H2L(11, ADADAT_ADATA_LSB)

/*  ADC Clock Divide Register (ADCLK) */
#define ADCLK_CLKDIV_MS_LSB	16
#define ADCLK_CLKDIV_MS_MASK	BITS_H2L(31, ADCLK_CLKDIV_MS_LSB)

#define ADCLK_CLKDIV_US_LSB	8
#define ADCLK_CLKDIV_US_MASK	BITS_H2L(15, ADCLK_CLKDIV_US_LSB)

#define ADCLK_CLKDIV_LSB	0
#define ADCLK_CLKDIV_MASK	BITS_H2L(7, ADCLK_CLKDIV_LSB)

/* ADC Filter Register (ADFLT) */
#define ADFLT_FLT_EN		BIT15

#define ADFLT_FLT_D_LSB		0
#define ADFLT_FLT_D_MASK	BITS_H2L(11, ADFLT_FLT_D_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_SADC_ADENA          REG8(SADC_ADENA)
#define REG_SADC_ADCFG          REG32(SADC_ADCFG)
#define REG_SADC_ADCTRL         REG8(SADC_ADCTRL)
#define REG_SADC_ADSTATE        REG8(SADC_ADSTATE)
#define REG_SADC_ADSAME         REG16(SADC_ADSAME)
#define REG_SADC_ADWAIT         REG16(SADC_ADWAIT)
#define REG_SADC_ADTCH          REG32(SADC_ADTCH)
#define REG_SADC_ADVDAT         REG16(SADC_ADVDAT)
#define REG_SADC_ADADAT         REG16(SADC_ADADAT)
#define REG_SADC_ADFLT          REG16(SADC_ADFLT)
#define REG_SADC_ADCLK          REG32(SADC_ADCLK)

#endif /* __MIPS_ASSEMBLER */

#define	SCC_BASE	0xB0040000

/*************************************************************************
 * SCC
 *************************************************************************/
#define	SCC_DR			(SCC_BASE + 0x000)
#define	SCC_FDR			(SCC_BASE + 0x004)
#define	SCC_CR			(SCC_BASE + 0x008)
#define	SCC_SR			(SCC_BASE + 0x00C)
#define	SCC_TFR			(SCC_BASE + 0x010)
#define	SCC_EGTR		(SCC_BASE + 0x014)
#define	SCC_ECR			(SCC_BASE + 0x018)
#define	SCC_RTOR		(SCC_BASE + 0x01C)

#define REG_SCC_DR		REG8(SCC_DR)
#define REG_SCC_FDR		REG8(SCC_FDR)
#define REG_SCC_CR		REG32(SCC_CR)
#define REG_SCC_SR		REG16(SCC_SR)
#define REG_SCC_TFR		REG16(SCC_TFR)
#define REG_SCC_EGTR		REG8(SCC_EGTR)
#define REG_SCC_ECR		REG32(SCC_ECR)
#define REG_SCC_RTOR		REG8(SCC_RTOR)

/* SCC FIFO Data Count Register (SCC_FDR) */

#define SCC_FDR_EMPTY		0x00
#define SCC_FDR_FULL		0x10

/* SCC Control Register (SCC_CR) */

#define SCC_CR_SCCE		(1 << 31)
#define SCC_CR_TRS		(1 << 30)
#define SCC_CR_T2R		(1 << 29)
#define SCC_CR_FDIV_BIT		24
#define SCC_CR_FDIV_MASK	(0x3 << SCC_CR_FDIV_BIT)
  #define SCC_CR_FDIV_1		(0 << SCC_CR_FDIV_BIT) /* SCC_CLK frequency is the same as device clock */
  #define SCC_CR_FDIV_2		(1 << SCC_CR_FDIV_BIT) /* SCC_CLK frequency is half of device clock */
#define SCC_CR_FLUSH		(1 << 23)
#define SCC_CR_TRIG_BIT		16
#define SCC_CR_TRIG_MASK	(0x3 << SCC_CR_TRIG_BIT)
  #define SCC_CR_TRIG_1		(0 << SCC_CR_TRIG_BIT) /* Receive/Transmit-FIFO Trigger is 1 */
  #define SCC_CR_TRIG_4		(1 << SCC_CR_TRIG_BIT) /* Receive/Transmit-FIFO Trigger is 4 */
  #define SCC_CR_TRIG_8		(2 << SCC_CR_TRIG_BIT) /* Receive/Transmit-FIFO Trigger is 8 */
  #define SCC_CR_TRIG_14	(3 << SCC_CR_TRIG_BIT) /* Receive/Transmit-FIFO Trigger is 14 */
#define SCC_CR_TP		(1 << 15)
#define SCC_CR_CONV		(1 << 14)
#define SCC_CR_TXIE		(1 << 13)
#define SCC_CR_RXIE		(1 << 12)
#define SCC_CR_TENDIE		(1 << 11)
#define SCC_CR_RTOIE		(1 << 10)
#define SCC_CR_ECIE		(1 << 9)
#define SCC_CR_EPIE		(1 << 8)
#define SCC_CR_RETIE		(1 << 7)
#define SCC_CR_EOIE		(1 << 6)
#define SCC_CR_TSEND		(1 << 3)
#define SCC_CR_PX_BIT		1
#define SCC_CR_PX_MASK		(0x3 << SCC_CR_PX_BIT)
  #define SCC_CR_PX_NOT_SUPPORT	(0 << SCC_CR_PX_BIT) /* SCC does not support clock stop */
  #define SCC_CR_PX_STOP_LOW	(1 << SCC_CR_PX_BIT) /* SCC_CLK stops at state low */
  #define SCC_CR_PX_STOP_HIGH	(2 << SCC_CR_PX_BIT) /* SCC_CLK stops at state high */
#define SCC_CR_CLKSTP		(1 << 0)

/* SCC Status Register (SCC_SR) */

#define SCC_SR_TRANS		(1 << 15)
#define SCC_SR_ORER		(1 << 12)
#define SCC_SR_RTO		(1 << 11)
#define SCC_SR_PER		(1 << 10)
#define SCC_SR_TFTG		(1 << 9)
#define SCC_SR_RFTG		(1 << 8)
#define SCC_SR_TEND		(1 << 7)
#define SCC_SR_RETR_3		(1 << 4)
#define SCC_SR_ECNTO		(1 << 0)

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * SCC
 ***************************************************************************/

#define __scc_enable()			( REG_SCC_CR |= SCC_CR_SCCE )
#define __scc_disable()			( REG_SCC_CR &= ~SCC_CR_SCCE )

#define __scc_set_tx_mode()		( REG_SCC_CR |= SCC_CR_TRS )
#define __scc_set_rx_mode()		( REG_SCC_CR &= ~SCC_CR_TRS )

#define __scc_enable_t2r()		( REG_SCC_CR |= SCC_CR_T2R )
#define __scc_disable_t2r()		( REG_SCC_CR &= ~SCC_CR_T2R )

#define __scc_clk_as_devclk()			\
do {						\
  REG_SCC_CR &= ~SCC_CR_FDIV_MASK;		\
  REG_SCC_CR |= SCC_CR_FDIV_1;			\
} while (0)

#define __scc_clk_as_half_devclk()		\
do {						\
  REG_SCC_CR &= ~SCC_CR_FDIV_MASK;		\
  REG_SCC_CR |= SCC_CR_FDIV_2;			\
} while (0)

/* n=1,4,8,14 */
#define __scc_set_fifo_trigger(n)		\
do {						\
  REG_SCC_CR &= ~SCC_CR_TRIG_MASK;		\
  REG_SCC_CR |= SCC_CR_TRIG_##n;		\
} while (0)

#define __scc_set_protocol(p)			\
do {						\
	if (p)					\
	  	REG_SCC_CR |= SCC_CR_TP;	\
	else					\
	 	REG_SCC_CR &= ~SCC_CR_TP;	\
} while (0)

#define __scc_flush_fifo()		( REG_SCC_CR |= SCC_CR_FLUSH )

#define __scc_set_invert_mode()		( REG_SCC_CR |= SCC_CR_CONV )
#define __scc_set_direct_mode()		( REG_SCC_CR &= ~SCC_CR_CONV )

#define SCC_ERR_INTRS \
    ( SCC_CR_ECIE | SCC_CR_EPIE | SCC_CR_RETIE | SCC_CR_EOIE )
#define SCC_ALL_INTRS \
    ( SCC_CR_TXIE | SCC_CR_RXIE | SCC_CR_TENDIE | SCC_CR_RTOIE | \
      SCC_CR_ECIE | SCC_CR_EPIE | SCC_CR_RETIE | SCC_CR_EOIE )

#define __scc_enable_err_intrs()	( REG_SCC_CR |= SCC_ERR_INTRS )
#define __scc_disable_err_intrs()	( REG_SCC_CR &= ~SCC_ERR_INTRS )

#define SCC_ALL_ERRORS \
    ( SCC_SR_ORER | SCC_SR_RTO | SCC_SR_PER | SCC_SR_RETR_3 | SCC_SR_ECNTO)

#define __scc_clear_errors()		( REG_SCC_SR &= ~SCC_ALL_ERRORS )

#define __scc_enable_all_intrs()	( REG_SCC_CR |= SCC_ALL_INTRS )
#define __scc_disable_all_intrs()	( REG_SCC_CR &= ~SCC_ALL_INTRS )

#define __scc_enable_tx_intr()		( REG_SCC_CR |= SCC_CR_TXIE | SCC_CR_TENDIE )
#define __scc_disable_tx_intr()		( REG_SCC_CR &= ~(SCC_CR_TXIE | SCC_CR_TENDIE) )

#define __scc_enable_rx_intr()		( REG_SCC_CR |= SCC_CR_RXIE)
#define __scc_disable_rx_intr()		( REG_SCC_CR &= ~SCC_CR_RXIE)

#define __scc_set_tsend()		( REG_SCC_CR |= SCC_CR_TSEND )
#define __scc_clear_tsend()		( REG_SCC_CR &= ~SCC_CR_TSEND )

#define __scc_set_clockstop()		( REG_SCC_CR |= SCC_CR_CLKSTP )
#define __scc_clear_clockstop()		( REG_SCC_CR &= ~SCC_CR_CLKSTP )

#define __scc_clockstop_low()			\
do {						\
  REG_SCC_CR &= ~SCC_CR_PX_MASK;		\
  REG_SCC_CR |= SCC_CR_PX_STOP_LOW;		\
} while (0)

#define __scc_clockstop_high()			\
do {						\
  REG_SCC_CR &= ~SCC_CR_PX_MASK;		\
  REG_SCC_CR |= SCC_CR_PX_STOP_HIGH;		\
} while (0)

/* SCC status checking */
#define __scc_check_transfer_status()		( REG_SCC_SR & SCC_SR_TRANS )
#define __scc_check_rx_overrun_error()		( REG_SCC_SR & SCC_SR_ORER )
#define __scc_check_rx_timeout()		( REG_SCC_SR & SCC_SR_RTO )
#define __scc_check_parity_error()		( REG_SCC_SR & SCC_SR_PER )
#define __scc_check_txfifo_trigger()		( REG_SCC_SR & SCC_SR_TFTG )
#define __scc_check_rxfifo_trigger()		( REG_SCC_SR & SCC_SR_RFTG )
#define __scc_check_tx_end()			( REG_SCC_SR & SCC_SR_TEND )
#define __scc_check_retx_3()			( REG_SCC_SR & SCC_SR_RETR_3 )
#define __scc_check_ecnt_overflow()		( REG_SCC_SR & SCC_SR_ECNTO )

#endif /* __MIPS_ASSEMBLER */

#define	SSI0_BASE	0xB0043000
#define	SSI1_BASE	0xB0044000
#define	SSI2_BASE	0xB0045000

/*************************************************************************
 * SSI (Synchronous Serial Interface)
 *************************************************************************/
/* n = 0, 1 (SSI0, SSI1) */
#define	SSI_DR(n)		(SSI0_BASE + 0x000 + (n)*0x1000)
#define	SSI_CR0(n)		(SSI0_BASE + 0x004 + (n)*0x1000)
#define	SSI_CR1(n)		(SSI0_BASE + 0x008 + (n)*0x1000)
#define	SSI_SR(n)		(SSI0_BASE + 0x00C + (n)*0x1000)
#define	SSI_ITR(n)		(SSI0_BASE + 0x010 + (n)*0x1000)
#define	SSI_ICR(n)		(SSI0_BASE + 0x014 + (n)*0x1000)
#define	SSI_GR(n)		(SSI0_BASE + 0x018 + (n)*0x1000)

#define	REG_SSI_DR(n)		REG32(SSI_DR(n))
#define	REG_SSI_CR0(n)		REG16(SSI_CR0(n))
#define	REG_SSI_CR1(n)		REG32(SSI_CR1(n))
#define	REG_SSI_SR(n)		REG32(SSI_SR(n))
#define	REG_SSI_ITR(n)		REG16(SSI_ITR(n))
#define	REG_SSI_ICR(n)		REG8(SSI_ICR(n))
#define	REG_SSI_GR(n)		REG16(SSI_GR(n))

/* SSI Data Register (SSI_DR) */

#define	SSI_DR_GPC_BIT		0
#define	SSI_DR_GPC_MASK		(0x1ff << SSI_DR_GPC_BIT)

#define SSI_MAX_FIFO_ENTRIES 	128 /* 128 txfifo and 128 rxfifo */

/* SSI Control Register 0 (SSI_CR0) */

#define SSI_CR0_SSIE		(1 << 15)
#define SSI_CR0_TIE		(1 << 14)
#define SSI_CR0_RIE		(1 << 13)
#define SSI_CR0_TEIE		(1 << 12)
#define SSI_CR0_REIE		(1 << 11)
#define SSI_CR0_LOOP		(1 << 10)
#define SSI_CR0_RFINE		(1 << 9)
#define SSI_CR0_RFINC		(1 << 8)
#define SSI_CR0_EACLRUN		(1 << 7) /* hardware auto clear underrun when TxFifo no empty */
#define SSI_CR0_FSEL		(1 << 6)
#define SSI_CR0_TFLUSH		(1 << 2)
#define SSI_CR0_RFLUSH		(1 << 1)
#define SSI_CR0_DISREV		(1 << 0)

/* SSI Control Register 1 (SSI_CR1) */

#define SSI_CR1_FRMHL_BIT	30
#define SSI_CR1_FRMHL_MASK	(0x3 << SSI_CR1_FRMHL_BIT)
  #define SSI_CR1_FRMHL_CELOW_CE2LOW	(0 << SSI_CR1_FRMHL_BIT) /* SSI_CE_ is low valid and SSI_CE2_ is low valid */
  #define SSI_CR1_FRMHL_CEHIGH_CE2LOW	(1 << SSI_CR1_FRMHL_BIT) /* SSI_CE_ is high valid and SSI_CE2_ is low valid */
  #define SSI_CR1_FRMHL_CELOW_CE2HIGH	(2 << SSI_CR1_FRMHL_BIT) /* SSI_CE_ is low valid  and SSI_CE2_ is high valid */
  #define SSI_CR1_FRMHL_CEHIGH_CE2HIGH	(3 << SSI_CR1_FRMHL_BIT) /* SSI_CE_ is high valid and SSI_CE2_ is high valid */
#define SSI_CR1_TFVCK_BIT	28
#define SSI_CR1_TFVCK_MASK	(0x3 << SSI_CR1_TFVCK_BIT)
  #define SSI_CR1_TFVCK_0	  (0 << SSI_CR1_TFVCK_BIT)
  #define SSI_CR1_TFVCK_1	  (1 << SSI_CR1_TFVCK_BIT)
  #define SSI_CR1_TFVCK_2	  (2 << SSI_CR1_TFVCK_BIT)
  #define SSI_CR1_TFVCK_3	  (3 << SSI_CR1_TFVCK_BIT)
#define SSI_CR1_TCKFI_BIT	26
#define SSI_CR1_TCKFI_MASK	(0x3 << SSI_CR1_TCKFI_BIT)
  #define SSI_CR1_TCKFI_0	  (0 << SSI_CR1_TCKFI_BIT)
  #define SSI_CR1_TCKFI_1	  (1 << SSI_CR1_TCKFI_BIT)
  #define SSI_CR1_TCKFI_2	  (2 << SSI_CR1_TCKFI_BIT)
  #define SSI_CR1_TCKFI_3	  (3 << SSI_CR1_TCKFI_BIT)
#define SSI_CR1_LFST		(1 << 25)
#define SSI_CR1_ITFRM		(1 << 24)
#define SSI_CR1_UNFIN		(1 << 23)
#define SSI_CR1_MULTS		(1 << 22)
#define SSI_CR1_FMAT_BIT	20
#define SSI_CR1_FMAT_MASK	(0x3 << SSI_CR1_FMAT_BIT)
  #define SSI_CR1_FMAT_SPI	  (0 << SSI_CR1_FMAT_BIT) /* Motorola¡¯s SPI format */
  #define SSI_CR1_FMAT_SSP	  (1 << SSI_CR1_FMAT_BIT) /* TI's SSP format */
  #define SSI_CR1_FMAT_MW1	  (2 << SSI_CR1_FMAT_BIT) /* National Microwire 1 format */
  #define SSI_CR1_FMAT_MW2	  (3 << SSI_CR1_FMAT_BIT) /* National Microwire 2 format */
#define SSI_CR1_TTRG_BIT	16 /* SSI1 TX trigger */
#define SSI_CR1_TTRG_MASK	(0xf << SSI_CR1_TTRG_BIT) 
#define SSI_CR1_MCOM_BIT	12
#define SSI_CR1_MCOM_MASK	(0xf << SSI_CR1_MCOM_BIT)
  #define SSI_CR1_MCOM_1BIT	  (0x0 << SSI_CR1_MCOM_BIT) /* 1-bit command selected */
  #define SSI_CR1_MCOM_2BIT	  (0x1 << SSI_CR1_MCOM_BIT) /* 2-bit command selected */
  #define SSI_CR1_MCOM_3BIT	  (0x2 << SSI_CR1_MCOM_BIT) /* 3-bit command selected */
  #define SSI_CR1_MCOM_4BIT	  (0x3 << SSI_CR1_MCOM_BIT) /* 4-bit command selected */
  #define SSI_CR1_MCOM_5BIT	  (0x4 << SSI_CR1_MCOM_BIT) /* 5-bit command selected */
  #define SSI_CR1_MCOM_6BIT	  (0x5 << SSI_CR1_MCOM_BIT) /* 6-bit command selected */
  #define SSI_CR1_MCOM_7BIT	  (0x6 << SSI_CR1_MCOM_BIT) /* 7-bit command selected */
  #define SSI_CR1_MCOM_8BIT	  (0x7 << SSI_CR1_MCOM_BIT) /* 8-bit command selected */
  #define SSI_CR1_MCOM_9BIT	  (0x8 << SSI_CR1_MCOM_BIT) /* 9-bit command selected */
  #define SSI_CR1_MCOM_10BIT	  (0x9 << SSI_CR1_MCOM_BIT) /* 10-bit command selected */
  #define SSI_CR1_MCOM_11BIT	  (0xA << SSI_CR1_MCOM_BIT) /* 11-bit command selected */
  #define SSI_CR1_MCOM_12BIT	  (0xB << SSI_CR1_MCOM_BIT) /* 12-bit command selected */
  #define SSI_CR1_MCOM_13BIT	  (0xC << SSI_CR1_MCOM_BIT) /* 13-bit command selected */
  #define SSI_CR1_MCOM_14BIT	  (0xD << SSI_CR1_MCOM_BIT) /* 14-bit command selected */
  #define SSI_CR1_MCOM_15BIT	  (0xE << SSI_CR1_MCOM_BIT) /* 15-bit command selected */
  #define SSI_CR1_MCOM_16BIT	  (0xF << SSI_CR1_MCOM_BIT) /* 16-bit command selected */
#define SSI_CR1_RTRG_BIT	8 /* SSI RX trigger */
#define SSI_CR1_RTRG_MASK	(0xf << SSI_CR1_RTRG_BIT)
#define SSI_CR1_FLEN_BIT	4
#define SSI_CR1_FLEN_MASK	(0xf << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_2BIT	  (0x0 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_3BIT	  (0x1 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_4BIT	  (0x2 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_5BIT	  (0x3 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_6BIT	  (0x4 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_7BIT	  (0x5 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_8BIT	  (0x6 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_9BIT	  (0x7 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_10BIT	  (0x8 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_11BIT	  (0x9 << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_12BIT	  (0xA << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_13BIT	  (0xB << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_14BIT	  (0xC << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_15BIT	  (0xD << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_16BIT	  (0xE << SSI_CR1_FLEN_BIT)
  #define SSI_CR1_FLEN_17BIT	  (0xF << SSI_CR1_FLEN_BIT)
#define SSI_CR1_PHA		(1 << 1)
#define SSI_CR1_POL		(1 << 0)

/* SSI Status Register (SSI_SR) */

#define SSI_SR_TFIFONUM_BIT	16
#define SSI_SR_TFIFONUM_MASK	(0xff << SSI_SR_TFIFONUM_BIT)
#define SSI_SR_RFIFONUM_BIT	8
#define SSI_SR_RFIFONUM_MASK	(0xff << SSI_SR_RFIFONUM_BIT)
#define SSI_SR_END		(1 << 7)
#define SSI_SR_BUSY		(1 << 6)
#define SSI_SR_TFF		(1 << 5)
#define SSI_SR_RFE		(1 << 4)
#define SSI_SR_TFHE		(1 << 3)
#define SSI_SR_RFHF		(1 << 2)
#define SSI_SR_UNDR		(1 << 1)
#define SSI_SR_OVER		(1 << 0)

/* SSI Interval Time Control Register (SSI_ITR) */

#define	SSI_ITR_CNTCLK		(1 << 15)
#define SSI_ITR_IVLTM_BIT	0
#define SSI_ITR_IVLTM_MASK	(0x7fff << SSI_ITR_IVLTM_BIT)

/*
 * SSI Character-per-frame Control Register (SSI_ICR)
 * SSI_ICR is ignored for SSI_ICR1.FMT != 00b
*/

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * SSI (Synchronous Serial Interface)
 ***************************************************************************/
/* n = 0, 1 (SSI0, SSI1) */
#define __ssi_enable(n) 	( REG_SSI_CR0(n) |= SSI_CR0_SSIE )
#define __ssi_disable(n) 	( REG_SSI_CR0(n) &= ~SSI_CR0_SSIE )
#define __ssi_select_ce(n) 	( REG_SSI_CR0(n) &= ~SSI_CR0_FSEL )

#define __ssi_normal_mode(n) ( REG_SSI_ITR(n) &= ~SSI_ITR_IVLTM_MASK )

#define __ssi_select_ce2(n) 		\
do { 					\
	REG_SSI_CR0(n) |= SSI_CR0_FSEL; 	\
	REG_SSI_CR1(n) &= ~SSI_CR1_MULTS;	\
} while (0)

#define __ssi_select_gpc(n) 			\
do { 						\
	REG_SSI_CR0(n) &= ~SSI_CR0_FSEL;	\
	REG_SSI_CR1(n) |= SSI_CR1_MULTS;	\
} while (0)

#define __ssi_underrun_auto_clear(n) 		\
do { 						\
	REG_SSI_CR0(n) |= SSI_CR0_EACLRUN; 	\
} while (0)

#define __ssi_underrun_clear_manually(n) 	\
do { 						\
	REG_SSI_CR0(n) &= ~SSI_CR0_EACLRUN; 	\
} while (0)

#define __ssi_enable_tx_intr(n)					\
	( REG_SSI_CR0(n) |= SSI_CR0_TIE | SSI_CR0_TEIE )

#define __ssi_disable_tx_intr(n)				\
	( REG_SSI_CR0(n) &= ~(SSI_CR0_TIE | SSI_CR0_TEIE) )

#define __ssi_enable_rx_intr(n)					\
	( REG_SSI_CR0(n) |= SSI_CR0_RIE | SSI_CR0_REIE )

#define __ssi_disable_rx_intr(n)				\
	( REG_SSI_CR0(n) &= ~(SSI_CR0_RIE | SSI_CR0_REIE) )

#define __ssi_enable_txfifo_half_empty_intr(n)  \
	( REG_SSI_CR0(n) |= SSI_CR0_TIE )
#define __ssi_disable_txfifo_half_empty_intr(n)	\
	( REG_SSI_CR0(n) &= ~SSI_CR0_TIE )
#define __ssi_enable_tx_error_intr(n)		\
	( REG_SSI_CR0(n) |= SSI_CR0_TEIE )
#define __ssi_disable_tx_error_intr(n)		\
	( REG_SSI_CR0(n) &= ~SSI_CR0_TEIE )
#define __ssi_enable_rxfifo_half_full_intr(n)	\
	( REG_SSI_CR0(n) |= SSI_CR0_RIE )
#define __ssi_disable_rxfifo_half_full_intr(n)  \
	( REG_SSI_CR0(n) &= ~SSI_CR0_RIE )
#define __ssi_enable_rx_error_intr(n)		\
	( REG_SSI_CR0(n) |= SSI_CR0_REIE )
#define __ssi_disable_rx_error_intr(n)		\
	( REG_SSI_CR0(n) &= ~SSI_CR0_REIE )

#define __ssi_enable_loopback(n)  ( REG_SSI_CR0(n) |= SSI_CR0_LOOP )
#define __ssi_disable_loopback(n) ( REG_SSI_CR0(n) &= ~SSI_CR0_LOOP )

#define __ssi_enable_receive(n)   ( REG_SSI_CR0(n) &= ~SSI_CR0_DISREV )
#define __ssi_disable_receive(n)  ( REG_SSI_CR0(n) |= SSI_CR0_DISREV )

#define __ssi_finish_receive(n)					\
	( REG_SSI_CR0(n) |= (SSI_CR0_RFINE | SSI_CR0_RFINC) )

#define __ssi_disable_recvfinish(n)				\
	( REG_SSI_CR0(n) &= ~(SSI_CR0_RFINE | SSI_CR0_RFINC) )

#define __ssi_flush_txfifo(n)   	( REG_SSI_CR0(n) |= SSI_CR0_TFLUSH )
#define __ssi_flush_rxfifo(n)   	( REG_SSI_CR0(n) |= SSI_CR0_RFLUSH )

#define __ssi_flush_fifo(n)					\
	( REG_SSI_CR0(n) |= SSI_CR0_TFLUSH | SSI_CR0_RFLUSH )

#define __ssi_finish_transmit(n) 	( REG_SSI_CR1(n) &= ~SSI_CR1_UNFIN )
#define __ssi_wait_transmit(n) 		( REG_SSI_CR1(n) |= SSI_CR1_UNFIN )
#define __ssi_use_busy_wait_mode(n) 	__ssi_wait_transmit(n)
#define __ssi_unset_busy_wait_mode(n) 	__ssi_finish_transmit(n)

#define __ssi_spi_format(n)						\
	do {								\
		REG_SSI_CR1(n) &= ~SSI_CR1_FMAT_MASK; 			\
		REG_SSI_CR1(n) |= SSI_CR1_FMAT_SPI;			\
		REG_SSI_CR1(n) &= ~(SSI_CR1_TFVCK_MASK|SSI_CR1_TCKFI_MASK); \
		REG_SSI_CR1(n) |= (SSI_CR1_TFVCK_1 | SSI_CR1_TCKFI_1);	\
	} while (0)

/* TI's SSP format, must clear SSI_CR1.UNFIN */
#define __ssi_ssp_format(n)						\
	do { 								\
		REG_SSI_CR1(n) &= ~(SSI_CR1_FMAT_MASK | SSI_CR1_UNFIN);	\
		REG_SSI_CR1(n) |= SSI_CR1_FMAT_SSP;			\
	} while (0)

/* National's Microwire format, must clear SSI_CR0.RFINE, and set max delay */
#define __ssi_microwire_format(n)					\
	do {								\
		REG_SSI_CR1(n) &= ~SSI_CR1_FMAT_MASK; 			\
		REG_SSI_CR1(n) |= SSI_CR1_FMAT_MW1;			\
		REG_SSI_CR1(n) &= ~(SSI_CR1_TFVCK_MASK|SSI_CR1_TCKFI_MASK); \
		REG_SSI_CR1(n) |= (SSI_CR1_TFVCK_3 | SSI_CR1_TCKFI_3);	\
		REG_SSI_CR0(n) &= ~SSI_CR0_RFINE;			\
	} while (0)

/* CE# level (FRMHL), CE# in interval time (ITFRM),
   clock phase and polarity (PHA POL),
   interval time (SSIITR), interval characters/frame (SSIICR) */

/* frmhl,endian,mcom,flen,pha,pol MASK */
#define SSICR1_MISC_MASK 					\
	( SSI_CR1_FRMHL_MASK | SSI_CR1_LFST | SSI_CR1_MCOM_MASK	\
	  | SSI_CR1_FLEN_MASK | SSI_CR1_PHA | SSI_CR1_POL )	

#define __ssi_spi_set_misc(n,frmhl,endian,flen,mcom,pha,pol)		\
	do {								\
		REG_SSI_CR1(n) &= ~SSICR1_MISC_MASK;			\
		REG_SSI_CR1(n) |= ((frmhl) << 30) | ((endian) << 25) | 	\
			(((mcom) - 1) << 12) | (((flen) - 2) << 4) | 	\
			((pha) << 1) | (pol); 				\
	} while(0)

/* Transfer with MSB or LSB first */
#define __ssi_set_msb(n) ( REG_SSI_CR1(n) &= ~SSI_CR1_LFST )
#define __ssi_set_lsb(n) ( REG_SSI_CR1(n) |= SSI_CR1_LFST )

#define __ssi_set_frame_length(n, m)					\
	REG_SSI_CR1(n) = (REG_SSI_CR1(n) & ~SSI_CR1_FLEN_MASK) | (((m) - 2) << 4) 

/* m = 1 - 16 */
#define __ssi_set_microwire_command_length(n,m)				\
	( REG_SSI_CR1(n) = ((REG_SSI_CR1(n) & ~SSI_CR1_MCOM_MASK) | SSI_CR1_MCOM_##m##BIT) )

/* Set the clock phase for SPI */
#define __ssi_set_spi_clock_phase(n, m)					\
	( REG_SSI_CR1(n) = ((REG_SSI_CR1(n) & ~SSI_CR1_PHA) | (((m)&0x1)<< 1)))

/* Set the clock polarity for SPI */
#define __ssi_set_spi_clock_polarity(n, p)				\
	( REG_SSI_CR1(n) = ((REG_SSI_CR1(n) & ~SSI_CR1_POL) | ((p)&0x1)) )

/* SSI tx trigger, m = i x 8 */
#define __ssi_set_tx_trigger(n, m)				\
	do {							\
		REG_SSI_CR1(n) &= ~SSI_CR1_TTRG_MASK;		\
		REG_SSI_CR1(n) |= ((m)/8)<<SSI_CR1_TTRG_BIT;	\
	} while (0)

/* SSI rx trigger, m = i x 8 */
#define __ssi_set_rx_trigger(n, m)				\
	do {							\
		REG_SSI_CR1(n) &= ~SSI_CR1_RTRG_MASK;		\
		REG_SSI_CR1(n) |= ((m)/8)<<SSI_CR1_RTRG_BIT;	\
	} while (0)

#define __ssi_get_txfifo_count(n)					\
	( (REG_SSI_SR(n) & SSI_SR_TFIFONUM_MASK) >> SSI_SR_TFIFONUM_BIT )

#define __ssi_get_rxfifo_count(n)					\
	( (REG_SSI_SR(n) & SSI_SR_RFIFONUM_MASK) >> SSI_SR_RFIFONUM_BIT )

#define __ssi_transfer_end(n)		( REG_SSI_SR(n) & SSI_SR_END )
#define __ssi_is_busy(n)		( REG_SSI_SR(n) & SSI_SR_BUSY )

#define __ssi_txfifo_full(n)		( REG_SSI_SR(n) & SSI_SR_TFF )
#define __ssi_rxfifo_empty(n)		( REG_SSI_SR(n) & SSI_SR_RFE )
#define __ssi_rxfifo_half_full(n)	( REG_SSI_SR(n) & SSI_SR_RFHF )
#define __ssi_txfifo_half_empty(n)	( REG_SSI_SR(n) & SSI_SR_TFHE )
#define __ssi_underrun(n)		( REG_SSI_SR(n) & SSI_SR_UNDR )
#define __ssi_overrun(n)		( REG_SSI_SR(n) & SSI_SR_OVER )
#define __ssi_clear_underrun(n)		( REG_SSI_SR(n) = ~SSI_SR_UNDR )
#define __ssi_clear_overrun(n)		( REG_SSI_SR(n) = ~SSI_SR_OVER )
#define __ssi_clear_errors(n)		( REG_SSI_SR(n) &= ~(SSI_SR_UNDR | SSI_SR_OVER) )

#define __ssi_set_clk(n, dev_clk, ssi_clk)			\
	( REG_SSI_GR(n) = (dev_clk) / (2*(ssi_clk)) - 1 )

#define __ssi_receive_data(n) 		REG_SSI_DR(n)
#define __ssi_transmit_data(n, v) 	(REG_SSI_DR(n) = (v))

#endif /* __MIPS_ASSEMBLER */

/*
 * Timer and counter unit module(TCU) address definition
 */
#define	TCU_BASE		0xb0002000

/* TCU group offset */
#define TCU_GOS			0x10

/* TCU total channel number */
#define TCU_CHANNEL_NUM		8

/*
 * TCU registers offset definition
 */
#define TCU_TER_OFFSET		(0x10)  /* r, 16, 0x0000     */
#define TCU_TESR_OFFSET		(0x14)  /* w, 16, 0x????     */
#define TCU_TECR_OFFSET		(0x18)  /* w, 16, 0x????     */

#define TCU_TSR_OFFSET		(0x1c)  /* r, 32, 0x00000000 */
#define TCU_TSSR_OFFSET		(0x2c)  /* w, 32, 0x00000000 */
#define TCU_TSCR_OFFSET		(0x3c)  /* w, 32, 0x0000     */

#define TCU_TFR_OFFSET		(0x20)  /* r, 32, 0x003F003F */
#define TCU_TFSR_OFFSET		(0x24)  /* w, 32, 0x???????? */
#define TCU_TFCR_OFFSET		(0x28)  /* w, 32, 0x???????? */

#define TCU_TMR_OFFSET		(0x30)  /* r, 32, 0x00000000 */
#define TCU_TMSR_OFFSET		(0x34)  /* w, 32, 0x???????? */
#define TCU_TMCR_OFFSET		(0x38)  /* w, 32, 0x???????? */

#define TCU_TSTR_OFFSET		(0xf0)  /* r, 32, 0x00000000 */
#define TCU_TSTSR_OFFSET	(0xf4)  /* w, 32, 0x???????? */
#define TCU_TSTCR_OFFSET	(0xf8)  /* w, 32, 0x???????? */

#define TCU_TDFR_OFFSET		(0x40)  /* rw,16, 0x????     */
#define TCU_TDHR_OFFSET		(0x44)  /* rw,16, 0x????     */
#define TCU_TCNT_OFFSET		(0x48)  /* rw,16, 0x????     */
#define TCU_TCSR_OFFSET		(0x4c)  /* rw,16, 0x0000     */

/*
 * TCU registers address definition
 */
#define TCU_TER		(TCU_BASE + TCU_TER_OFFSET)
#define TCU_TESR	(TCU_BASE + TCU_TESR_OFFSET)
#define TCU_TECR	(TCU_BASE + TCU_TECR_OFFSET)
#define TCU_TSR		(TCU_BASE + TCU_TSR_OFFSET)
#define TCU_TFR		(TCU_BASE + TCU_TFR_OFFSET)
#define TCU_TFSR	(TCU_BASE + TCU_TFSR_OFFSET)
#define TCU_TFCR	(TCU_BASE + TCU_TFCR_OFFSET)
#define TCU_TSSR	(TCU_BASE + TCU_TSSR_OFFSET)
#define TCU_TMR		(TCU_BASE + TCU_TMR_OFFSET)
#define TCU_TMSR	(TCU_BASE + TCU_TMSR_OFFSET)
#define TCU_TMCR	(TCU_BASE + TCU_TMCR_OFFSET)
#define TCU_TSCR	(TCU_BASE + TCU_TSCR_OFFSET)
#define TCU_TSTR	(TCU_BASE + TCU_TSTR_OFFSET)
#define TCU_TSTSR	(TCU_BASE + TCU_TSTSR_OFFSET)
#define TCU_TSTCR	(TCU_BASE + TCU_TSTCR_OFFSET)

/* n is the TCU channel index (0 - 7) */
#define TCU_TDFR(n)	(TCU_BASE + (n) * TCU_GOS + TCU_TDFR_OFFSET)
#define TCU_TDHR(n)	(TCU_BASE + (n) * TCU_GOS + TCU_TDHR_OFFSET)
#define TCU_TCNT(n)	(TCU_BASE + (n) * TCU_GOS + TCU_TCNT_OFFSET)
#define TCU_TCSR(n)	(TCU_BASE + (n) * TCU_GOS + TCU_TCSR_OFFSET)

/*
 * TCU registers bit field common define
 */

/* When n is NOT less than TCU_CHANNEL_NUM, change to TCU_CHANNEL_NUM - 1 */
#define __TIMER(n)	(1 << ((n) < TCU_CHANNEL_NUM ? (n) : (TCU_CHANNEL_NUM - 1))

/* Timer counter enable register(TER) */
#define TER_OSTEN	BIT15
#define TER_TCEN(n)	__TIMER(n)

/* Timer counter enable set register(TESR) */
#define TESR_OST	BIT15
#define TESR_TIMER(n)	__TIMER(n)

/* Timer counter enable clear register(TECR) */
#define TECR_OST	BIT15
#define TECR_TIMER(n)	__TIMER(n)

/* Timer stop register(TSR) */
#define TSR_WDT_STOP		BIT16
#define TSR_OST_STOP		BIT15
#define TSR_TIMER_STOP(n)	__TIMER(n)

/* Timer stop set register(TSSR) */
#define TSSR_WDT		BIT16
#define TSSR_OST		BIT15
#define TSSR_TIMER(n)		__TIMER(n)

/* Timer stop clear register(TSCR) */
#define TSCR_WDT		BIT16
#define TSCR_OST		BIT15
#define TSSR_TIMER(n)		__TIMER(n)

/* Timer flag register(TFR) */
#define TFR_HFLAG(n)	(__TIMER(n) << 16)
#define TFR_OSTFLAG	BIT15
#define TFR_FFLAG(n)	__TIMER(n)

/* Timer flag set register(TFSR) */
#define TFSR_HFLAG(n)	(__TIMER(n) << 16)
#define TFSR_OSTFLAG	BIT15
#define TFSR_FFLAG(n)	__TIMER(n)

/* Timer flag clear register(TFCR) */
#define TFCR_HFLAG(n)	(__TIMER(n) << 16)
#define TFCR_OSTFLAG	BIT15
#define TFCR_FFLAG(n)	(__TIMER(n))

/* Timer mast register(TMR) */
#define TMR_HMASK(n)	(__TIMER(n) << 16)
#define TMR_OSTMASK	BIT15
#define TMR_FMASK(n)	(__TIMER(n))

/* Timer mask set register(TMSR) */
#define TMSR_HMASK(n)	(__TIMER(n) << 16)
#define TMSR_OSTMASK	BIT15
#define TMSR_FMASK(n)	(__TIMER(n))

/* Timer mask clear register(TMCR) */
#define TMCR_HMASK(n)	(__TIMER(n) << 16)
#define TMCR_OSTMASK	BIT15
#define TMCR_FMASK(n)	(__TIMER(n))

/* Timer control register(TCSR) */
#define TCSR_BYPASS		BIT11
#define TCSR_CLRZ		BIT10
#define TCSR_SD_ABRUPT		BIT9
#define TCSR_INITL_HIGH		BIT8
#define TCSR_PWM_EN		BIT7
#define TCSR_PWM_IN_EN		BIT6
#define TCSR_EXT_EN		BIT2
#define TCSR_RTC_EN		BIT1
#define TCSR_PCK_EN		BIT0

#define TCSR_PRESCALE_LSB	3
#define TCSR_PRESCALE_MASK	BITS_H2L(5, TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE1		(0x0 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE4		(0x1 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE16		(0x2 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE64		(0x3 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE256	(0x4 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE1024	(0x5 << TCSR_PRESCALE_LSB)

/* Timer data full register(TDFR) */
#define TDFR_TDFR_LSB		0
#define TDFR_TDFR_MASK		BITS_H2L(15, TDFR_TDFR_LSB)

/* Timer data half register(TDHR) */
#define TDHR_TDHR_LSB		0
#define TDHR_TDHR_MASK		BITS_H2L(15, TDHR_TDHR_LSB)

/* Timer counter register(TCNT) */
#define TCNT_TCNT_LSB		0
#define TCNT_TCNT_MASK		BITS_H2L(15, TCNT_TCNT_LSB)

/* Timer status register(TSTR) */
#define TSTR_REAL2	BIT18
#define TSTR_REAL1	BIT17
#define TSTR_BUSY2	BIT2
#define TSTR_BUSY1	BIT1

/* Timer status set register(TSTSR) */
#define TSTSR_REALS2	BIT18
#define TSTSR_REALS1	BIT17
#define TSTSR_BUSYS2	BIT2
#define TSTSR_BUSYS1	BIT1

/* Timer status clear register(TSTCR) */
#define TSTCR_REALC2	BIT18
#define TSTCR_REALC1	BIT17
#define TSTCR_BUSYC2	BIT2
#define TSTCR_BUSYC1	BIT1

#ifndef __MIPS_ASSEMBLER

#define REG_TCU_TER	REG16(TCU_TER)
#define REG_TCU_TESR	REG16(TCU_TESR)
#define REG_TCU_TECR	REG16(TCU_TECR)
#define REG_TCU_TSR	REG32(TCU_TSR)
#define REG_TCU_TFR	REG32(TCU_TFR)
#define REG_TCU_TFSR	REG32(TCU_TFSR)
#define REG_TCU_TFCR	REG32(TCU_TFCR)
#define REG_TCU_TSSR	REG32(TCU_TSSR)
#define REG_TCU_TMR	REG32(TCU_TMR)
#define REG_TCU_TMSR	REG32(TCU_TMSR)
#define REG_TCU_TMCR	REG32(TCU_TMCR)
#define REG_TCU_TSCR	REG32(TCU_TSCR)
#define REG_TCU_TSTR	REG32(TCU_TSTR)
#define REG_TCU_TSTSR	REG32(TCU_TSTSR)
#define REG_TCU_TSTCR	REG32(TCU_TSTCR)

#define REG_TCU_TDFR(n)	REG16(TCU_TDFR(n))
#define REG_TCU_TDHR(n)	REG16(TCU_TDHR(n))
#define REG_TCU_TCNT(n)	REG16(TCU_TCNT(n))
#define REG_TCU_TCSR(n)	REG16(TCU_TCSR(n))

// where 'n' is the TCU channel
#define __tcu_select_extalclk(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~(TCSR_EXT_EN | TCSR_RTC_EN | TCSR_PCK_EN)) | TCSR_EXT_EN)
#define __tcu_select_rtcclk(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~(TCSR_EXT_EN | TCSR_RTC_EN | TCSR_PCK_EN)) | TCSR_RTC_EN)
#define __tcu_select_pclk(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~(TCSR_EXT_EN | TCSR_RTC_EN | TCSR_PCK_EN)) | TCSR_PCK_EN)
#define __tcu_disable_pclk(n) \
	REG_TCU_TCSR(n) = (REG_TCU_TCSR((n)) & ~TCSR_PCK_EN);
#define __tcu_select_clk_div1(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE1)
#define __tcu_select_clk_div4(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE4)
#define __tcu_select_clk_div16(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE16)
#define __tcu_select_clk_div64(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE64)
#define __tcu_select_clk_div256(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE256)
#define __tcu_select_clk_div1024(n) \
	(REG_TCU_TCSR((n)) = (REG_TCU_TCSR((n)) & ~TCSR_PRESCALE_MASK) | TCSR_PRESCALE1024)

#define __tcu_enable_pwm_output(n)	(REG_TCU_TCSR((n)) |= TCSR_PWM_EN)
#define __tcu_disable_pwm_output(n)	(REG_TCU_TCSR((n)) &= ~TCSR_PWM_EN)

#define __tcu_init_pwm_output_high(n)	(REG_TCU_TCSR((n)) |= TCSR_INITL_HIGH)
#define __tcu_init_pwm_output_low(n)	(REG_TCU_TCSR((n)) &= ~TCSR_INITL_HIGH)

#define __tcu_set_pwm_output_shutdown_graceful(n)	(REG_TCU_TCSR((n)) &= ~TCSR_SD_ABRUPT)
#define __tcu_set_pwm_output_shutdown_abrupt(n)		(REG_TCU_TCSR((n)) |= TCSR_SD_ABRUPT)

#define __tcu_clear_counter_to_zero(n)	(REG_TCU_TCSR((n)) |= TCSR_CLRZ)

#define __tcu_ost_enabled()		(REG_TCU_TER & TER_OSTEN)
#define __tcu_enable_ost()		(REG_TCU_TESR = TESR_OST)
#define __tcu_disable_ost()		(REG_TCU_TECR = TECR_OST)

#define __tcu_counter_enabled(n)	(REG_TCU_TER & (1 << (n)))
#define __tcu_start_counter(n)		(REG_TCU_TESR |= (1 << (n)))
#define __tcu_stop_counter(n)		(REG_TCU_TECR |= (1 << (n)))

#define __tcu_half_match_flag(n)	(REG_TCU_TFR & (1 << ((n) + 16)))
#define __tcu_full_match_flag(n)	(REG_TCU_TFR & (1 << (n)))
#define __tcu_set_half_match_flag(n)	(REG_TCU_TFSR = (1 << ((n) + 16)))
#define __tcu_set_full_match_flag(n)	(REG_TCU_TFSR = (1 << (n)))
#define __tcu_clear_half_match_flag(n)	(REG_TCU_TFCR = (1 << ((n) + 16)))
#define __tcu_clear_full_match_flag(n)	(REG_TCU_TFCR = (1 << (n)))
#define __tcu_mask_half_match_irq(n)	(REG_TCU_TMSR = (1 << ((n) + 16)))
#define __tcu_mask_full_match_irq(n)	(REG_TCU_TMSR = (1 << (n)))
#define __tcu_unmask_half_match_irq(n)	(REG_TCU_TMCR = (1 << ((n) + 16)))
#define __tcu_unmask_full_match_irq(n)	(REG_TCU_TMCR = (1 << (n)))

#define __tcu_ost_match_flag()		(REG_TCU_TFR & TFR_OSTFLAG)
#define __tcu_set_ost_match_flag()	(REG_TCU_TFSR = TFSR_OSTFLAG)
#define __tcu_clear_ost_match_flag()	(REG_TCU_TFCR = TFCR_OSTFLAG)
#define __tcu_ost_match_irq_masked()	(REG_TCU_TMR & TMR_OSTMASK)
#define __tcu_mask_ost_match_irq()	(REG_TCU_TMSR = TMSR_OSTMASK)
#define __tcu_unmask_ost_match_irq()	(REG_TCU_TMCR = TMCR_OSTMASK)

#define __tcu_wdt_clock_stopped()	(REG_TCU_TSR & TSR_WDT_STOP)
#define __tcu_ost_clock_stopped()	(REG_TCU_TSR & TSR_OST_STOP)
#define __tcu_timer_clock_stopped(n)	(REG_TCU_TSR & (1 << (n)))

#define __tcu_start_wdt_clock()		(REG_TCU_TSCR = TSCR_WDT)
#define __tcu_start_ost_clock()		(REG_TCU_TSCR = TSCR_OST)
#define __tcu_start_timer_clock(n)	(REG_TCU_TSCR = (1 << (n)))

#define __tcu_stop_wdt_clock()		(REG_TCU_TSSR = TSSR_WDT)
#define __tcu_stop_ost_clock()		(REG_TCU_TSSR = TSSR_OST)
#define __tcu_stop_timer_clock(n)	(REG_TCU_TSSR = (1 << (n)))

#define __tcu_get_count(n)		(REG_TCU_TCNT((n)))
#define __tcu_set_count(n,v)		(REG_TCU_TCNT((n)) = (v))
#define __tcu_set_full_data(n,v)	(REG_TCU_TDFR((n)) = (v))
#define __tcu_set_half_data(n,v)	(REG_TCU_TDHR((n)) = (v))

/* TCU2, counter 1, 2*/
#define __tcu_read_real_value(n)	(REG_TCU_TSTR & (1 << ((n) + 16)))
#define __tcu_read_false_value(n)	(REG_TCU_TSTR & (1 << ((n) + 16)))
#define __tcu_counter_busy(n)		(REG_TCU_TSTR & (1 << (n)))
#define __tcu_counter_ready(n)		(REG_TCU_TSTR & (1 << (n)))

#define __tcu_set_read_real_value(n)	(REG_TCU_TSTSR = (1 << ((n) + 16)))
#define __tcu_set_read_false_value(n)	(REG_TCU_TSTCR = (1 << ((n) + 16)))
#define __tcu_set_counter_busy(n)	(REG_TCU_TSTSR = (1 << (n)))
#define __tcu_set_counter_ready(n)	(REG_TCU_TSTCR = (1 << (n)))

#endif /* __MIPS_ASSEMBLER */

#define	TSSI0_BASE	0xB0073000

/*************************************************************************
 * TSSI MPEG 2-TS slave interface
 *************************************************************************/
#define TSSI_ENA       ( TSSI0_BASE + 0x00 )   /* TSSI enable register */
#define TSSI_CFG       ( TSSI0_BASE + 0x04 )   /* TSSI configure register */
#define TSSI_CTRL      ( TSSI0_BASE + 0x08 )   /* TSSI control register */
#define TSSI_STAT      ( TSSI0_BASE + 0x0c )   /* TSSI state register */
#define TSSI_FIFO      ( TSSI0_BASE + 0x10 )   /* TSSI FIFO register */
#define TSSI_PEN       ( TSSI0_BASE + 0x14 )   /* TSSI PID enable register */
#define TSSI_NUM       ( TSSI0_BASE + 0x18 )
#define TSSI_DTR       ( TSSI0_BASE + 0x1c )
#define TSSI_PID(n)    ( TSSI0_BASE + 0x20 + 4*(n) )   /* TSSI PID filter register */
#define TSSI_PID0      ( TSSI0_BASE + 0x20 )
#define TSSI_PID1      ( TSSI0_BASE + 0x24 )
#define TSSI_PID2      ( TSSI0_BASE + 0x28 )
#define TSSI_PID3      ( TSSI0_BASE + 0x2c )
#define TSSI_PID4      ( TSSI0_BASE + 0x30 )
#define TSSI_PID5      ( TSSI0_BASE + 0x34 )
#define TSSI_PID6      ( TSSI0_BASE + 0x38 )
#define TSSI_PID7      ( TSSI0_BASE + 0x3c )
#define TSSI_PID8      ( TSSI0_BASE + 0x40 )
#define TSSI_PID9      ( TSSI0_BASE + 0x44 )
#define TSSI_PID10     ( TSSI0_BASE + 0x48 )
#define TSSI_PID11     ( TSSI0_BASE + 0x4c )
#define TSSI_PID12     ( TSSI0_BASE + 0x50 )
#define TSSI_PID13     ( TSSI0_BASE + 0x54 )
#define TSSI_PID14     ( TSSI0_BASE + 0x58 )
#define TSSI_PID15     ( TSSI0_BASE + 0x5c )
#define TSSI_PID_MAX   16	/* max PID: 15 */

#define TSSI_DDA	( TSSI0_BASE + 0x60 )
#define TSSI_DTA	( TSSI0_BASE + 0x64 )
#define TSSI_DID	( TSSI0_BASE + 0x68 )
#define TSSI_DCMD	( TSSI0_BASE + 0x6c )
#define TSSI_DST	( TSSI0_BASE + 0x70 )
#define TSSI_TC		( TSSI0_BASE + 0x74 )
 
#define REG_TSSI_ENA       REG8( TSSI_ENA )
#define REG_TSSI_CFG       REG16( TSSI_CFG )
#define REG_TSSI_CTRL      REG8( TSSI_CTRL )
#define REG_TSSI_STAT      REG8( TSSI_STAT )
#define REG_TSSI_FIFO      REG32( TSSI_FIFO )
#define REG_TSSI_PEN       REG32( TSSI_PEN )
#define REG_TSSI_NUM       REG32( TSSI_NUM )
#define REG_TSSI_DTR	   REG32( TSSI_DTR )
#define REG_TSSI_PID(n)    REG32( TSSI_PID(n) )
#define REG_TSSI_PID0      REG32( TSSI_PID0 )
#define REG_TSSI_PID1      REG32( TSSI_PID1 )
#define REG_TSSI_PID2      REG32( TSSI_PID2 )
#define REG_TSSI_PID3      REG32( TSSI_PID3 )
#define REG_TSSI_PID4      REG32( TSSI_PID4 )
#define REG_TSSI_PID5      REG32( TSSI_PID5 )
#define REG_TSSI_PID6      REG32( TSSI_PID6 )
#define REG_TSSI_PID7      REG32( TSSI_PID7 )
#define REG_TSSI_PID8      REG32( TSSI_PID8 )
#define REG_TSSI_PID9      REG32( TSSI_PID9 )
#define REG_TSSI_PID10     REG32( TSSI_PID10 )
#define REG_TSSI_PID11     REG32( TSSI_PID11 )
#define REG_TSSI_PID12     REG32( TSSI_PID12 )
#define REG_TSSI_PID13     REG32( TSSI_PID13 )
#define REG_TSSI_PID14     REG32( TSSI_PID14 )
#define REG_TSSI_PID15     REG32( TSSI_PID15 )

/* TSSI enable register */
#define TSSI_ENA_SFT_RST 	( 1 << 7 )      /* soft reset bit */
#define TSSI_ENA_PID_EN 	( 1 << 2 )      /* soft filtering function enable bit */
#define TSSI_ENA_FAIL		( 1 << 4 )	/* fail signal bit */
#define TSSI_ENA_PEN_0		( 1 << 3 )	/* PID filter enable bit for PID */
#define TSSI_ENA_DMA_EN 	( 1 << 1 )      /* DMA enable bit */
#define TSSI_ENA_ENA 		( 1 << 0 )      /* TSSI enable bit */

/* TSSI configure register */
#define TSSI_CFG_TRIG_BIT 	14 /* fifo trig number */
#define TSSI_CFG_TRIG_MASK 	( 0x7 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_4 	( 0 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_8 	( 1 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_16 	( 2 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_32 	( 3 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_48 	( 4 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_64 	( 5 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_80 	( 6 << TSSI_CFG_TRIG_BIT)
#define TSSI_CFG_TRIG_96 	( 7 << TSSI_CFG_TRIG_BIT)

/* mode of adding data 0 select bit */
#define TSSI_CFG_TRANS_MD_BIT	10
#define TSSI_CFG_TRANS_MD_MASK	( 0x3 << TSSI_CFG_TRANS_MD_BIT)	
#define TSSI_CFG_TRANS_MD_0	(0 << TSSI_CFG_TRANS_MD_BIT)
#define TSSI_CFG_TRANS_MD_1	(1 << TSSI_CFG_TRANS_MD_BIT)
#define TSSI_CFG_TRANS_MD_2	(2 << TSSI_CFG_TRANS_MD_BIT)

#define TSSI_CFG_END_WD 	( 1 << 9 )      /* order of data in word */
#define TSSI_CFG_END_BT 	( 1 << 8 )      /* order of data in byte */

#define TSSI_CFG_TSDI_H 	( 1 << 7 )      /* data pin polarity */
#define TSSI_CFG_USE_0 		( 1 << 6 )      /* serial mode data pin select */
#define TSSI_CFG_USE_TSDI0 	( 1 << 6 )      /* TSDI0 as serial mode data pin */
#define TSSI_CFG_USE_TSDI7 	( 0 << 6 )      /* TSDI7 as serial mode data pin */
#define TSSI_CFG_TSCLK_CH 	( 1 << 5 )      /* clk channel select */
#define TSSI_CFG_PARAL 		( 1 << 4 )      /* mode select */
#define TSSI_CFG_PARAL_MODE 	( 1 << 4 )      /* parallel select */
#define TSSI_CFG_SERIAL_MODE 	( 0 << 4 )      /* serial select */
#define TSSI_CFG_TSCLK_P 	( 1 << 3 )      /* clk edge select */
#define TSSI_CFG_TSFRM_H 	( 1 << 2 )      /* TSFRM polarity select */
#define TSSI_CFG_TSSTR_H 	( 1 << 1 )      /* TSSTR polarity select */
#define TSSI_CFG_TSFAIL_H 	( 1 << 0 )      /* TSFAIL polarity select */

/* TSSI control register */
#define TSSI_CTRL_DTRM		( 1 << 2 ) 	/* FIFO data trigger interrupt mask bit */
#define TSSI_CTRL_OVRNM 	( 1 << 1 )      /* FIFO overrun interrupt mask bit */
#define TSSI_CTRL_TRIGM 	( 1 << 0 )      /* FIFO trigger interrupt mask bit */

/* TSSI state register */
#define TSSI_STAT_DTR		( 1 << 2 )	/* FIFO data trigger interrupt flag bit */
#define TSSI_STAT_OVRN 		( 1 << 1 )      /* FIFO overrun interrupt flag bit */
#define TSSI_STAT_TRIG 		( 1 << 0 )      /* FIFO trigger interrupt flag bit */

/* TSSI PID enable register */
#define TSSI_PEN_EN00 	( 1 << 0 )      /* enable PID n */
#define TSSI_PEN_EN10 	( 1 << 1 )      
#define TSSI_PEN_EN20 	( 1 << 2 )      
#define TSSI_PEN_EN30 	( 1 << 3 )      
#define TSSI_PEN_EN40 	( 1 << 4 )      
#define TSSI_PEN_EN50 	( 1 << 5 )      
#define TSSI_PEN_EN60 	( 1 << 6 )      
#define TSSI_PEN_EN70 	( 1 << 7 )      
#define TSSI_PEN_EN80 	( 1 << 8 )      
#define TSSI_PEN_EN90 	( 1 << 9 )      
#define TSSI_PEN_EN100 	( 1 << 10 )      
#define TSSI_PEN_EN110 	( 1 << 11 )      
#define TSSI_PEN_EN120 	( 1 << 12 )      
#define TSSI_PEN_EN130 	( 1 << 13 )      
#define TSSI_PEN_EN140 	( 1 << 14 )      
#define TSSI_PEN_EN150 	( 1 << 15 )      
#define TSSI_PEN_EN01 	( 1 << 16 )      
#define TSSI_PEN_EN11 	( 1 << 17 )      
#define TSSI_PEN_EN21 	( 1 << 18 )     
#define TSSI_PEN_EN31 	( 1 << 19 )      
#define TSSI_PEN_EN41 	( 1 << 20 )     
#define TSSI_PEN_EN51 	( 1 << 21 )      
#define TSSI_PEN_EN61 	( 1 << 22 )     
#define TSSI_PEN_EN71 	( 1 << 23 )      
#define TSSI_PEN_EN81 	( 1 << 24 )      
#define TSSI_PEN_EN91 	( 1 << 25 )      
#define TSSI_PEN_EN101 	( 1 << 26 )      
#define TSSI_PEN_EN111 	( 1 << 27 )      
#define TSSI_PEN_EN121 	( 1 << 28 )      
#define TSSI_PEN_EN131 	( 1 << 29 )      
#define TSSI_PEN_EN141 	( 1 << 30 )      
#define TSSI_PEN_EN151 	( 1 << 31 )      
//#define TSSI_PEN_PID0 	( 1 << 31 ) /* PID filter enable PID0 */

/* TSSI Data Number Registers */
#define TSSI_DNUM_BIT	0
#define TSSI_DNUM_MASK	(0x7f << TSSI_DNUM_BIT)

/* TSSI Data Trigger Register */
#define TSSI_DTRG_BIT 	0
#define TSSI_DTRG_MASK	(0x7f << TSSI_DTRG_BIT)

/* TSSI PID Filter Registers */
#define TSSI_PID_PID1_BIT 	16
#define TSSI_PID_PID1_MASK 	(0x1fff<<TSSI_PID_PID1_BIT)
#define TSSI_PID_PID0_BIT 	0
#define TSSI_PID_PID0_MASK 	(0x1fff<<TSSI_PID_PID0_BIT)

/* TSSI DMA Identifier Registers */
#define TSSI_DMA_ID_BIT		0
#define TSSI_DMA_ID_MASK	(0xffff << TSSI_DMA_ID_BIT)

/* TSSI DMA Command Registers */
#define TSSI_DCMD_TLEN_BIT	8
#define TSSI_DCMD_TLEN_MASK	(0xff << TSSI_DCMD_TLEN_BIT)
#define TSSI_DCMD_TEFE		(1 << 4)
#define TSSI_DCMD_TSZ_BIT	2
#define TSSI_DCMD_TSZ_MASK	(0x3 << TSSI_DCMD_TSZ_BIT)
#define TSSI_DCMD_TSZ_4		(0 << TSSI_DCMD_TSZ_BIT)
#define TSSI_DCMD_TSZ_8		(1 << TSSI_DCMD_TSZ_BIT)
#define TSSI_DCMD_TSZ_16	(2 << TSSI_DCMD_TSZ_BIT)
#define TSSI_DCMD_TSZ_32	(3 << TSSI_DCMD_TSZ_BIT)
#define TSSI_DCMD_TEIE		(1 << 1)
#define TSSI_DCMD_LINK		(1 << 0)

/* TSSI DMA Status Registers */
#define TSSI_DST_DID_BIT	16
#define TSSI_DST_DID_MASK	(0xffff << 16)
#define TSSI_DST_TEND		(1 << 0)

/* TSSI Transfer Control Registers */
#define TSSI_TC_OP_BIT		4
#define TSSI_TC_OP_MASK		(0x3 << TSSI_TC_OP_BIT)
//////////////////#define TSSI_TC_OP_0		( 
#define TSSI_TC_OPE		(1 << 2)
#define TSSI_TC_EME		(1 << 1)
#define TSSI_TC_APM		(1 << 0)
#ifndef __MIPS_ASSEMBLER

/*************************************************************************
 * TSSI MPEG 2-TS slave interface operation
 *************************************************************************/
#define __tssi_enable()                       ( REG_TSSI_ENA |= TSSI_ENA_ENA )
#define __tssi_disable()                      ( REG_TSSI_ENA &= ~TSSI_ENA_ENA )
#define __tssi_soft_reset()                   ( REG_TSSI_ENA |= TSSI_ENA_SFT_RST )
#define __tssi_filter_enable_pid0()	      ( REG_TSSI_ENA |= TSSI_ENA_PEN_0)
#define __tssi_filter_disable_pid0()	      ( REG_TSSI_ENA &= ~TSSI_ENA_PEN_0)
#define __tssi_dma_enable()                   ( REG_TSSI_ENA |= TSSI_ENA_DMA_EN )
#define __tssi_dma_disable()                  ( REG_TSSI_ENA &= ~TSSI_ENA_DMA_EN )
#define __tssi_filter_enable()                ( REG_TSSI_ENA |= TSSI_ENA_PID_EN )
#define __tssi_filter_disable()               ( REG_TSSI_ENA &= ~TSSI_ENA_PID_EN )

/* n = 4, 8, 16 */
#define __tssi_set_tigger_num(n)			\
	do {						\
		REG_TSSI_CFG &= ~TSSI_CFG_TRIG_MASK;	\
		REG_TSSI_CFG |= TSSI_CFG_TRIG_##n;	\
	} while (0)

#define __tssi_set_data0_mode(n)				\
	do {							\
		REG_TSSI_CFG &= ~ TSSI_CFG_TRANS_MD_MASK;	\
		REG_TSSI_CFG |= TSSI_CFG_TRANS_MD_##n;		\
	}  while(0)

#define __tssi_set_wd_1()                     ( REG_TSSI_CFG |= TSSI_CFG_END_WD )
#define __tssi_set_wd_0()                     ( REG_TSSI_CFG &= ~TSSI_CFG_END_WD )

#define __tssi_set_bt_1()                     ( REG_TSSI_CFG |= TSSI_CFG_END_BD )
#define __tssi_set_bt_0()                     ( REG_TSSI_CFG &= ~TSSI_CFG_END_BD )

#define __tssi_set_data_pola_high()           ( REG_TSSI_CFG |= TSSI_CFG_TSDI_H )
#define __tssi_set_data_pola_low()            ( REG_TSSI_CFG &= ~TSSI_CFG_TSDI_H )

#define __tssi_set_data_use_data0()           ( REG_TSSI_CFG |= TSSI_CFG_USE_0 )
#define __tssi_set_data_use_data7()           ( REG_TSSI_CFG &= ~TSSI_CFG_USE_0 )

#define __tssi_select_clk_fast()              ( REG_TSSI_CFG &= ~TSSI_CFG_TSCLK_CH )
#define __tssi_select_clk_slow()              ( REG_TSSI_CFG |= TSSI_CFG_TSCLK_CH )

#define __tssi_select_serail_mode()           ( REG_TSSI_CFG &= ~TSSI_CFG_PARAL )
#define __tssi_select_paral_mode()            ( REG_TSSI_CFG |= TSSI_CFG_PARAL )

#define __tssi_select_clk_nega_edge()         ( REG_TSSI_CFG &= ~TSSI_CFG_TSCLK_P )
#define __tssi_select_clk_posi_edge()         ( REG_TSSI_CFG |= TSSI_CFG_TSCLK_P )

#define __tssi_select_frm_act_high()          ( REG_TSSI_CFG |= TSSI_CFG_TSFRM_H )
#define __tssi_select_frm_act_low()           ( REG_TSSI_CFG &= ~TSSI_CFG_TSFRM_H )

#define __tssi_select_str_act_high()          ( REG_TSSI_CFG |= TSSI_CFG_TSSTR_H )
#define __tssi_select_str_act_low()           ( REG_TSSI_CFG &= ~TSSI_CFG_TSSTR_H )

#define __tssi_select_fail_act_high()         ( REG_TSSI_CFG |= TSSI_CFG_TSFAIL_H )
#define __tssi_select_fail_act_low()          ( REG_TSSI_CFG &= ~TSSI_CFG_TSFAIL_H )

#define __tssi_enable_data_trigger_irq()      (REG_TSSI_CTRL &= ~TSSI_CTRL_DTRM)
#define __tssi_disable_data_trigger_irq()     (REG_TSSI_CTRL |= TSSI_CTRL_DTRM)

#define __tssi_enable_ovrn_irq()              ( REG_TSSI_CTRL &= ~TSSI_CTRL_OVRNM )
#define __tssi_disable_ovrn_irq()             ( REG_TSSI_CTRL |= TSSI_CTRL_OVRNM )

#define __tssi_enable_trig_irq()              ( REG_TSSI_CTRL &= ~TSSI_CTRL_TRIGM )
#define __tssi_disable_trig_irq()             ( REG_TSSI_CTRL |= TSSI_CTRL_TRIGM ) 

#define __tssi_state_is_dtr()		      ( REG_TSSI_STAT & TSSI_STAT_DTR )
#define __tssi_state_is_overrun()             ( REG_TSSI_STAT & TSSI_STAT_OVRN )
#define __tssi_state_trigger_meet()           ( REG_TSSI_STAT & TSSI_STAT_TRIG )
#define __tssi_clear_state()                  ( REG_TSSI_STAT = 0 ) /* write 0??? */
#define __tssi_state_clear_overrun()          ( REG_TSSI_STAT = TSSI_STAT_OVRN )   //??????? xyma

//#define __tssi_enable_filte_pid0()            ( REG_TSSI_PEN |= TSSI_PEN_PID0 )
//#define __tssi_disable_filte_pid0()           ( REG_TSSI_PEN &= ~TSSI_PEN_PID0 )

/* m = 0, ..., 31 */
////////////////???????????????????????????????????????????????????????????

#define __tssi_enable_pid_filter(m)				\
	do {							\
		int n = (m);					\
		if ( n>=0 && n <(TSSI_PID_MAX*2) ) {		\
			REG_TSSI_PEN |= ( 1 << n );		\
		}						\
	} while (0)

/* m = 0, ..., 31 */
#define __tssi_disable_pid_filter(m)				       \
	do {							       \
		int n = (m);					       \
		if ( n>=0 && n <(TSSI_PID_MAX*2) ) {		       \
			REG_TSSI_PEN &= ~( 1 << n );		       \
		}						       \
	} while (0)

/* n = 0, ..., 15 */
#define __tssi_set_pid0(n, pid0)					\
	do {								\
		REG_TSSI_PID(n) &= ~TSSI_PID_PID0_MASK;			\
		REG_TSSI_PID(n) |= ((pid0)<<TSSI_PID_PID0_BIT)&TSSI_PID_PID0_MASK; \
	}while (0)
/* n = 0, ..., 15 */
#define __tssi_set_pid1(n, pid1)					\
	do {								\
		REG_TSSI_PID(n) &= ~TSSI_PID_PID1_MASK;			\
		REG_TSSI_PID(n) |= ((pid1)<<TSSI_PID_PID1_BIT)&TSSI_PID_PID1_MASK; \
	}while (0)

/* n = 0, ..., 15 */
#define __tssi_set_pid(n, pid)						\
	do {								\
		if ( n>=0 && n < TSSI_PID_MAX*2) {			\
			if ( n < TSSI_PID_MAX )				\
				__tssi_set_pid0(n, pid);		\
			else						\
				__tssi_set_pid1(n-TSSI_PID_MAX, pid);	\
		}							\
	}while (0)

#endif /* __MIPS_ASSEMBLER */

#define	TVE_BASE	0xB3050100 

/*************************************************************************
 * TVE (TV Encoder Controller)
 *************************************************************************/
#define TVE_CTRL	(TVE_BASE + 0x40) /* TV Encoder Control register */
#define TVE_FRCFG	(TVE_BASE + 0x44) /* Frame configure register */
#define TVE_SLCFG1	(TVE_BASE + 0x50) /* TV signal level configure register 1 */
#define TVE_SLCFG2	(TVE_BASE + 0x54) /* TV signal level configure register 2*/
#define TVE_SLCFG3	(TVE_BASE + 0x58) /* TV signal level configure register 3*/
#define TVE_LTCFG1	(TVE_BASE + 0x60) /* Line timing configure register 1 */
#define TVE_LTCFG2	(TVE_BASE + 0x64) /* Line timing configure register 2 */
#define TVE_CFREQ	(TVE_BASE + 0x70) /* Chrominance sub-carrier frequency configure register */
#define TVE_CPHASE	(TVE_BASE + 0x74) /* Chrominance sub-carrier phase configure register */
#define TVE_CBCRCFG	(TVE_BASE + 0x78) /* Chrominance filter configure register */
#define TVE_WSSCR	(TVE_BASE + 0x80) /* Wide screen signal control register */
#define TVE_WSSCFG1	(TVE_BASE + 0x84) /* Wide screen signal configure register 1 */
#define TVE_WSSCFG2	(TVE_BASE + 0x88) /* Wide screen signal configure register 2 */
#define TVE_WSSCFG3	(TVE_BASE + 0x8c) /* Wide screen signal configure register 3 */

#define REG_TVE_CTRL     REG32(TVE_CTRL)
#define REG_TVE_FRCFG    REG32(TVE_FRCFG)
#define REG_TVE_SLCFG1   REG32(TVE_SLCFG1)
#define REG_TVE_SLCFG2   REG32(TVE_SLCFG2)
#define REG_TVE_SLCFG3   REG32(TVE_SLCFG3)
#define REG_TVE_LTCFG1   REG32(TVE_LTCFG1)
#define REG_TVE_LTCFG2   REG32(TVE_LTCFG2)
#define REG_TVE_CFREQ    REG32(TVE_CFREQ)
#define REG_TVE_CPHASE   REG32(TVE_CPHASE)
#define REG_TVE_CBCRCFG	 REG32(TVE_CBCRCFG)
#define REG_TVE_WSSCR    REG32(TVE_WSSCR)
#define REG_TVE_WSSCFG1  REG32(TVE_WSSCFG1)
#define REG_TVE_WSSCFG2	 REG32(TVE_WSSCFG2)
#define REG_TVE_WSSCFG3  REG32(TVE_WSSCFG3)

/* TV Encoder Control register */
#define TVE_CTRL_EYCBCR         (1 << 25)    /* YCbCr_enable */
#define TVE_CTRL_ECVBS          (1 << 24)    /* 1: cvbs_enable 0: s-video*/
#define TVE_CTRL_DAPD3	        (1 << 23)    /* DAC 3 power down */
#define TVE_CTRL_DAPD2	        (1 << 22)    /* DAC 2 power down */	
#define TVE_CTRL_DAPD1	        (1 << 21)    /* DAC 1 power down */	
#define TVE_CTRL_DAPD           (1 << 20)    /* power down all DACs */
#define TVE_CTRL_YCDLY_BIT      16
#define TVE_CTRL_YCDLY_MASK     (0x7 << TVE_CTRL_YCDLY_BIT)
#define TVE_CTRL_CGAIN_BIT      14
#define TVE_CTRL_CGAIN_MASK     (0x3 << TVE_CTRL_CGAIN_BIT)
  #define TVE_CTRL_CGAIN_FULL		(0 << TVE_CTRL_CGAIN_BIT) /* gain = 1 */
  #define TVE_CTRL_CGAIN_QUTR		(1 << TVE_CTRL_CGAIN_BIT) /* gain = 1/4 */
  #define TVE_CTRL_CGAIN_HALF		(2 << TVE_CTRL_CGAIN_BIT) /* gain = 1/2 */
  #define TVE_CTRL_CGAIN_THREE_QURT	(3 << TVE_CTRL_CGAIN_BIT) /* gain = 3/4 */
#define TVE_CTRL_CBW_BIT        12
#define TVE_CTRL_CBW_MASK       (0x3 << TVE_CTRL_CBW_BIT)
  #define TVE_CTRL_CBW_NARROW	(0 << TVE_CTRL_CBW_BIT) /* Narrow band */
  #define TVE_CTRL_CBW_WIDE	(1 << TVE_CTRL_CBW_BIT) /* Wide band */
  #define TVE_CTRL_CBW_EXTRA	(2 << TVE_CTRL_CBW_BIT) /* Extra wide band */
  #define TVE_CTRL_CBW_ULTRA	(3 << TVE_CTRL_CBW_BIT) /* Ultra wide band */
#define TVE_CTRL_SYNCT          (1 << 9)
#define TVE_CTRL_PAL            (1 << 8) /* 1: PAL, 0: NTSC */
#define TVE_CTRL_FINV           (1 << 7) /* invert_top:1-invert top and bottom fields. */
#define TVE_CTRL_ZBLACK         (1 << 6) /* bypass_yclamp:1-Black of luminance (Y) input is 0.*/
#define TVE_CTRL_CR1ST          (1 << 5) /* uv_order:0-Cb before Cr,1-Cr before Cb */
#define TVE_CTRL_CLBAR          (1 << 4) /* Color bar mode:0-Output input video to TV,1-Output color bar to TV */
#define TVE_CTRL_SWRST          (1 << 0) /* Software reset:1-TVE is reset */

/* Signal level configure register 1 */
#define TVE_SLCFG1_BLACKL_BIT   0
#define TVE_SLCFG1_BLACKL_MASK  (0x3ff << TVE_SLCFG1_BLACKL_BIT)
#define TVE_SLCFG1_WHITEL_BIT   16
#define TVE_SLCFG1_WHITEL_MASK  (0x3ff << TVE_SLCFG1_WHITEL_BIT)

/* Signal level configure register 2 */
#define TVE_SLCFG2_BLANKL_BIT    0
#define TVE_SLCFG2_BLANKL_MASK   (0x3ff << TVE_SLCFG2_BLANKL_BIT)
#define TVE_SLCFG2_VBLANKL_BIT   16
#define TVE_SLCFG2_VBLANKL_MASK  (0x3ff << TVE_SLCFG2_VBLANKL_BIT)

/* Signal level configure register 3 */
#define TVE_SLCFG3_SYNCL_BIT   0
#define TVE_SLCFG3_SYNCL_MASK  (0xff << TVE_SLCFG3_SYNCL_BIT)

/* Line timing configure register 1 */
#define TVE_LTCFG1_BACKP_BIT   0
#define TVE_LTCFG1_BACKP_MASK  (0x7f << TVE_LTCFG1_BACKP_BIT)
#define TVE_LTCFG1_HSYNCW_BIT   8
#define TVE_LTCFG1_HSYNCW_MASK  (0x7f << TVE_LTCFG1_HSYNCW_BIT)
#define TVE_LTCFG1_FRONTP_BIT   16
#define TVE_LTCFG1_FRONTP_MASK  (0x1f << TVE_LTCFG1_FRONTP_BIT)

/* Line timing configure register 2 */
#define TVE_LTCFG2_BURSTW_BIT    0
#define TVE_LTCFG2_BURSTW_MASK   (0x3f << TVE_LTCFG2_BURSTW_BIT)
#define TVE_LTCFG2_PREBW_BIT     8
#define TVE_LTCFG2_PREBW_MASK    (0x1f << TVE_LTCFG2_PREBW_BIT)
#define TVE_LTCFG2_ACTLIN_BIT    16
#define TVE_LTCFG2_ACTLIN_MASK	(0x7ff << TVE_LTCFG2_ACTLIN_BIT)

/* Chrominance sub-carrier phase configure register */
#define TVE_CPHASE_CCRSTP_BIT    0
#define TVE_CPHASE_CCRSTP_MASK   (0x3 << TVE_CPHASE_CCRSTP_BIT)
  #define TVE_CPHASE_CCRSTP_8	(0 << TVE_CPHASE_CCRSTP_BIT) /* Every 8 field */
  #define TVE_CPHASE_CCRSTP_4	(1 << TVE_CPHASE_CCRSTP_BIT) /* Every 4 field */
  #define TVE_CPHASE_CCRSTP_2	(2 << TVE_CPHASE_CCRSTP_BIT) /* Every 2 lines */
  #define TVE_CPHASE_CCRSTP_0	(3 << TVE_CPHASE_CCRSTP_BIT) /* Never */
#define TVE_CPHASE_ACTPH_BIT     16
#define TVE_CPHASE_ACTPH_MASK    (0xff << TVE_CPHASE_ACTPH_BIT)
#define TVE_CPHASE_INITPH_BIT    24
#define TVE_CPHASE_INITPH_MASK   (0xff << TVE_CPHASE_INITPH_BIT)

/* Chrominance filter configure register */
#define TVE_CBCRCFG_CRGAIN_BIT       0
#define TVE_CBCRCFG_CRGAIN_MASK      (0xff << TVE_CBCRCFG_CRGAIN_BIT)
#define TVE_CBCRCFG_CBGAIN_BIT       8
#define TVE_CBCRCFG_CBGAIN_MASK      (0xff << TVE_CBCRCFG_CBGAIN_BIT)
#define TVE_CBCRCFG_CRBA_BIT         16
#define TVE_CBCRCFG_CRBA_MASK        (0xff << TVE_CBCRCFG_CRBA_BIT)
#define TVE_CBCRCFG_CBBA_BIT         24
#define TVE_CBCRCFG_CBBA_MASK        (0xff << TVE_CBCRCFG_CBBA_BIT)

/* Frame configure register */
#define TVE_FRCFG_NLINE_BIT          0
#define TVE_FRCFG_NLINE_MASK         (0x3ff << TVE_FRCFG_NLINE_BIT)
#define TVE_FRCFG_L1ST_BIT           16
#define TVE_FRCFG_L1ST_MASK          (0xff << TVE_FRCFG_L1ST_BIT)

/* Wide screen signal control register */
#define TVE_WSSCR_EWSS0_BIT	0
#define TVE_WSSCR_EWSS1_BIT	1
#define TVE_WSSCR_WSSTP_BIT	2
#define TVE_WSSCR_WSSCKBP_BIT	3
#define TVE_WSSCR_WSSEDGE_BIT	4
#define TVE_WSSCR_WSSEDGE_MASK	(0x7 << TVE_WSSCR_WSSEDGE_BIT)
#define TVE_WSSCR_ENCH_BIT	8
#define TVE_WSSCR_NCHW_BIT	9
#define TVE_WSSCR_NCHFREQ_BIT	12
#define TVE_WSSCR_NCHFREQ_MASK	(0x7 << TVE_WSSCR_NCHFREQ_BIT)

#ifndef __MIPS_ASSEMBLER

/*************************************************************************
 * TVE (TV Encoder Controller) ops
 *************************************************************************/
/* TV Encoder Control register ops */
#define __tve_soft_reset()		(REG_TVE_CTRL |= TVE_CTRL_SWRST)

#define __tve_output_colorbar()		(REG_TVE_CTRL |= TVE_CTRL_CLBAR)
#define __tve_output_video()		(REG_TVE_CTRL &= ~TVE_CTRL_CLBAR)

#define __tve_input_cr_first()		(REG_TVE_CTRL |= TVE_CTRL_CR1ST)
#define __tve_input_cb_first()		(REG_TVE_CTRL &= ~TVE_CTRL_CR1ST)

#define __tve_set_0_as_black()		(REG_TVE_CTRL |= TVE_CTRL_ZBLACK)
#define __tve_set_16_as_black()		(REG_TVE_CTRL &= ~TVE_CTRL_ZBLACK)

#define __tve_ena_invert_top_bottom()	(REG_TVE_CTRL |= TVE_CTRL_FINV)
#define __tve_dis_invert_top_bottom()	(REG_TVE_CTRL &= ~TVE_CTRL_FINV)

#define __tve_set_pal_mode()		(REG_TVE_CTRL |= TVE_CTRL_PAL)
#define __tve_set_ntsc_mode()		(REG_TVE_CTRL &= ~TVE_CTRL_PAL)

#define __tve_set_pal_dura()		(REG_TVE_CTRL |= TVE_CTRL_SYNCT)
#define __tve_set_ntsc_dura()		(REG_TVE_CTRL &= ~TVE_CTRL_SYNCT)

/* n = 0 ~ 3 */
#define __tve_set_c_bandwidth(n) \
do {\
	REG_TVE_CTRL &= ~TVE_CTRL_CBW_MASK;\
	REG_TVE_CTRL |= (n) << TVE_CTRL_CBW_BIT;	\
}while(0)

/* n = 0 ~ 3 */
#define __tve_set_c_gain(n) \
do {\
	REG_TVE_CTRL &= ~TVE_CTRL_CGAIN_MASK;\
	(REG_TVE_CTRL |= (n) << TVE_CTRL_CGAIN_BIT;	\
}while(0)

/* n = 0 ~ 7 */
#define __tve_set_yc_delay(n)				\
do {							\
	REG_TVE_CTRL &= ~TVE_CTRL_YCDLY_MASK		\
	REG_TVE_CTRL |= ((n) << TVE_CTRL_YCDLY_BIT);	\
} while(0)

#define __tve_disable_all_dacs()	(REG_TVE_CTRL |= TVE_CTRL_DAPD)
#define __tve_disable_dac1()		(REG_TVE_CTRL |= TVE_CTRL_DAPD1)
#define __tve_enable_dac1()		(REG_TVE_CTRL &= ~TVE_CTRL_DAPD1)
#define __tve_disable_dac2()		(REG_TVE_CTRL |= TVE_CTRL_DAPD2)
#define __tve_enable_dac2()		(REG_TVE_CTRL &= ~TVE_CTRL_DAPD2)
#define __tve_disable_dac3()		(REG_TVE_CTRL |= TVE_CTRL_DAPD3)
#define __tve_enable_dac3()		(REG_TVE_CTRL &= ~TVE_CTRL_DAPD3)

#define __tve_enable_svideo_fmt()	(REG_TVE_CTRL |= TVE_CTRL_ECVBS)
#define __tve_enable_cvbs_fmt()		(REG_TVE_CTRL &= ~TVE_CTRL_ECVBS)

/* TV Encoder Frame Configure register ops */
/* n = 0 ~ 255 */
#define __tve_set_first_video_line(n)		\
do {\
		REG_TVE_FRCFG &= ~TVE_FRCFG_L1ST_MASK;\
		REG_TVE_FRCFG |= (n) << TVE_FRCFG_L1ST_BIT;\
} while(0)
/* n = 0 ~ 1023 */
#define __tve_set_line_num_per_frm(n)		\
do {\
		REG_TVE_FRCFG &= ~TVE_FRCFG_NLINE_MASK;\
		REG_TVE_CFG |= (n) << TVE_FRCFG_NLINE_BIT;\
} while(0)
#define __tve_get_video_line_num()\
	(((REG_TVE_FRCFG & TVE_FRCFG_NLINE_MASK) >> TVE_FRCFG_NLINE_BIT) - 1 - 2 * ((REG_TVE_FRCFG & TVE_FRCFG_L1ST_MASK) >> TVE_FRCFG_L1ST_BIT))

/* TV Encoder Signal Level Configure register ops */
/* n = 0 ~ 1023 */
#define __tve_set_white_level(n)		\
do {\
		REG_TVE_SLCFG1 &= ~TVE_SLCFG1_WHITEL_MASK;\
		REG_TVE_SLCFG1 |= (n) << TVE_SLCFG1_WHITEL_BIT;\
} while(0)
/* n = 0 ~ 1023 */
#define __tve_set_black_level(n)		\
do {\
		REG_TVE_SLCFG1 &= ~TVE_SLCFG1_BLACKL_MASK;\
		REG_TVE_SLCFG1 |= (n) << TVE_SLCFG1_BLACKL_BIT;\
} while(0)
/* n = 0 ~ 1023 */
#define __tve_set_blank_level(n)		\
do {\
		REG_TVE_SLCFG2 &= ~TVE_SLCFG2_BLANKL_MASK;\
		REG_TVE_SLCFG2 |= (n) << TVE_SLCFG2_BLANKL_BIT;\
} while(0)
/* n = 0 ~ 1023 */
#define __tve_set_vbi_blank_level(n)		\
do {\
		REG_TVE_SLCFG2 &= ~TVE_SLCFG2_VBLANKL_MASK;\
		REG_TVE_SLCFG2 |= (n) << TVE_SLCFG2_VBLANKL_BIT;\
} while(0)
/* n = 0 ~ 1023 */
#define __tve_set_sync_level(n)		\
do {\
		REG_TVE_SLCFG3 &= ~TVE_SLCFG3_SYNCL_MASK;\
		REG_TVE_SLCFG3 |= (n) << TVE_SLCFG3_SYNCL_BIT;\
} while(0)

/* TV Encoder Signal Level Configure register ops */
/* n = 0 ~ 31 */
#define __tve_set_front_porch(n)		\
do {\
		REG_TVE_LTCFG1 &= ~TVE_LTCFG1_FRONTP_MASK;\
		REG_TVE_LTCFG1 |= (n) << TVE_LTCFG1_FRONTP_BIT;	\
} while(0)
/* n = 0 ~ 127 */
#define __tve_set_hsync_width(n)		\
do {\
		REG_TVE_LTCFG1 &= ~TVE_LTCFG1_HSYNCW_MASK;\
		REG_TVE_LTCFG1 |= (n) << TVE_LTCFG1_HSYNCW_BIT;	\
} while(0)
/* n = 0 ~ 127 */
#define __tve_set_back_porch(n)		\
do {\
		REG_TVE_LTCFG1 &= ~TVE_LTCFG1_BACKP_MASK;\
		REG_TVE_LTCFG1 |= (n) << TVE_LTCFG1_BACKP_BIT;	\
} while(0)
/* n = 0 ~ 2047 */
#define __tve_set_active_linec(n)		\
do {\
		REG_TVE_LTCFG2 &= ~TVE_LTCFG2_ACTLIN_MASK;\
		REG_TVE_LTCFG2 |= (n) << TVE_LTCFG2_ACTLIN_BIT;	\
} while(0)
/* n = 0 ~ 31 */
#define __tve_set_breezy_way(n)		\
do {\
		REG_TVE_LTCFG2 &= ~TVE_LTCFG2_PREBW_MASK;\
		REG_TVE_LTCFG2 |= (n) << TVE_LTCFG2_PREBW_BIT;	\
} while(0)

/* n = 0 ~ 127 */
#define __tve_set_burst_width(n)		\
do {\
		REG_TVE_LTCFG2 &= ~TVE_LTCFG2_BURSTW_MASK;\
		REG_TVE_LTCFG2 |= (n) << TVE_LTCFG2_BURSTW_BIT;	\
} while(0)

/* TV Encoder Chrominance filter and Modulation register ops */
/* n = 0 ~ (2^32-1) */
#define __tve_set_c_sub_carrier_freq(n)  REG_TVE_CFREQ = (n)
/* n = 0 ~ 255 */
#define __tve_set_c_sub_carrier_init_phase(n) \
do {   \
	REG_TVE_CPHASE &= ~TVE_CPHASE_INITPH_MASK;	\
	REG_TVE_CPHASE |= (n) << TVE_CPHASE_INITPH_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_c_sub_carrier_act_phase(n) \
do {   \
	REG_TVE_CPHASE &= ~TVE_CPHASE_ACTPH_MASK;	\
	REG_TVE_CPHASE |= (n) << TVE_CPHASE_ACTPH_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_c_phase_rst_period(n) \
do {   \
	REG_TVE_CPHASE &= ~TVE_CPHASE_CCRSTP_MASK;	\
	REG_TVE_CPHASE |= (n) << TVE_CPHASE_CCRSTP_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_cb_burst_amp(n) \
do {   \
	REG_TVE_CBCRCFG &= ~TVE_CBCRCFG_CBBA_MASK;	\
	REG_TVE_CBCRCFG |= (n) << TVE_CBCRCFG_CBBA_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_cr_burst_amp(n) \
do {   \
	REG_TVE_CBCRCFG &= ~TVE_CBCRCFG_CRBA_MASK;	\
	REG_TVE_CBCRCFG |= (n) << TVE_CBCRCFG_CRBA_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_cb_gain_amp(n) \
do {   \
	REG_TVE_CBCRCFG &= ~TVE_CBCRCFG_CBGAIN_MASK;	\
	REG_TVE_CBCRCFG |= (n) << TVE_CBCRCFG_CBGAIN_BIT;	\
} while(0)
/* n = 0 ~ 255 */
#define __tve_set_cr_gain_amp(n) \
do {   \
	REG_TVE_CBCRCFG &= ~TVE_CBCRCFG_CRGAIN_MASK;	\
	REG_TVE_CBCRCFG |= (n) << TVE_CBCRCFG_CRGAIN_BIT;	\
} while(0)

/* TV Encoder Wide Screen Signal Control register ops */
/* n = 0 ~ 7 */
#define __tve_set_notch_freq(n) \
do {   \
	REG_TVE_WSSCR &= ~TVE_WSSCR_NCHFREQ_MASK;	\
	REG_TVE_WSSCR |= (n) << TVE_WSSCR_NCHFREQ_BIT;	\
} while(0)
/* n = 0 ~ 7 */
#define __tve_set_notch_width()	(REG_TVE_WSSCR |= TVE_WSSCR_NCHW_BIT)
#define __tve_clear_notch_width()	(REG_TVE_WSSCR &= ~TVE_WSSCR_NCHW_BIT)
#define __tve_enable_notch()		(REG_TVE_WSSCR |= TVE_WSSCR_ENCH_BIT)
#define __tve_disable_notch()		(REG_TVE_WSSCR &= ~TVE_WSSCR_ENCH_BIT)
/* n = 0 ~ 7 */
#define __tve_set_wss_edge(n) \
do {   \
	REG_TVE_WSSCR &= ~TVE_WSSCR_WSSEDGE_MASK;	\
	REG_TVE_WSSCR |= (n) << TVE_WSSCR_WSSEDGE_BIT;	\
} while(0)
#define __tve_set_wss_clkbyp()		(REG_TVE_WSSCR |= TVE_WSSCR_WSSCKBP_BIT)
#define __tve_set_wss_type()		(REG_TVE_WSSCR |= TVE_WSSCR_WSSTP_BIT)
#define __tve_enable_wssf1()		(REG_TVE_WSSCR |= TVE_WSSCR_EWSS1_BIT)
#define __tve_enable_wssf0()		(REG_TVE_WSSCR |= TVE_WSSCR_EWSS0_BIT)

/* TV Encoder Wide Screen Signal Configure register 1, 2 and 3 ops */
/* n = 0 ~ 1023 */
#define __tve_set_wss_level(n) \
do {   \
	REG_TVE_WSSCFG1 &= ~TVE_WSSCFG1_WSSL_MASK;	\
	REG_TVE_WSSCFG1 |= (n) << TVE_WSSCFG1_WSSL_BIT;	\
} while(0)
/* n = 0 ~ 4095 */
#define __tve_set_wss_freq(n) \
do {   \
	REG_TVE_WSSCFG1 &= ~TVE_WSSCFG1_WSSFREQ_MASK;	\
	REG_TVE_WSSCFG1 |= (n) << TVE_WSSCFG1_WSSFREQ_BIT;	\
} while(0)
/* n = 0, 1; l = 0 ~ 255 */
#define __tve_set_wss_line(n,v)			\
do {   \
	REG_TVE_WSSCFG##n &= ~TVE_WSSCFG_WSSLINE_MASK;	\
	REG_TVE_WSSCFG##n |= (v) << TVE_WSSCFG_WSSLINE_BIT;	\
} while(0)
/* n = 0, 1; d = 0 ~ (2^20-1) */
#define __tve_set_wss_data(n, v)			\
do {   \
	REG_TVE_WSSCFG##n &= ~TVE_WSSCFG_WSSLINE_MASK;	\
	REG_TVE_WSSCFG##n |= (v) << TVE_WSSCFG_WSSLINE_BIT;	\
} while(0)

#endif /* __MIPS_ASSEMBLER */

#define	UART0_BASE	0xB0030000
#define	UART1_BASE	0xB0031000
#define	UART2_BASE	0xB0032000
#define	UART3_BASE	0xB0033000

/*************************************************************************
 * UART
 *************************************************************************/

#define IRDA_BASE	UART0_BASE
#define UART_BASE	UART0_BASE
#define UART_OFF	0x1000

/* Register Offset */
#define OFF_RDR		(0x00)	/* R  8b H'xx */
#define OFF_TDR		(0x00)	/* W  8b H'xx */
#define OFF_DLLR	(0x00)	/* RW 8b H'00 */
#define OFF_DLHR	(0x04)	/* RW 8b H'00 */
#define OFF_IER		(0x04)	/* RW 8b H'00 */
#define OFF_ISR		(0x08)	/* R  8b H'01 */
#define OFF_FCR		(0x08)	/* W  8b H'00 */
#define OFF_LCR		(0x0C)	/* RW 8b H'00 */
#define OFF_MCR		(0x10)	/* RW 8b H'00 */
#define OFF_LSR		(0x14)	/* R  8b H'00 */
#define OFF_MSR		(0x18)	/* R  8b H'00 */
#define OFF_SPR		(0x1C)	/* RW 8b H'00 */
#define OFF_SIRCR	(0x20)	/* RW 8b H'00, UART0 */
#define OFF_UMR		(0x24)	/* RW 8b H'00, UART M Register */
#define OFF_UACR	(0x28)	/* RW 8b H'00, UART Add Cycle Register */

/* Register Address */
#define UART0_RDR	(UART0_BASE + OFF_RDR)
#define UART0_TDR	(UART0_BASE + OFF_TDR)
#define UART0_DLLR	(UART0_BASE + OFF_DLLR)
#define UART0_DLHR	(UART0_BASE + OFF_DLHR)
#define UART0_IER	(UART0_BASE + OFF_IER)
#define UART0_ISR	(UART0_BASE + OFF_ISR)
#define UART0_FCR	(UART0_BASE + OFF_FCR)
#define UART0_LCR	(UART0_BASE + OFF_LCR)
#define UART0_MCR	(UART0_BASE + OFF_MCR)
#define UART0_LSR	(UART0_BASE + OFF_LSR)
#define UART0_MSR	(UART0_BASE + OFF_MSR)
#define UART0_SPR	(UART0_BASE + OFF_SPR)
#define UART0_SIRCR	(UART0_BASE + OFF_SIRCR)
#define UART0_UMR	(UART0_BASE + OFF_UMR)
#define UART0_UACR	(UART0_BASE + OFF_UACR)

#define UART1_RDR	(UART1_BASE + OFF_RDR)
#define UART1_TDR	(UART1_BASE + OFF_TDR)
#define UART1_DLLR	(UART1_BASE + OFF_DLLR)
#define UART1_DLHR	(UART1_BASE + OFF_DLHR)
#define UART1_IER	(UART1_BASE + OFF_IER)
#define UART1_ISR	(UART1_BASE + OFF_ISR)
#define UART1_FCR	(UART1_BASE + OFF_FCR)
#define UART1_LCR	(UART1_BASE + OFF_LCR)
#define UART1_MCR	(UART1_BASE + OFF_MCR)
#define UART1_LSR	(UART1_BASE + OFF_LSR)
#define UART1_MSR	(UART1_BASE + OFF_MSR)
#define UART1_SPR	(UART1_BASE + OFF_SPR)
#define UART1_SIRCR	(UART1_BASE + OFF_SIRCR)

#define UART2_RDR	(UART2_BASE + OFF_RDR)
#define UART2_TDR	(UART2_BASE + OFF_TDR)
#define UART2_DLLR	(UART2_BASE + OFF_DLLR)
#define UART2_DLHR	(UART2_BASE + OFF_DLHR)
#define UART2_IER	(UART2_BASE + OFF_IER)
#define UART2_ISR	(UART2_BASE + OFF_ISR)
#define UART2_FCR	(UART2_BASE + OFF_FCR)
#define UART2_LCR	(UART2_BASE + OFF_LCR)
#define UART2_MCR	(UART2_BASE + OFF_MCR)
#define UART2_LSR	(UART2_BASE + OFF_LSR)
#define UART2_MSR	(UART2_BASE + OFF_MSR)
#define UART2_SPR	(UART2_BASE + OFF_SPR)
#define UART2_SIRCR	(UART2_BASE + OFF_SIRCR)

#define UART3_RDR	(UART3_BASE + OFF_RDR)
#define UART3_TDR	(UART3_BASE + OFF_TDR)
#define UART3_DLLR	(UART3_BASE + OFF_DLLR)
#define UART3_DLHR	(UART3_BASE + OFF_DLHR)
#define UART3_IER	(UART3_BASE + OFF_IER)
#define UART3_ISR	(UART3_BASE + OFF_ISR)
#define UART3_FCR	(UART3_BASE + OFF_FCR)
#define UART3_LCR	(UART3_BASE + OFF_LCR)
#define UART3_MCR	(UART3_BASE + OFF_MCR)
#define UART3_LSR	(UART3_BASE + OFF_LSR)
#define UART3_MSR	(UART3_BASE + OFF_MSR)
#define UART3_SPR	(UART3_BASE + OFF_SPR)
#define UART3_SIRCR	(UART3_BASE + OFF_SIRCR)

/*
 * Define macros for UARTIER
 * UART Interrupt Enable Register
 */
#define UARTIER_RIE	(1 << 0)	/* 0: receive fifo full interrupt disable */
#define UARTIER_TIE	(1 << 1)	/* 0: transmit fifo empty interrupt disable */
#define UARTIER_RLIE	(1 << 2)	/* 0: receive line status interrupt disable */
#define UARTIER_MIE	(1 << 3)	/* 0: modem status interrupt disable */
#define UARTIER_RTIE	(1 << 4)	/* 0: receive timeout interrupt disable */

/*
 * Define macros for UARTISR
 * UART Interrupt Status Register
 */
#define UARTISR_IP	(1 << 0)	/* 0: interrupt is pending  1: no interrupt */
#define UARTISR_IID	(7 << 1)	/* Source of Interrupt */
#define UARTISR_IID_MSI		(0 << 1)  /* Modem status interrupt */
#define UARTISR_IID_THRI	(1 << 1)  /* Transmitter holding register empty */
#define UARTISR_IID_RDI		(2 << 1)  /* Receiver data interrupt */
#define UARTISR_IID_RLSI	(3 << 1)  /* Receiver line status interrupt */
#define UARTISR_IID_RTO		(6 << 1)  /* Receive timeout */
#define UARTISR_FFMS		(3 << 6)  /* FIFO mode select, set when UARTFCR.FE is set to 1 */
#define UARTISR_FFMS_NO_FIFO	(0 << 6)
#define UARTISR_FFMS_FIFO_MODE	(3 << 6)

/*
 * Define macros for UARTFCR
 * UART FIFO Control Register
 */
#define UARTFCR_FE	(1 << 0)	/* 0: non-FIFO mode  1: FIFO mode */
#define UARTFCR_RFLS	(1 << 1)	/* write 1 to flush receive FIFO */
#define UARTFCR_TFLS	(1 << 2)	/* write 1 to flush transmit FIFO */
#define UARTFCR_DMS	(1 << 3)	/* 0: disable DMA mode */
#define UARTFCR_UUE	(1 << 4)	/* 0: disable UART */
#define UARTFCR_RTRG	(3 << 6)	/* Receive FIFO Data Trigger */
#define UARTFCR_RTRG_1	(0 << 6)
#define UARTFCR_RTRG_4	(1 << 6)
#define UARTFCR_RTRG_8	(2 << 6)
#define UARTFCR_RTRG_15	(3 << 6)

/*
 * Define macros for UARTLCR
 * UART Line Control Register
 */
#define UARTLCR_WLEN	(3 << 0)	/* word length */
#define UARTLCR_WLEN_5	(0 << 0)
#define UARTLCR_WLEN_6	(1 << 0)
#define UARTLCR_WLEN_7	(2 << 0)
#define UARTLCR_WLEN_8	(3 << 0)
#define UARTLCR_STOP	(1 << 2)	/* 0: 1 stop bit when word length is 5,6,7,8
					   1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */
#define UARTLCR_STOP1	(0 << 2)
#define UARTLCR_STOP2	(1 << 2)
#define UARTLCR_PE	(1 << 3)	/* 0: parity disable */
#define UARTLCR_PROE	(1 << 4)	/* 0: even parity  1: odd parity */
#define UARTLCR_SPAR	(1 << 5)	/* 0: sticky parity disable */
#define UARTLCR_SBRK	(1 << 6)	/* write 0 normal, write 1 send break */
#define UARTLCR_DLAB	(1 << 7)	/* 0: access UARTRDR/TDR/IER  1: access UARTDLLR/DLHR */

/*
 * Define macros for UARTLSR
 * UART Line Status Register
 */
#define UARTLSR_DR	(1 << 0)	/* 0: receive FIFO is empty  1: receive data is ready */
#define UARTLSR_ORER	(1 << 1)	/* 0: no overrun error */
#define UARTLSR_PER	(1 << 2)	/* 0: no parity error */
#define UARTLSR_FER	(1 << 3)	/* 0; no framing error */
#define UARTLSR_BRK	(1 << 4)	/* 0: no break detected  1: receive a break signal */
#define UARTLSR_TDRQ	(1 << 5)	/* 1: transmit FIFO half "empty" */
#define UARTLSR_TEMT	(1 << 6)	/* 1: transmit FIFO and shift registers empty */
#define UARTLSR_RFER	(1 << 7)	/* 0: no receive error  1: receive error in FIFO mode */

/*
 * Define macros for UARTMCR
 * UART Modem Control Register
 */
#define UARTMCR_RTS	(1 << 1)	/* 0: RTS_ output high, 1: RTS_ output low */
#define UARTMCR_LOOP	(1 << 4)	/* 0: normal  1: loopback mode */
#define UARTMCR_FCM	(1 << 6)	/* 0: software  1: hardware */
#define UARTMCR_MCE	(1 << 7)	/* 0: modem function is disable */

/*
 * Define macros for UARTMSR
 * UART Modem Status Register
 */
#define UARTMSR_CCTS	(1 << 0)        /* 1: a change on CTS_ pin */
#define UARTMSR_CTS	(1 << 4)	/* 0: CTS_ pin is high */

/*
 * Define macros for SIRCR
 * Slow IrDA Control Register
 */
#define SIRCR_TSIRE	(1 << 0)  /* 0: transmitter is in UART mode  1: SIR mode */
#define SIRCR_RSIRE	(1 << 1)  /* 0: receiver is in UART mode  1: SIR mode */
#define SIRCR_TPWS	(1 << 2)  /* 0: transmit 0 pulse width is 3/16 of bit length
					   1: 0 pulse width is 1.6us for 115.2Kbps */
#define SIRCR_TDPL	(1 << 3)  /* 0: encoder generates a positive pulse for 0 */
#define SIRCR_RDPL	(1 << 4)  /* 0: decoder interprets positive pulse as 0 */

#ifndef __MIPS_ASSEMBLER

/***************************************************************************
 * UART
 ***************************************************************************/
#define __jtag_as_uart3()			\
do {	                    			\
	REG_GPIO_PXSELC(0) = 0x40000000;	\
	REG_GPIO_PXSELS(0) = 0x80000000;	\
} while(0)

#define __uart_enable(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_FCR) |= UARTFCR_UUE | UARTFCR_FE )
#define __uart_disable(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_FCR) = ~UARTFCR_UUE )

#define __uart_enable_transmit_irq(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_IER) |= UARTIER_TIE )
#define __uart_disable_transmit_irq(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_IER) &= ~UARTIER_TIE )

#define __uart_enable_receive_irq(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_IER) |= UARTIER_RIE | UARTIER_RLIE | UARTIER_RTIE )
#define __uart_disable_receive_irq(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_IER) &= ~(UARTIER_RIE | UARTIER_RLIE | UARTIER_RTIE) )

#define __uart_enable_loopback(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_MCR) |= UARTMCR_LOOP )
#define __uart_disable_loopback(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_MCR) &= ~UARTMCR_LOOP )

#define __uart_set_8n1(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_LCR) = UARTLCR_WLEN_8 )

#define __uart_set_baud(n, devclk, baud)						\
  do {											\
	REG8(UART_BASE + UART_OFF*(n) + OFF_LCR) |= UARTLCR_DLAB;			\
	REG8(UART_BASE + UART_OFF*(n) + OFF_DLLR) = (devclk / 16 / baud) & 0xff;	\
	REG8(UART_BASE + UART_OFF*(n) + OFF_DLHR) = ((devclk / 16 / baud) >> 8) & 0xff;	\
	REG8(UART_BASE + UART_OFF*(n) + OFF_LCR) &= ~UARTLCR_DLAB;			\
  } while (0)

#define __uart_parity_error(n) \
  ( (REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) & UARTLSR_PER) != 0 )

#define __uart_clear_errors(n) \
  ( REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) &= ~(UARTLSR_ORER | UARTLSR_BRK | UARTLSR_FER | UARTLSR_PER | UARTLSR_RFER) )

#define __uart_transmit_fifo_empty(n) \
  ( (REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) & UARTLSR_TDRQ) != 0 )

#define __uart_transmit_end(n) \
  ( (REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) & UARTLSR_TEMT) != 0 )

#define __uart_transmit_char(n, ch) \
  REG8(UART_BASE + UART_OFF*(n) + OFF_TDR) = (ch)

#define __uart_receive_fifo_full(n) \
  ( (REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) & UARTLSR_DR) != 0 )

#define __uart_receive_ready(n) \
  ( (REG8(UART_BASE + UART_OFF*(n) + OFF_LSR) & UARTLSR_DR) != 0 )

#define __uart_receive_char(n) \
  REG8(UART_BASE + UART_OFF*(n) + OFF_RDR)

#define __uart_disable_irda() \
  ( REG8(IRDA_BASE + OFF_SIRCR) &= ~(SIRCR_TSIRE | SIRCR_RSIRE) )
#define __uart_enable_irda() \
  /* Tx high pulse as 0, Rx low pulse as 0 */ \
  ( REG8(IRDA_BASE + OFF_SIRCR) = SIRCR_TSIRE | SIRCR_RSIRE | SIRCR_RXPL | SIRCR_TPWS )

#endif /* __MIPS_ASSEMBLER */

/*
 * Watchdog timer module(WDT) address definition
 */
#define	WDT_BASE	0xb0002000

/*
 * WDT registers offset address definition
 */
#define WDT_WDR_OFFSET		(0x00)	/* rw, 16, 0x???? */
#define WDT_WCER_OFFSET		(0x04)	/* rw,  8, 0x00 */
#define WDT_WCNT_OFFSET		(0x08)	/* rw, 16, 0x???? */
#define WDT_WCSR_OFFSET		(0x0c)	/* rw, 16, 0x0000 */

/*
 * WDT registers address definition
 */
#define WDT_WDR		(WDT_BASE + WDT_WDR_OFFSET)
#define WDT_WCER	(WDT_BASE + WDT_WCER_OFFSET)
#define WDT_WCNT	(WDT_BASE + WDT_WCNT_OFFSET)
#define WDT_WCSR	(WDT_BASE + WDT_WCSR_OFFSET)

/*
 * WDT registers common define
 */

/* Watchdog counter enable register(WCER) */
#define WCER_TCEN	BIT0

/* Watchdog control register(WCSR) */
#define WCSR_PRESCALE_LSB	3
#define WCSR_PRESCALE_MASK	BITS_H2L(5, WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE1		(0x0 << WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE4		(0x1 << WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE16		(0x2 << WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE64		(0x3 << WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE256	(0x4 << WCSR_PRESCALE_LSB)
#define WCSR_PRESCALE1024	(0x5 << WCSR_PRESCALE_LSB)

#define WCSR_CLKIN_LSB		0
#define WCSR_CLKIN_MASK		BITS_H2L(2, WCSR_CLKIN_LSB)
#define WCSR_CLKIN_PCK		(0x1 << WCSR_CLKIN_LSB)
#define WCSR_CLKIN_RTC		(0x2 << WCSR_CLKIN_LSB)
#define WCSR_CLKIN_EXT		(0x4 << WCSR_CLKIN_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_WDT_WDR	REG16(WDT_WDR)
#define REG_WDT_WCER	REG8(WDT_WCER)
#define REG_WDT_WCNT	REG16(WDT_WCNT)
#define REG_WDT_WCSR	REG16(WDT_WCSR)

#endif /* __MIPS_ASSEMBLER */

/*
 * Operating system timer module(OST) address definition
 */
#define	OST_BASE	0xb0002000

/*
 * OST registers offset address definition
 */
#define OST_OSTDR_OFFSET	(0xe0)  /* rw, 32, 0x???????? */
#define OST_OSTCNT_OFFSET	(0xe4)  /* rw, 32, 0x???????? */
#define OST_OSTCNTL_OFFSET	(0xe4)  /* rw, 32, 0x???????? */
#define OST_OSTCNTH_OFFSET	(0xe8)  /* rw, 32, 0x???????? */
#define OST_OSTCSR_OFFSET	(0xec)  /* rw, 16, 0x0000 */
#define OST_OSTCNTHBUF_OFFSET	(0xfc)	/*  r, 32, 0x???????? */

/*
 * OST registers address definition
 */
#define OST_OSTDR	(OST_BASE + OST_OSTDR_OFFSET)
#define OST_OSTCNT	(OST_BASE + OST_OSTCNT_OFFSET)
#define OST_OSTCNTL	(OST_BASE + OST_OSTCNTL_OFFSET)
#define OST_OSTCNTH	(OST_BASE + OST_OSTCNTH_OFFSET)
#define OST_OSTCSR	(OST_BASE + OST_OSTCSR_OFFSET)
#define OST_OSTCNTHBUF	(OST_BASE + OST_OSTCNTHBUF_OFFSET)

/*
 * OST registers common define
 */

/* Operating system control register(OSTCSR) */
#define OSTCSR_CNT_MD		BIT15
#define OSTCSR_SD		BIT9
#define OSTCSR_EXT_EN		BIT2
#define OSTCSR_RTC_EN		BIT1
#define OSTCSR_PCK_EN		BIT0

#define OSTCSR_PRESCALE_LSB	3
#define OSTCSR_PRESCALE_MASK	BITS_H2L(5, OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE1	(0x0 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE4	(0x1 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE16	(0x2 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE64	(0x3 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE256	(0x4 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE1024	(0x5 << OSTCSR_PRESCALE_LSB)

#ifndef __MIPS_ASSEMBLER

#define REG_OST_OSTDR		REG32(OST_OSTDR)
#define REG_OST_OSTCNT		REG32(OST_OSTCNT)
#define REG_OST_OSTCNTL		REG32(OST_OSTCNTL)
#define REG_OST_OSTCNTH		REG32(OST_OSTCNTH)
#define REG_OST_OSTCSR		REG16(OST_OSTCSR)
#define REG_OST_OSTCNTHBUF	REG32(OST_OSTCNTHBUF)

#endif /* __MIPS_ASSEMBLER */

#define AOSD_BASE        0xB3070000

/*************************************************************************
 * OSD (On Screen Display)
 *************************************************************************/
#define AOSD_ADDR0             (AOSD_BASE + 0x00)
#define AOSD_ADDR1             (AOSD_BASE + 0x04)
#define AOSD_ADDR2             (AOSD_BASE + 0x08)
#define AOSD_ADDR3             (AOSD_BASE + 0x0C)
#define AOSD_WADDR             (AOSD_BASE + 0x10)
#define AOSD_ADDRLEN           (AOSD_BASE + 0x14)
#define AOSD_ALPHA_VALUE       (AOSD_BASE + 0x18)
#define AOSD_CTRL              (AOSD_BASE + 0x1C)
#define AOSD_INT               (AOSD_BASE + 0x20)

#define REG_AOSD_ADDR0         REG32(AOSD_ADDR0)
#define REG_AOSD_ADDR1         REG32(AOSD_ADDR1)
#define REG_AOSD_ADDR2         REG32(AOSD_ADDR2)
#define REG_AOSD_ADDR3         REG32(AOSD_ADDR3)
#define REG_AOSD_WADDR         REG32(AOSD_WADDR)
#define REG_AOSD_ADDRLEN       REG32(AOSD_ADDRLEN)
#define REG_AOSD_ALPHA_VALUE   REG32(AOSD_ALPHA_VALUE)
#define REG_AOSD_CTRL          REG32(AOSD_CTRL)
#define REG_AOSD_INT           REG32(AOSD_INT)

#define AOSD_CTRL_FRMLV_MASK        (0x3 << 18)
#define AOSD_CTRL_FRMLV_2        (0x1 << 18)
#define AOSD_CTRL_FRMLV_3        (0x2 << 18)
#define AOSD_CTRL_FRMLV_4        (0x3 << 18)

#define AOSD_CTRL_FRM_END        (1 << 17)
#define AOSD_CTRL_ALPHA_START    (1 << 16)
#define AOSD_CTRL_INT_MAKS            (1 << 15)
#define AOSD_CTRL_CHANNEL_LEVEL_BIT    7
#define AOSD_CTRL_CHANNEL_LEVEL_MASK   (0xff <<  AOSD_CTRL_CHANNEL_LEVEL_BIT)
#define AOSD_CTRL_ALPHA_MODE_BIT       3
#define AOSD_CTRL_ALPHA_MODE_MASK      (0xf << AOSD_CTRL_ALPHA_MODE_BIT)
#define AOSD_CTRL_ALPHA_PIXEL_MODE     0
#define AOSD_CTRL_ALPHA_FRAME_MODE     1

#define AOSD_CTRL_FORMAT_MODE_BIT     1
#define AOSD_CTRL_FORMAT_MODE_MASK     (0x3 << 1)
#define AOSD_CTRL_RGB565_FORMAT_MODE   (0 << AOSD_CTRL_FORMAT_MODE_BIT)
#define AOSD_CTRL_RGB555_FORMAT_MODE   (1 << AOSD_CTRL_FORMAT_MODE_BIT)
#define AOSD_CTRL_RGB8888_FORMAT_MODE  (2 << AOSD_CTRL_FORMAT_MODE_BIT)

#define AOSD_ALPHA_ENABLE        (1 << 0)

#define AOSD_INT_COMPRESS_END   (1 << 1)
#define AOSD_INT_AOSD_END        (1 << 0)

#define __osd_enable_alpha() 	(REG_AOSD_CTRL |= AOSD_ALPHA_ENABLE)
#define __osd_alpha_start()     (REG_AOSD_CTRL |= AOSD_CTRL_ALPHA_START) 

/*************************************************************************
 * COMPRESS
 *************************************************************************/
#define COMPRESS_SCR_ADDR      (AOSD_BASE + 0x00)
#define COMPRESS_DES_ADDR      (AOSD_BASE + 0x10)
#define COMPRESS_DST_OFFSET       (AOSD_BASE + 0x34)
#define COMPRESS_FRAME_SIZE       (AOSD_BASE + 0x38)
#define COMPRESS_CTRL       (AOSD_BASE + 0x3C)
#define COMPRESS_RATIO      (AOSD_BASE + 0x40)
#define COMPRESS_SRC_OFFSET     (AOSD_BASE + 0x44)
 
#define REG_COMPRESS_SCR_ADDR      REG32(COMPRESS_SCR_ADDR)
#define REG_COMPRESS_DES_ADDR      REG32(COMPRESS_DES_ADDR)
#define REG_COMPRESS_DST_OFFSET       REG32(COMPRESS_DST_OFFSET)
#define REG_COMPRESS_FRAME_SIZE       REG32(COMPRESS_FRAME_SIZE)
#define REG_COMPRESS_CTRL       REG32(COMPRESS_CTRL)
#define REG_COMPRESS_RATIO      REG32(COMPRESS_RATIO)
#define REG_COMPRESS_SRC_OFFSET     REG32(COMPRESS_SRC_OFFSET)

#define COMPRESS_CTRL_WITHOUT_ALPHA     (1 << 4)
#define COMPRESS_CTRL_WITH_ALPHA     (0 << 4)
#define COMPRESS_CTRL_COMP_START   (1 << 3)
#define COMPRESS_CTRL_COMP_END     (1 << 2)
#define COMPRESS_CTRL_INT_MASK     (1 << 1)
#define COMPRESS_CTRL_COMP_ENABLE  (1 << 0)

#define COMPRESS_RATIO_FRM_BYPASS   (1 << 31)
#define COMPRESS_BYPASS_ROW         (1 << 12)
#define COMPRESS_ROW_QUARTER        (1 << 0)

#define COMPRESS_CTRL_ALIGNED_MODE_BIT	(31)
#define COMPRESS_CTRL_ALIGNED_16_WORD	(0 << COMPRESS_CTRL_ALIGNED_MODE_BIT)
#define COMPRESS_CTRL_ALIGNED_64_WORD	(1 << COMPRESS_CTRL_ALIGNED_MODE_BIT)

#define __compress_enable()       (REG_COMPRESS_CTRL |= COMPRESS_INT_AOSD_END)      
#define __compress_start()        (REG_COMPRESS_CTRL |= COMPRESS_CTRL_COMP_START)      
#define __compress_with_alpha()   (REG_COMPRESS_CTRL |= COMPRESS_CTRL_ALPHA_EN)      

/* Rockbox defines */

/* Timer frequency */
#define TIMER_FREQ (CFG_EXTAL) /* For full precision! */

/* Serial */
#define CFG_UART_BASE UART1_BASE /* Base of the UART channel */
#define CFG_BAUDRATE  57600

#endif /* __JZ4760B_H__ */
