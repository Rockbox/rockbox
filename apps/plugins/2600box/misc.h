/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: misc.h,v 1.2 1996/04/01 14:51:50 alex Exp $
******************************************************************************/

/*
 * This file was part of x64.
 *
 * This file contains misc funtions to help debugging.
 * Included are:
 *	o Show numeric conversions
 *	o Show CPU registers
 *	o Show Stack contents
 *	o Print binary number
 *	o Print instruction from memory
 *	o Decode instruction
 *	o Find effective address for operand
 *	o Create a copy of string
 *	o Move memory
 *
 * sprint_opcode returns mnemonic code of machine instruction.
 * sprint_binary returns binary form of given code (8bit)
 *
 *
 * Written by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jouko Valta (jopi@stekt.oulu.fi)
 *
 */

#ifndef X2600_MISC_H
#define X2600_MISC_H

#include "types.h"

extern void    show_bases ( char *line, int mode );
extern void    show ( void );
extern void    print_stack ( BYTE sp );
extern char   *sprint_binary ( BYTE code );
extern char   *sprint_ophex ( ADDRESS p);
extern char   *sprint_opcode ( ADDRESS counter, int base );
extern char   *sprint_disassembled ( ADDRESS counter, BYTE x, BYTE p1, BYTE p2, int base );
extern char   *xstrdup ( char *str );
extern char   *strndupp ( char *str, int n );
extern void    memmov ( char *target, char *source, unsigned int length );
extern int   eff_address(ADDRESS counter, int step);

#endif  /* X2600_MISC_H */



