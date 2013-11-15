/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: mnemonic.h,v 1.2 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/*
 *
 * This file was part of x64.
 * See README for copyright notice
 *
 * This file contains #defines for MOS6010 instruction mnemonics.
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jouko Valta (jopi@stekt.oulu.fi)
 */

#ifndef X2600_MNEMONIC_H
#define X2600_MNEMONIC_H


/* INSTRUCTION MNEMONICS. */

#define ADC	"ADC"
#define AND	"AND"
#define ASL	"ASL"
#define BCC	"BCC"
#define BCS	"BCS"
#define BEQ	"BEQ"
#define BIT	"BIT"
#define BMI	"BMI"
#define BNE	"BNE"
#define BPL	"BPL"
#define BRK	"BRK"
#define BVC	"BVC"
#define BVS	"BVS"
#define CLC	"CLC"
#define CLD	"CLD"
#define CLI	"CLI"
#define CLV	"CLV"
#define CMP	"CMP"
#define CPX	"CPX"
#define CPY	"CPY"
#define DEC	"DEC"
#define DEX	"DEX"
#define DEY	"DEY"
#define EOR	"EOR"
#define INC	"INC"
#define INX	"INX"
#define INY	"INY"
#define JMP	"JMP"
#define JSR	"JSR"
#define LDA	"LDA"
#define LDX	"LDX"
#define LDY	"LDY"
#define LSR	"LSR"
#define NOOP	"NOOP"
#define NOP	"NOP"
#define ORA	"ORA"
#define PHA	"PHA"
#define PHP	"PHP"
#define PLA	"PLA"
#define PLP	"PLP"
#define ROL	"ROL"
#define ROR	"ROR"
#define RTI	"RTI"
#define RTS	"RTS"
#define SBC	"SBC"
#define SEC	"SEC"
#define SED	"SED"
#define SEI	"SEI"
#define STA	"STA"
#define STX	"STX"
#define STY	"STY"
#define TAX	"TAX"
#define TAY	"TAY"
#define TSX	"TSX"
#define TXA	"TXA"
#define TXS	"TXS"
#define TYA	"TYA"

#ifndef NO_UNDOC_CMDS
#define ANC	"ANC"
#define ANE	"ANE"
#define ARR	"ARR"
#define ASR	"ASR"
#define DCP	"DCP"
#define ISB	"ISB"
#define JAM	"JAM"
#define LAS	"LAS"
#define LAX	"LAX"
#define LXA	"LXA"
 /* NOOP undefined NOP */
#define RLA	"RLA"
#define RRA	"RRA"
#define SAX	"SAX"
#define USBC	"USBC"	/* undefined SBC */
#define SBX	"SBX"
#define SHA	"SHA"
#define SHS	"SHS"
#define SHX	"SHX"
#define SHY	"SHY"
#define SLO	"SLO"
#define SRE	"SRE"

#else
#define ANC	NOOP
#define ANE	NOOP
#define ARR	NOOP
#define ASR	NOOP
#define DCP	NOOP
#define ISB	NOOP
#define JAM	NOOP
#define LAS	NOOP
#define LAX	NOOP
#define LXA	NOOP
 /* NOOP undefined NOP */
#define RLA	NOOP
#define RRA	NOOP
#define SAX	NOOP
#define USBC	NOOP
#define SBX	NOOP
#define SHA	NOOP
#define SHS	NOOP
#define SHX	NOOP
#define SHY	NOOP
#define SLO	NOOP
#define SRE	NOOP
#endif

#endif  /* X2600_MNEMONIC_H */
