/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: cpu.h,v 1.6 1997/11/22 14:29:54 ahornby Exp $
******************************************************************************/
/*
 *
 * This file was part of x64.
 *
 * This file contains useful stuff when you are creating
 * virtual machine like MCS6510 based microcomputer.
 *
 * Included are:
 *	o registers
 *	o flags in PSW
 *      o addressing modes
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jarkko Sonninen (sonninen@lut.fi)
 *   Jouko Valta (jopi@stekt.oulu.fi)
 *
 *
 */

#ifndef X2600_CPU_H
#define X2600_CPU_H


#include "rbconfig.h"
#include "types.h"

/* 6507 Registers. */
#define AC		accumulator
#define XR		x_register
#define YR		y_register
#define SP		stack_pointer
#define PC		program_counter
#define PCH		((PC>>8)&0xff)
#define PCL		(PC&0xff)

#define ZF		zero_flag
#define SF		sign_flag
#define OF		overflow_flag
#define BF		break_flag
#define DF		decimal_flag
#define IF		interrupt_flag
#define CF		carry_flag


/* Masks which indicate location of status flags in PSW. */
#define S_SIGN		0x80
#define S_OVERFLOW	0x40
#define S_NOTUSED 	0x20
#define S_BREAK		0x10
#define S_DECIMAL	0x08
#define S_INTERRUPT	0x04
#define S_ZERO		0x02
#define S_CARRY		0x01


/* ADDRESSING MODES */

#define IMPLIED		0
#define ACCUMULATOR	1
#define IMMEDIATE	2

#define ZERO_PAGE	3
#define ZERO_PAGE_X	4
#define ZERO_PAGE_Y	5

#define ABSOLUTE	6
#define ABSOLUTE_X	7
#define ABSOLUTE_Y	8

#define ABS_INDIRECT	9
#define INDIRECT_X	10
#define INDIRECT_Y	11

#define RELATIVE	12

#define ASS_CODE	13


/*
 * Declaration for lookup-table which is used to translate MOS6502
 * machine instructions. Machine code is used as index to array called
 * lookup. Pointer to function is then fetched from array and function
 * is called.
 */

extern struct lookup_tag {
    char          *mnemonic;	/* Selfdocumenting? */
    short          addr_mode;
    unsigned char  source;
    unsigned char  destination;
    unsigned char  cycles;
    unsigned char  pbc_fix;	/* Cycle for Page Boundary Crossing */
}       lookup[];


/* Addressing mode (addr_mode) is used when instruction is diassembled
 * or assembled by diassembler or assembler. This is used i.e.
 * in function char *sprint_opcode() in the file misc.c.
 *
 * MOS6502 addressing modes are #defined in the file "vmachine.h".
 *
 * Mnemonic is character string telling the name of the instruction.
 */

#define M_NONE	0
#define M_AC 	1
#define M_XR	2
#define M_YR	3
#define M_SP	4
#define M_SR	5
#define M_PC	6
#define M_IMM	7
#define M_ZERO	8
#define M_ZERX	9
#define M_ZERY	10
#define M_ABS	11
#define M_ABSX	12
#define M_ABSY	13
#define M_AIND	14
#define M_INDX	15
#define M_INDY	16
#define M_REL	17
#define M_FC	18
#define M_FD	19
#define M_FI	20
#define M_FV	21
#define M_ADDR	22
#define M_	23

#ifndef NO_UNDOC_CMDS
#define M_ACIM	24	/* Source: AC & IMMED (bus collision) */
#define M_ANXR	25	/* Source: AC & XR (bus collision) */
#define M_AXIM	26	/* Source: (AC | #EE) & XR & IMMED (bus collision) */
#define M_ACNC	27	/* Dest: M_AC and Carry = Negative */
#define M_ACXR	28	/* Dest: M_AC, M_XR */

#define M_SABY	29	/* Source: (ABS_Y & SP) (bus collision) */
#define M_ACXS	30	/* Dest: M_AC, M_XR, M_SP */
#define M_STH0	31	/* Dest: Store (src & Addr_Hi+1) to (Addr +0x100) */
#define M_STH1	32
#define M_STH2	33
#define M_STH3	34

#else
#define M_ACIM	M_NONE
#define M_ANXR	M_NONE
#define M_AXIM	M_NONE
#define M_ACNC	M_NONE
#define M_ACXR	M_NONE

#define M_SABY	M_NONE
#define M_ACXS	M_NONE
#define M_STH0	M_NONE
#define M_STH1	M_NONE
#define M_STH2	M_NONE
#define M_STH3	M_NONE
#endif



#endif  /* X2600_CPU_H */








