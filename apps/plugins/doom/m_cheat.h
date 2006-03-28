// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
// Cheat code checking.
//
//-----------------------------------------------------------------------------


#ifndef __M_CHEAT__
#define __M_CHEAT__

//
// CHEAT SEQUENCE PACKAGE
//

#define SCRAMBLE(a) \
((((a)&1)<<7) + (((a)&2)<<5) + ((a)&4) + (((a)&8)<<1) \
 + (((a)&16)>>1) + ((a)&32) + (((a)&64)>>5) + (((a)&128)>>7))

typedef struct
{
   unsigned char* sequence;
   unsigned char* p;

}
cheatseq_t;

int
cht_CheckCheat
( cheatseq_t*  cht,
  char   key );


void
cht_GetParam
( cheatseq_t*  cht,
  char*   buffer );


#endif
//-----------------------------------------------------------------------------
//
// $Log$
// Revision 1.1  2006/03/28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
//-----------------------------------------------------------------------------
