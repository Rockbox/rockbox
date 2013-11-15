




/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for Details.
   
   $Id: table.c,v 1.4 1996/08/26 15:04:20 ahornby Exp $
******************************************************************************/

/*
 * $Id: table.c,v 1.4 1996/08/26 15:04:20 ahornby Exp $
 *
 * This was part of the x64 Commodore 64 emulator.
 * See README for copyright notice
 *
 * This file contains lookup-table which is used to translate
 * MOS6502 machine instructions. Machine code is used as index
 * to array called lookup. Pointer to function is then fetched
 * from array and function is called.
 * Timing of the undocumented opcodes is based on information
 * in an article in C=Lehti by Kai Lindfors and Topi Maurola.
 *
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jarkko Sonninen (sonninen@lut.fi)
 *   Jouko Valta (jopi@stekt.oulu.fi)
 *
 */

#include "cpu.h"
#include "mnemonic.h"


/*
 * The "mnemonic.h" file contains #defines for BRK, ORA, NOOP... which
 * are character strings, i.e. #define BRK    "BRK"
 * #define ORA    "ORA" . . . Used mainly to reduce typing...
 *
 * There are #defines for addressing modes i.e. IMPLIED, INDIRECT_X,
 * ZERO_PAGE in "cpu.h"... These can be used to make a diassembler.
 */


int clength[] =
{1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 0};

struct lookup_tag lookup[] =
{

/****  Positive  ****/

    /* 00 */
  {BRK, IMPLIED, M_NONE, M_PC, 7, 0},	/* Pseudo Absolute */
    /* 01 */
  {ORA, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (Indirect,X) */
    /* 02 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 03 */
  {SLO, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* 04 */
  {NOOP, ZERO_PAGE, M_NONE, M_NONE, 3, 0},
    /* 05 */
  {ORA, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* 06 */
  {ASL, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* 07 */
  {SLO, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* 08 */
  {PHP, IMPLIED, M_SR, M_NONE, 3, 0},
    /* 09 */
  {ORA, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* 0a */
  {ASL, ACCUMULATOR, M_AC, M_AC, 2, 0},		/* Accumulator */
    /* 0b */
  {ANC, IMMEDIATE, M_ACIM, M_ACNC, 2, 0},

    /* 0c */
  {NOOP, ABSOLUTE, M_NONE, M_NONE, 4, 0},
    /* 0d */
  {ORA, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* 0e */
  {ASL, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* 0f */
  {SLO, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* 10 */
  {BPL, RELATIVE, M_REL, M_NONE, 2, 0},
    /* 11 */
  {ORA, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (Indirect),Y */
    /* 12 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 13 */
  {SLO, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* 14 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* 15 */
  {ORA, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* 16 */
  {ASL, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* 17 */
  {SLO, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* 18 */
  {CLC, IMPLIED, M_NONE, M_FC, 2, 0},
    /* 19 */
  {ORA, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* 1a */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* 1b */
  {SLO, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* 1c */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* 1d */
  {ORA, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* 1e */
  {ASL, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* 1f */
  {SLO, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},

    /* 20 */
  {JSR, ABSOLUTE, M_ADDR, M_PC, 6, 0},
    /* 21 */
  {AND, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (Indirect ,X) */
    /* 22 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 23 */
  {RLA, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* 24 */
  {BIT, ZERO_PAGE, M_ZERO, M_NONE, 3, 0},	/* Zeropage */
    /* 25 */
  {AND, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* 26 */
  {ROL, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* 27 */
  {RLA, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* 28 */
  {PLP, IMPLIED, M_NONE, M_SR, 4, 0},
    /* 29 */
  {AND, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* 2a */
  {ROL, ACCUMULATOR, M_AC, M_AC, 2, 0},		/* Accumulator */
    /* 2b */
  {ANC, IMMEDIATE, M_ACIM, M_ACNC, 2, 0},

    /* 2c */
  {BIT, ABSOLUTE, M_ABS, M_NONE, 4, 0},		/* Absolute */
    /* 2d */
  {AND, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* 2e */
  {ROL, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* 2f */
  {RLA, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* 30 */
  {BMI, RELATIVE, M_REL, M_NONE, 2, 0},
    /* 31 */
  {AND, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (Indirect),Y */
    /* 32 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 33 */
  {RLA, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* 34 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* 35 */
  {AND, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* 36 */
  {ROL, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* 37 */
  {RLA, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* 38 */
  {SEC, IMPLIED, M_NONE, M_FC, 2, 0},
    /* 39 */
  {AND, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* 3a */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* 3b */
  {RLA, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* 3c */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* 3d */
  {AND, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* 3e */
  {ROL, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* 3f */
  {RLA, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},

    /* 40 */
  {RTI, IMPLIED, M_NONE, M_PC, 6, 0},
    /* 41 */
  {EOR, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (Indirect,X) */
    /* 42 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 43 */
  {SRE, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* 44 */
  {NOOP, ZERO_PAGE, M_NONE, M_NONE, 3, 0},
    /* 45 */
  {EOR, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* 46 */
  {LSR, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* 47 */
  {SRE, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* 48 */
  {PHA, IMPLIED, M_AC, M_NONE, 3, 0},
    /* 49 */
  {EOR, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* 4a */
  {LSR, ACCUMULATOR, M_AC, M_AC, 2, 0},		/* Accumulator */
    /* 4b */
  {ASR, IMMEDIATE, M_ACIM, M_AC, 2, 0},		/* (AC & IMM) >>1 */

    /* 4c */
  {JMP, ABSOLUTE, M_ADDR, M_PC, 3, 0},	/* Absolute */
    /* 4d */
  {EOR, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* 4e */
  {LSR, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* 4f */
  {SRE, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* 50 */
  {BVC, RELATIVE, M_REL, M_NONE, 2, 0},
    /* 51 */
  {EOR, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (Indirect),Y */
    /* 52 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 53 */
  {SRE, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* 54 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* 55 */
  {EOR, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* 56 */
  {LSR, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* 57 */
  {SRE, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* 58 */
  {CLI, IMPLIED, M_NONE, M_FI, 2, 0},
    /* 59 */
  {EOR, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* 5a */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* 5b */
  {SRE, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* 5c */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* 5d */
  {EOR, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* 5e */
  {LSR, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* 5f */
  {SRE, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},

    /* 60 */
  {RTS, IMPLIED, M_NONE, M_PC, 6, 0},
    /* 61 */
  {ADC, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (Indirect,X) */
    /* 62 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* 63 */
  {RRA, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* 64 */
  {NOOP, ZERO_PAGE, M_NONE, M_NONE, 3, 0},
    /* 65 */
  {ADC, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* 66 */
  {ROR, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* 67 */
  {RRA, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* 68 */
  {PLA, IMPLIED, M_NONE, M_AC, 4, 0},
    /* 69 */
  {ADC, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* 6a */
  {ROR, ACCUMULATOR, M_AC, M_AC, 2, 0},		/* Accumulator */
    /* 6b */
  {ARR, IMMEDIATE, M_ACIM, M_AC, 2, 0},		/* ARR isn't typo */

    /* 6c */
  {JMP, ABS_INDIRECT, M_AIND, M_PC, 5, 0},	/* Indirect */
    /* 6d */
  {ADC, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* 6e */
  {ROR, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* 6f */
  {RRA, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* 70 */
  {BVS, RELATIVE, M_REL, M_NONE, 2, 0},
    /* 71 */
  {ADC, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (Indirect),Y */
    /* 72 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT relative? */
    /* 73 */
  {RRA, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* 74 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* 75 */
  {ADC, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* 76 */
  {ROR, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* 77 */
  {RRA, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* 78 */
  {SEI, IMPLIED, M_NONE, M_FI, 2, 0},
    /* 79 */
  {ADC, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* 7a */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* 7b */
  {RRA, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* 7c */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* 7d */
  {ADC, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* 7e */
  {ROR, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* 7f */
  {RRA, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},

/****  Negative  ****/

    /* 80 */
  {NOOP, IMMEDIATE, M_NONE, M_NONE, 2, 0},
    /* 81 */
  {STA, INDIRECT_X, M_AC, M_INDX, 6, 0},	/* (Indirect,X) */
    /* 82 */
  {NOOP, IMMEDIATE, M_NONE, M_NONE, 2, 0},
    /* 83 */
  {SAX, INDIRECT_X, M_ANXR, M_INDX, 6, 0},

    /* 84 */
  {STY, ZERO_PAGE, M_YR, M_ZERO, 3, 0},		/* Zeropage */
    /* 85 */
  {STA, ZERO_PAGE, M_AC, M_ZERO, 3, 0},		/* Zeropage */
    /* 86 */
  {STX, ZERO_PAGE, M_XR, M_ZERO, 3, 0},		/* Zeropage */
    /* 87 */
  {SAX, ZERO_PAGE, M_ANXR, M_ZERO, 3, 0},

    /* 88 */
  {DEY, IMPLIED, M_YR, M_YR, 2, 0},
    /* 89 */
  {NOOP, IMMEDIATE, M_NONE, M_NONE, 2, 0},
    /* 8a */
  {TXA, IMPLIED, M_XR, M_AC, 2, 0},
/****  very abnormal: usually AC = AC | #$EE & XR & #$oper  ****/
    /* 8b */
  {ANE, IMMEDIATE, M_AXIM, M_AC, 2, 0},

    /* 8c */
  {STY, ABSOLUTE, M_YR, M_ABS, 4, 0},	/* Absolute */
    /* 8d */
  {STA, ABSOLUTE, M_AC, M_ABS, 4, 0},	/* Absolute */
    /* 8e */
  {STX, ABSOLUTE, M_XR, M_ABS, 4, 0},	/* Absolute */
    /* 8f */
  {SAX, ABSOLUTE, M_ANXR, M_ABS, 4, 0},

    /* 90 */
  {BCC, RELATIVE, M_REL, M_NONE, 2, 0},
    /* 91 */
  {STA, INDIRECT_Y, M_AC, M_INDY, 6, 0},	/* (Indirect),Y */
    /* 92 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT relative? */
    /* 93 */
  {SHA, INDIRECT_Y, M_ANXR, M_STH0, 6, 0},

    /* 94 */
  {STY, ZERO_PAGE_X, M_YR, M_ZERX, 4, 0},	/* Zeropage,X */
    /* 95 */
  {STA, ZERO_PAGE_X, M_AC, M_ZERX, 4, 0},	/* Zeropage,X */
    /* 96 */
  {STX, ZERO_PAGE_Y, M_XR, M_ZERY, 4, 0},	/* Zeropage,Y */
    /* 97 */
  {SAX, ZERO_PAGE_Y, M_ANXR, M_ZERY, 4, 0},

    /* 98 */
  {TYA, IMPLIED, M_YR, M_AC, 2, 0},
    /* 99 */
  {STA, ABSOLUTE_Y, M_AC, M_ABSY, 5, 0},	/* Absolute,Y */
    /* 9a */
  {TXS, IMPLIED, M_XR, M_SP, 2, 0},
/*** This is very mysterious command ... */
    /* 9b */
  {SHS, ABSOLUTE_Y, M_ANXR, M_STH3, 5, 0},

    /* 9c */
  {SHY, ABSOLUTE_X, M_YR, M_STH2, 5, 0},
    /* 9d */
  {STA, ABSOLUTE_X, M_AC, M_ABSX, 5, 0},	/* Absolute,X */
    /* 9e */
  {SHX, ABSOLUTE_Y, M_XR, M_STH1, 5, 0},
    /* 9f */
  {SHA, ABSOLUTE_Y, M_ANXR, M_STH1, 5, 0},

    /* a0 */
  {LDY, IMMEDIATE, M_IMM, M_YR, 2, 0},	/* Immediate */
    /* a1 */
  {LDA, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (indirect,X) */
    /* a2 */
  {LDX, IMMEDIATE, M_IMM, M_XR, 2, 0},	/* Immediate */
    /* a3 */
  {LAX, INDIRECT_X, M_INDX, M_ACXR, 6, 0},	/* (indirect,X) */

    /* a4 */
  {LDY, ZERO_PAGE, M_ZERO, M_YR, 3, 0},		/* Zeropage */
    /* a5 */
  {LDA, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* a6 */
  {LDX, ZERO_PAGE, M_ZERO, M_XR, 3, 0},		/* Zeropage */
    /* a7 */
  {LAX, ZERO_PAGE, M_ZERO, M_ACXR, 3, 0},

    /* a8 */
  {TAY, IMPLIED, M_AC, M_YR, 2, 0},
    /* a9 */
  {LDA, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* aa */
  {TAX, IMPLIED, M_AC, M_XR, 2, 0},
    /* ab */
  {LXA, IMMEDIATE, M_ACIM, M_ACXR, 2, 0},	/* LXA isn't a typo */

    /* ac */
  {LDY, ABSOLUTE, M_ABS, M_YR, 4, 0},	/* Absolute */
    /* ad */
  {LDA, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* ae */
  {LDX, ABSOLUTE, M_ABS, M_XR, 4, 0},	/* Absolute */
    /* af */
  {LAX, ABSOLUTE, M_ABS, M_ACXR, 4, 0},

    /* b0 */
  {BCS, RELATIVE, M_REL, M_NONE, 2, 0},
    /* b1 */
  {LDA, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (indirect),Y */
    /* b2 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* b3 */
  {LAX, INDIRECT_Y, M_INDY, M_ACXR, 5, 1},

    /* b4 */
  {LDY, ZERO_PAGE_X, M_ZERX, M_YR, 4, 0},	/* Zeropage,X */
    /* b5 */
  {LDA, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* b6 */
  {LDX, ZERO_PAGE_Y, M_ZERY, M_XR, 4, 0},	/* Zeropage,Y */
    /* b7 */
  {LAX, ZERO_PAGE_Y, M_ZERY, M_ACXR, 4, 0},

    /* b8 */
  {CLV, IMPLIED, M_NONE, M_FV, 2, 0},
    /* b9 */
  {LDA, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* ba */
  {TSX, IMPLIED, M_SP, M_XR, 2, 0},
    /* bb */
  {LAS, ABSOLUTE_Y, M_SABY, M_ACXS, 4, 1},

    /* bc */
  {LDY, ABSOLUTE_X, M_ABSX, M_YR, 4, 1},	/* Absolute,X */
    /* bd */
  {LDA, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* be */
  {LDX, ABSOLUTE_Y, M_ABSY, M_XR, 4, 1},	/* Absolute,Y */
    /* bf */
  {LAX, ABSOLUTE_Y, M_ABSY, M_ACXR, 4, 1},

    /* c0 */
  {CPY, IMMEDIATE, M_IMM, M_NONE, 2, 0},	/* Immediate */
    /* c1 */
  {CMP, INDIRECT_X, M_INDX, M_NONE, 6, 0},	/* (Indirect,X) */
    /* c2 */
  {NOOP, IMMEDIATE, M_NONE, M_NONE, 2, 0},	/* occasional TILT */
    /* c3 */
  {DCP, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* c4 */
  {CPY, ZERO_PAGE, M_ZERO, M_NONE, 3, 0},	/* Zeropage */
    /* c5 */
  {CMP, ZERO_PAGE, M_ZERO, M_NONE, 3, 0},	/* Zeropage */
    /* c6 */
  {DEC, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* c7 */
  {DCP, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* c8 */
  {INY, IMPLIED, M_YR, M_YR, 2, 0},
    /* c9 */
  {CMP, IMMEDIATE, M_IMM, M_NONE, 2, 0},	/* Immediate */
    /* ca */
  {DEX, IMPLIED, M_XR, M_XR, 2, 0},
    /* cb */
  {SBX, IMMEDIATE, M_IMM, M_XR, 2, 0},

    /* cc */
  {CPY, ABSOLUTE, M_ABS, M_NONE, 4, 0},		/* Absolute */
    /* cd */
  {CMP, ABSOLUTE, M_ABS, M_NONE, 4, 0},		/* Absolute */
    /* ce */
  {DEC, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* cf */
  {DCP, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* d0 */
  {BNE, RELATIVE, M_REL, M_NONE, 2, 0},
    /* d1 */
  {CMP, INDIRECT_Y, M_INDY, M_NONE, 5, 1},	/* (Indirect),Y */
    /* d2 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* d3 */
  {DCP, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* d4 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* d5 */
  {CMP, ZERO_PAGE_X, M_ZERX, M_NONE, 4, 0},	/* Zeropage,X */
    /* d6 */
  {DEC, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* d7 */
  {DCP, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* d8 */
  {CLD, IMPLIED, M_NONE, M_FD, 2, 0},
    /* d9 */
  {CMP, ABSOLUTE_Y, M_ABSY, M_NONE, 4, 1},	/* Absolute,Y */
    /* da */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* db */
  {DCP, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* dc */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* dd */
  {CMP, ABSOLUTE_X, M_ABSX, M_NONE, 4, 1},	/* Absolute,X */
    /* de */
  {DEC, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* df */
  {DCP, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},

    /* e0 */
  {CPX, IMMEDIATE, M_IMM, M_NONE, 2, 0},	/* Immediate */
    /* e1 */
  {SBC, INDIRECT_X, M_INDX, M_AC, 6, 0},	/* (Indirect,X) */
    /* e2 */
  {NOOP, IMMEDIATE, M_NONE, M_NONE, 2, 0},
    /* e3 */
  {ISB, INDIRECT_X, M_INDX, M_INDX, 8, 0},

    /* e4 */
  {CPX, ZERO_PAGE, M_ZERO, M_NONE, 3, 0},	/* Zeropage */
    /* e5 */
  {SBC, ZERO_PAGE, M_ZERO, M_AC, 3, 0},		/* Zeropage */
    /* e6 */
  {INC, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},	/* Zeropage */
    /* e7 */
  {ISB, ZERO_PAGE, M_ZERO, M_ZERO, 5, 0},

    /* e8 */
  {INX, IMPLIED, M_XR, M_XR, 2, 0},
    /* e9 */
  {SBC, IMMEDIATE, M_IMM, M_AC, 2, 0},	/* Immediate */
    /* ea */
  {NOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* eb */
  {USBC, IMMEDIATE, M_IMM, M_AC, 2, 0},		/* same as e9 */

    /* ec */
  {CPX, ABSOLUTE, M_ABS, M_NONE, 4, 0},		/* Absolute */
    /* ed */
  {SBC, ABSOLUTE, M_ABS, M_AC, 4, 0},	/* Absolute */
    /* ee */
  {INC, ABSOLUTE, M_ABS, M_ABS, 6, 0},	/* Absolute */
    /* ef */
  {ISB, ABSOLUTE, M_ABS, M_ABS, 6, 0},

    /* f0 */
  {BEQ, RELATIVE, M_REL, M_NONE, 2, 0},
    /* f1 */
  {SBC, INDIRECT_Y, M_INDY, M_AC, 5, 1},	/* (Indirect),Y */
    /* f2 */
  {JAM, IMPLIED, M_NONE, M_NONE, 0, 0},		/* TILT */
    /* f3 */
  {ISB, INDIRECT_Y, M_INDY, M_INDY, 8, 0},

    /* f4 */
  {NOOP, ZERO_PAGE_X, M_NONE, M_NONE, 4, 0},
    /* f5 */
  {SBC, ZERO_PAGE_X, M_ZERX, M_AC, 4, 0},	/* Zeropage,X */
    /* f6 */
  {INC, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},	/* Zeropage,X */
    /* f7 */
  {ISB, ZERO_PAGE_X, M_ZERX, M_ZERX, 6, 0},

    /* f8 */
  {SED, IMPLIED, M_NONE, M_FD, 2, 0},
    /* f9 */
  {SBC, ABSOLUTE_Y, M_ABSY, M_AC, 4, 1},	/* Absolute,Y */
    /* fa */
  {NOOP, IMPLIED, M_NONE, M_NONE, 2, 0},
    /* fb */
  {ISB, ABSOLUTE_Y, M_ABSY, M_ABSY, 7, 0},

    /* fc */
  {NOOP, ABSOLUTE_X, M_NONE, M_NONE, 4, 1},
    /* fd */
  {SBC, ABSOLUTE_X, M_ABSX, M_AC, 4, 1},	/* Absolute,X */
    /* fe */
  {INC, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0},	/* Absolute,X */
    /* ff */
  {ISB, ABSOLUTE_X, M_ABSX, M_ABSX, 7, 0}
};
