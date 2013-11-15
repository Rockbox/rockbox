/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: address.h,v 1.4 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

#ifndef ADDRESS_H 
#define ADDRESS_H 

/* Contains the addresses of the 2600 hardware */
/* $Id: address.h,v 1.4 1996/04/01 14:51:50 alex Exp $ */

/* TIA Write Addresses (6 bit) */

#define VSYNC	0x00
#define VBLANK	0x01
#define WSYNC	0x02
#define RSYNC	0x03
#define NUSIZ0	0x04
#define NUSIZ1	0x05
#define COLUP0	0x06
#define COLUP1	0x07
#define COLUPF	0x08
#define COLUBK	0x09
#define CTRLPF	0x0A
#define REFP0	0x0B
#define REFP1	0x0C
#define PF0	0x0D
#define PF1	0x0E
#define PF2	0x0F
#define RESP0	0x10
#define RESP1	0x11
#define RESM0	0x12
#define RESM1	0x13
#define RESBL	0x14
#define AUDC0	0x15
#define AUDC1	0x16
#define AUDF0	0x17
#define AUDF1	0x18
#define AUDV0	0x19
#define AUDV1	0x1A
#define GRP0	0x1B
#define GRP1	0x1C
#define ENAM0	0x1D
#define ENAM1	0x1E
#define ENABL	0x1F
#define HMP0	0x20
#define HMP1	0x21
#define HMM0	0x22
#define HMM1	0x23
#define HMBL	0x24
#define VDELP0	0x25
#define VDELP1	0x26
#define VDELBL	0x27
#define RESMP0	0x28
#define RESMP1	0x29
#define HMOVE	0x2A
#define HMCLR	0x2B
#define CXCLR	0x2C

/* TIA Read Addresses */
#define CXM0P	0x0
#define CXM1P	0x1
#define CXP0FB	0x2
#define CXP1FB	0x3
#define CXM0FB	0x4
#define CXM1FB	0x5
#define CXBLPF	0x6
#define CXPPMM	0x7
#define INPT0	0x8
#define INPT1	0x9
#define INPT2	0xA
#define INPT3	0xB
#define INPT4	0xC
#define INPT5	0xD

/* RIOT Addresses */

#define RAM	0x80	/* till 0xff */
#define SWCHA	0x280
#define SWACNT	0x281
#define SWCHB	0x282
#define SWBCNT	0x283
#define INTIM	0x284

#define TIM1T	0x294
#define TIM8T	0x295
#define TIM64T	0x296
#define T1024T	0x297

#define ROM	0xE000	/* To FFFF,0x1000-1FFF */ 

#endif








