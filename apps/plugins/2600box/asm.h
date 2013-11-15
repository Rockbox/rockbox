/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: asm.h,v 1.2 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/*
 * This file is was part of x64.
 *
 * Error codes and messages used in asm.c and
 * control codes for both asm and mon.
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *
 *
 */

#ifndef X64_ASM_H
#define X64_ASM_H


#define NUM_OF_MNEMS	56
#define TOTAL_CODES	256
#define MAXARG		19	/* command + addr + 16 bytes + 1 */

 /* optimise searching a little */
#define OP_MNEM_SPC	0x04

#define OP_IMPL_MIN	0x00
#define OP_IMPL_MAX	0xfa
#define OP_IMPL_SPC	0x02

#define OP_IMM_MIN	0x09
#define OP_IMM_MAX	0xeb
#define OP_IMM_SPC	0x01	/* not used */

#define OP_ACCU_MIN	0x0a
#define OP_ACCU_MAX	0x6a
#define OP_ACCU_SPC	0x20

#define OP_ABS_MIN	0x0c
#define OP_ABS_MAX	0xff
#define OP_ABS_SPC	0x04


 /* Symbol definitions */

#define SYMBOL_BYTE	1
#define SYMBOL_WORD	2
#define SYMBOL_FOUND	16
#define SYMBOL_SET	32
#define SYMBOL_VALID	64

#define SYMBOL_MAX_CHARS 8
	/* For portability, labels should be 6 characters or less. */


 /*
  * Define 'mode'
  */

#define MODE_HEX	 1
#define MODE_SYMBOL	 2
#define MODE_QUOTE	 4
#define MODE_MON	(1 << 5)
#define MODE_ASM	(1 << 6)
#define MODE_INF	(1 << 7)
#define MODE_OUTF	(1 << 8)
#define MODE_QUIET	(1 << 9)
#define MODE_VERBOSE	(1 << 10)
#define MODE_QUERY	(1 << 12)
#define MODE_SPACE	(1 << 13)	/* space terminates calculation (MON) */


 /*
  * Error messages
  */

#define ERRORS_TO_STOP	20	/* screenfull on terminal */

#define E_OK			0

 /* Warnings */

#define E_UNDOCUMENTED		(-1)
#define E_SIZE			(-2)
#define E_LARGE_VALUE		(-3)
#define E_LONG_NAME		(-4)
#define E_FORWARD_REF		(-5)

 /* Errors */
#define E_ERROR			(-64)	/* General error */

/* Line Syntax */
#define E_SYNTAX		(E_ERROR)
#define E_PARSE_ERROR		(E_ERROR -1)
#define E_TOO_MANY_ERRORS	(E_ERROR -2)

/* Assembler */
#define E_BAD_IDENTIFIER	(E_ERROR -8)
#define E_BAD_DIRECTIVE		(E_ERROR -9)
#define E_SYMBOL_UNDEFINED	(E_ERROR -10)
#define E_SYMBOL_REDEF		(E_ERROR -11)
#define E_PC_DECREMENT		(E_ERROR -12)

/* Mnemonic */
#define E_BAD_MNEM		(E_ERROR -16)
#define E_LONG_BRANCH		(E_ERROR -17)
#define E_MISSING_OPER		(E_ERROR -18)

/* Operand Syntax */
#define E_PARAMETER_SYNTAX	(E_ERROR -24)
#define E_TOO_MANY_COMMAS	(E_ERROR -25)
#define E_RIGHT_PARENTHESIS	(E_ERROR -26)
#define E_LEFT_PARENTHESIS	(E_ERROR -27)
#define E_PARENTHESIS		(E_ERROR -28)

#define E_MIXED_XY		(E_ERROR -30)
#define E_MISSING_XY		(E_ERROR -31)
#define E_BAD_INDEX		(E_ERROR -32)



 /* Warnings */
#define EM_UNDOCUMENTED		"Undocumented opcode used"
#define EM_SIZE			"Operand length changed"
#define EM_LARGE_VALUE		"Value too large"
#define EM_LONG_NAME		"Symbol name too long"
#define EM_FORWARD_REF		"Forward reference"

 /* Errors */
#define EM_SYNTAX		"Syntax error"
#define EM_PARSE_ERROR		"Parse error: I don't get it."
#define EM_TOO_MANY_ERRORS	"Keep your filthy fingers off here!"

#define EM_BAD_IDENTIFIER	"Identifier error"
#define EM_BAD_DIRECTIVE	"Unrecognised directive"
#define EM_SYMBOL_UNDEFINED	"Undefined symbol"
#define EM_SYMBOL_REDEF 	"Symbol redefined"
#define EM_PC_DECREMENT		"PC decrement"

#define EM_BAD_MNEM		"Illegal mnemonic"
#define EM_LONG_BRANCH		"Branch out of range"
#define EM_MISSING_OPER		"Operand missing"

#define EM_PARAMETER_SYNTAX 	"Parameter syntax error"
#define EM_TOO_MANY_COMMAS  	"Too many commas found"
#define EM_RIGHT_PARENTHESIS	"Too many right parenthesis, 1 is maximum"
#define EM_LEFT_PARENTHESIS 	"Too many left parenthesis, 1 is maximum"
#define EM_PARENTHESIS	    	"Not equally right and left parenthesis"

#define EM_MIXED_XY         	"Indirect mode indexed both X and Y"
#define EM_MISSING_XY		"Index register missing"
#define EM_BAD_INDEX		"Index register must be X or Y"


#endif  /* X64_ASM_H */
