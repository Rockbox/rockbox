/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================
   
   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.
   
   See the file COPYING for details.
   
   $Id: macro.h,v 1.7 1996/11/24 16:55:40 ahornby Exp $
******************************************************************************/

/*
 *
 * Originally from x64 by
 *   Vesa-Matti Puro (vmp@lut.fi)
 *   Jarkko Sonninen (sonninen@lut.fi)
 * 
 * NOTE: Can add zero page optimizations
 */

#ifndef X2600_MACRO_H
#define X2600_MACRO_H


#include "types.h"
#include "cpu.h"

#define LOAD(a)      		decRead(a)
#define LOADEXEC(a)      	undecRead(a)
#define DLOAD(a)                dbgRead(a)
#define LOAD_ZERO(a) 		decRead((ADDRESS)a)
#define LOAD_ADDR(a)		((LOAD(a+1)<<8)+LOAD(a))
#define LOAD_ZERO_ADDR(a)	LOAD_ADDR(a)

#define STORE(a,b)     	 	decWrite((a),(b))
#define STORE_ZERO(a,b)         decWrite((a),(b))

#define PUSH(b) 		decWrite(SP+0x100,(b));SP--
#define PULL()			decRead((++SP)+0x100)

#define UPPER(ad)		(((ad)>>8)&0xff)
#define LOWER(ad)		((ad)&0xff)
#define LOHI(lo,hi)             ((lo)|((hi)<<8))

#define REL_ADDR(pc,src) 	(pc+((SIGNED_CHAR)src))

#define SET_SIGN(a)		(SF=(a)&S_SIGN)
#define SET_ZERO(a)		(ZF=!(a))
#define SET_CARRY(a)  		(CF=(a))

#define SET_INTERRUPT(a)	(IF=(a))
#define SET_DECIMAL(a)		(DF=(a))
#define SET_OVERFLOW(a)		(OF=(a))
#define SET_BREAK(a)		(BF=(a))

#define SET_SR(a)		(SF=(a) & S_SIGN,\
				 ZF=(a) & S_ZERO,\
				 CF=(a) & S_CARRY,\
				 IF=(a) & S_INTERRUPT,\
				 DF=(a) & S_DECIMAL,\
				 OF=(a) & S_OVERFLOW,\
				 BF=(a) & S_BREAK)

#define GET_SR()		((SF ? S_SIGN : 0) |\
				 (ZF ? S_ZERO : 0) |\
				 (CF ? S_CARRY : 0) |\
				 (IF ? S_INTERRUPT : 0) |\
				 (DF ? S_DECIMAL : 0) |\
				 (OF ? S_OVERFLOW : 0) |\
				 (BF ? S_BREAK : 0) | S_NOTUSED)

#define IF_SIGN()		SF
#define IF_ZERO()		ZF
#define IF_CARRY()		CF
#define IF_INTERRUPT()		IF
#define IF_DECIMAL()		DF
#define IF_OVERFLOW()		OF
#define IF_BREAK()		BF


#define sprint_status()	 sprint_binary(GET_SR())


#endif  /* X2600_MACRO_H */











