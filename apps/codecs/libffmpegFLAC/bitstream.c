/*
 * Common bit i/o utils
 * Copyright (c) 2000, 2001 Fabrice Bellard.
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * alternative bitstream reader & writer by Michael Niedermayer <michaelni@gmx.at>
 */

/**
 * @file bitstream.c
 * bitstream api.
 */

#include <stdio.h> 
#include "bitstream.h"

/* bit input functions */

/** 
 * reads 0-32 bits.
 */
unsigned int get_bits_long(GetBitContext *s, int n){
    if(n<=17) return get_bits(s, n);
    else{
        int ret= get_bits(s, 16) << (n-16);
        return ret | get_bits(s, n-16);
    }
}

/** 
 * shows 0-32 bits.
 */
unsigned int show_bits_long(GetBitContext *s, int n){
    if(n<=17) return show_bits(s, n);
    else{
        GetBitContext gb= *s;
        int ret= get_bits_long(s, n);
        *s= gb;
        return ret;
    }
}

void align_get_bits(GetBitContext *s)
{
    int n= (-get_bits_count(s)) & 7;
    if(n) skip_bits(s, n);
}

