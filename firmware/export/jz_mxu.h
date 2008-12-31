/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 * Copyright (C) 2006-2007 by Ingenic Semiconductor Inc.
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

/* Jz47xx Ingenic Media Extension Instruction Set

   These are ~60 SIMD instructions for the Jz47xx MIPS core.
   
   To compile assembly files using these instructions, they
   must be piped through a bash script called mxu_as.
*/

#ifndef JZ_MXU_H_
#define JZ_MXU_H_

#define ptn0 0
#define ptn1 1
#define ptn2 2
#define ptn3 3

#ifdef C_VERSION

/* MXU registers */

#ifndef MXU_REGS_USE_ARRAY

#define xr0 0
static int xr1, xr2, xr3, xr4, xr5, xr6, xr7, xr8, xr9;
static int xr10, xr11, xr12, xr13, xr14, xr15, xr16;

#else

static int mxu_xr[17] = {0};

#define xr0        mxu_xr[ 0]
#define xr1        mxu_xr[ 1]
#define xr2        mxu_xr[ 2]
#define xr3        mxu_xr[ 3]
#define xr4        mxu_xr[ 4]
#define xr5        mxu_xr[ 5]
#define xr6        mxu_xr[ 6]
#define xr7        mxu_xr[ 7]
#define xr8        mxu_xr[ 8]
#define xr9        mxu_xr[ 9]
#define xr10       mxu_xr[10]
#define xr11       mxu_xr[11]
#define xr12       mxu_xr[12]
#define xr13       mxu_xr[13]
#define xr14       mxu_xr[14]
#define xr15       mxu_xr[15]
#define xr16       mxu_xr[16]

#endif

#else /* C_VERSION */

#define xr0  0
#define xr1  1
#define xr2  2
#define xr3  3
#define xr4  4
#define xr5  5
#define xr6  6
#define xr7  7
#define xr8  8
#define xr9  9
#define xr10 10
#define xr11 11
#define xr12 12
#define xr13 13
#define xr14 14
#define xr15 15
#define xr16 16

#endif /* C_VERSION */

#ifdef C_VERSION

#define S32I2M(xr, r)        if (&xr != mxu_xr) xr = r
#define S32M2I(xr)           xr
#define S32LDD(xr, p, o)     if (&xr != mxu_xr) xr = *(long*)((unsigned long)p + o)
#define S32STD(xr, p, o)     *(long*)((unsigned long)p + o) = xr

#define S32LDDV(xr, p, o, s)    if (&xr != mxu_xr) xr = *(long*)((unsigned long)p + ((o) << s))
#define S32STDV(xr, p, o, s)    *(long*)((unsigned long)p + ((o) << s)) = xr

#define S32LDIV(xra, rb, rc, strd2) \
{\
    if (&xra != mxu_xr) xra = *(long*)((unsigned long)rb + ((rc) << strd2));\
    rb  = (char*)rb + ((rc) << strd2);\
}

#define S32SDIV(xra, rb, rc, strd2) \
{\
    *(long*)((unsigned long)rb + ((rc) << strd2)) = xra;\
    rb  = (char*)rb + ((rc) << strd2);\
}

#define S32LDI(xra, rb, o) \
{\
    if (&xra != mxu_xr) xra = *(long*)((unsigned long)rb + o);\
    rb  = (char*)rb + o;\
}

#define S32SDI(xra, rb, o) \
{\
    *(long*)((unsigned long)rb + o) = xra;\
    rb  = (char*)rb + o;\
}

#define S32LDIV(xra, rb, rc, strd2) \
{\
    if (&xra != mxu_xr) xra = *(long*)((unsigned long)rb + ((rc) << strd2));\
    rb  = (char*)rb + ((rc) << strd2);\
}

#define S32SDIV(xra, rb, rc, strd2) \
{\
    *(long*)((unsigned long)rb + ((rc) << strd2)) = xra;\
    rb  = (char*)rb + ((rc) << strd2);\
}

#define Q16ADD_AS_WW(a, b, c, d) \
{\
    short bh = b >> 16;\
    short bl = b & 0xFFFF;\
    short ch = c >> 16;\
    short cl = c & 0xFFFF;\
    int ah = bh + ch;\
    int al = bl + cl;\
    int dh = bh - ch;\
    int dl = bl - cl;\
    if (&a != mxu_xr) a = (ah << 16) | (al & 0xFFFF);\
    if (&d != mxu_xr) d = (dh << 16) | (dl & 0xFFFF);\
}

#define Q16ADD_AS_XW(a, b, c, d) \
{\
    short bh = b >> 16;\
    short bl = b & 0xFFFF;\
    short ch = c >> 16;\
    short cl = c & 0xFFFF;\
    int ah = bl + ch;\
    int al = bh + cl;\
    int dh = bl - ch;\
    int dl = bh - cl;\
    if (&a != mxu_xr) a = (ah << 16) | (al & 0xFFFF);\
    if (&d != mxu_xr) d = (dh << 16) | (dl & 0xFFFF);\
}

#define Q16ADD_AA_WW(a, b, c, d) \
{\
    short bh = b >> 16;\
    short bl = b & 0xFFFF;\
    short ch = c >> 16;\
    short cl = c & 0xFFFF;\
    int ah = bh + ch;\
    int al = bl + cl;\
    if (&a != mxu_xr) a = (ah << 16) | (al & 0xFFFF);\
    if (&d != mxu_xr) d = (ah << 16) | (al & 0xFFFF);\
}

#define D16MUL_LW(a, b, c, d)\
{\
    short bl = b & 0xFFFF;\
    short cl = c & 0xFFFF;\
    short ch = c >> 16;\
    if (&a != mxu_xr) a = ch * bl;\
    if (&d != mxu_xr) d = cl * bl;\
}

#define D16MUL_WW(a, b, c, d)\
{\
    short bh = b >> 16;\
    short bl = b & 0xFFFF;\
    short ch = c >> 16;\
    short cl = c & 0xFFFF;\
    if (&a != mxu_xr) a = ch * bh;\
    if (&d != mxu_xr) d = cl * bl;\
}

#define D16MAC_AA_LW(a, b, c, d)\
{\
    short bl = b & 0xFFFF;\
    short cl = c & 0xFFFF;\
    short ch = c >> 16;\
    if (&a != mxu_xr) a += ch * bl;\
    if (&d != mxu_xr) d += cl * bl;\
}

#define D16MUL_HW(a, b, c, d)\
{\
    short bh = b >> 16;\
    short cl = c & 0xFFFF;\
    short ch = c >> 16;\
    if (&a != mxu_xr) a = ch * bh;\
    if (&d != mxu_xr) d = cl * bh;\
}

#define D16MAC_AA_HW(a, b, c, d)\
{\
    short bh = b >> 16;\
    short cl = c & 0xFFFF;\
    short ch = c >> 16;\
    if (&a != mxu_xr) a += ch * bh;\
    if (&d != mxu_xr) d += cl * bh;\
}

#define D32SLL(a, b, c, d, sft)\
{\
    if (&a != mxu_xr) a = b << sft;\
    if (&d != mxu_xr) d = c << sft;\
}

#define D32SARL(a, b, c, sft) if (&a != mxu_xr) a = (((long)b >> sft) << 16) | (((long)c >> sft) & 0xFFFF)

#define S32SFL(a, b, c, d, ptn) \
{\
    unsigned char b3 = (unsigned char)((unsigned long)b >> 24);\
    unsigned char b2 = (unsigned char)((unsigned long)b >> 16);\
    unsigned char b1 = (unsigned char)((unsigned long)b >>  8);\
    unsigned char b0 = (unsigned char)((unsigned long)b >>  0);\
    unsigned char c3 = (unsigned char)((unsigned long)c >> 24);\
    unsigned char c2 = (unsigned char)((unsigned long)c >> 16);\
    unsigned char c1 = (unsigned char)((unsigned long)c >>  8);\
    unsigned char c0 = (unsigned char)((unsigned long)c >>  0);\
    unsigned char a3, a2, a1, a0, d3, d2, d1, d0;\
    if (ptn0 == ptn)    \
    {\
        a3 = b3;\
        a2 = c3;\
        a1 = b2;\
        a0 = c2;\
        d3 = b1;\
        d2 = c1;\
        d1 = b0;\
        d0 = c0;\
    }\
    else if (ptn1 == ptn)\
    {\
        a3 = b3;\
        a2 = b1;\
        a1 = c3;\
        a0 = c1;\
        d3 = b2;\
        d2 = b0;\
        d1 = c2;\
        d0 = c0;\
    }\
    else if (ptn2 == ptn)\
    {\
        a3 = b3;\
        a2 = c3;\
        a1 = b1;\
        a0 = c1;\
        d3 = b2;\
        d2 = c2;\
        d1 = b0;\
        d0 = c0;\
    }\
    else if (ptn3 == ptn)\
    {\
        a3 = b3;\
        a2 = b2;\
        a1 = c3;\
        a0 = c2;\
        d3 = b1;\
        d2 = b0;\
        d1 = c1;\
        d0 = c0;\
    }\
    if (&a != mxu_xr) a = ((unsigned long)a3 << 24) | ((unsigned long)a2 << 16) | ((unsigned long)a1 << 8) | (unsigned long)a0;\
    if (&d != mxu_xr) d = ((unsigned long)d3 << 24) | ((unsigned long)d2 << 16) | ((unsigned long)d1 << 8) | (unsigned long)d0;\
}

#define D32SAR(a, b, c, d, sft)\
{\
    if (&a != mxu_xr) a = (long)b >> sft;\
    if (&d != mxu_xr) d = (long)c >> sft;\
}

#define D32SLR(a, b, c, d, sft)\
{\
    if (&a != mxu_xr) a = (unsigned long)b >> sft;\
    if (&d != mxu_xr) d = (unsigned long)c >> sft;\
}
#define Q16SLL(a,b,c,d,sft)\
{\
    short bh=b>>16;\
    short bl=b&0xffff;\
    short ch=c>>16;\
    short cl=c&0xffff;\
    if(&a!=mxu_xr) a=((bh<<sft)<<16)|(((long)bl<<sft) & 0xffff);\
    if(&d!=mxu_xr) d=((dh<<sft)<<16)|(((long)bl<<sft) & 0xffff);\
}

#define Q16SAR(a,b,c,d,sft)\
{\
   short bh = b >> 16;\
   short bl = b & 0xffff;\
   short ch = c >> 16;\
   short cl = c & 0xffff;\
   if(&a!=mxu_xr) a=(((short)bh>>sft)<<16)|((long)((short)b1>>sft) & 0xffff);\
   if(&d!=mxu_xr) d=(((short)ch>>sft)<<16)|((long)((short)cl>>sft) & 0xffff);\
}

#define D32ACC_AA(a, b, c, d)\
{\
    int _b = b;\
    int _c = c;\
    int _a = a;\
    int _d = d;\
    if (&a != mxu_xr) a = _a + _b + _c;\
    if (&d != mxu_xr) d = _d + _b + _c;\
}

#define D32ACC_AS(a, b, c, d)\
{\
    int _b = b;\
    int _c = c;\
    int _a = a;\
    int _d = d;\
    if (&a != mxu_xr) a = _a + _b + _c;\
    if (&d != mxu_xr) d = _d + _b - _c;\
}

#define D32ADD_AS(a, b, c, d)\
{\
    int _b = b;\
    int _c = c;\
    if (&a != mxu_xr) a = _b + _c;\
    if (&d != mxu_xr) d = _b - _c;\
}

#define D32ADD_SS(a, b, c, d)\
{\
    int _b = b;\
    int _c = c;\
    if (&a != mxu_xr) a = _b - _c;\
    if (&d != mxu_xr) d = _b - _c;\
}

#define D32ADD_AA(a, b, c, d)\
{\
    int _b = b;\
    int _c = c;\
    if (&a != mxu_xr) a = _b + _c;\
    if (&d != mxu_xr) d = _b + _c;\
}

#define D16MADL_AA_WW(a, b, c, d)                                  \
 do {                                                             \
    short _ah = a >> 16;\
    short _al = (a << 16) >> 16;\
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = _bh * _ch;\
    R32 = _bl * _cl; \
    _ah += (L32 << 16) >> 16; \
    _al += (R32 << 16) >> 16; \
    if (&d != mxu_xr) d = (_ah << 16) + (_al & 0xffff);\
  } while (0)

#define D16MACF_AA_WW(a, b, c, d)                          \
 do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch) << 1;\
    R32 = (_bl * _cl) << 1; \
    L32 = a + L32; \
    R32 = d + R32; \
    if (&a != mxu_xr) a = ((((L32 >> 15) + 1) >> 1) << 16) + ((((R32 >> 15) + 1) >> 1) & 0xffff);\
  } while (0)

#define D16MAC_AA_WW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch);\
    R32 = (_bl * _cl); \
    if (&a != mxu_xr) a = a + L32;\
    if (&d != mxu_xr) d = d + R32;\
  } while (0)

#define D16MAC_SS_WW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch);\
    R32 = (_bl * _cl); \
    if (&a != mxu_xr) a = a - L32;\
    if (&d != mxu_xr) d = d - R32;\
  } while (0)

#define D16MAC_SA_HW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch);\
    R32 = (_bh * _cl); \
    if (&a != mxu_xr) a = a - L32;\
    if (&d != mxu_xr) d = d + R32;\
  } while (0)

#define D16MAC_SS_HW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch);\
    R32 = (_bh * _cl); \
    if (&a != mxu_xr) a = a - L32;\
    if (&d != mxu_xr) d = d - R32;\
  } while (0)

#define D16MAC_AS_HW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bh * _ch);\
    R32 = (_bh * _cl); \
    if (&a != mxu_xr) a = a + L32;\
    if (&d != mxu_xr) d = d - R32;\
  } while (0)

#define D16MAC_AS_LW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bl * _ch);\
    R32 = (_bl * _cl); \
    if (&a != mxu_xr) a = a + L32;\
    if (&d != mxu_xr) d = d - R32;\
  } while (0)


#define D16MAC_SA_LW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bl * _ch);\
    R32 = (_bl * _cl); \
    if (&a != mxu_xr) a = a - L32;\
    if (&d != mxu_xr) d = d + R32;\
  } while (0)

#define D16MAC_SS_LW(a, b, c, d)                           \
do {                                                             \
    short _bh = b >> 16;\
    short _bl = (b << 16) >> 16;\
    short _ch = c >> 16;\
    short _cl = (c << 16) >> 16;\
    int L32, R32; \
    L32 = (_bl * _ch);\
    R32 = (_bl * _cl); \
    if (&a != mxu_xr) a = a - L32;\
    if (&d != mxu_xr) d = d - R32;\
  } while (0)


#define Q8ADDE_AA(xra, xrb, xrc, xrd) \
{\
    unsigned char b3 = (unsigned char)((unsigned long)xrb >> 24);\
    unsigned char b2 = (unsigned char)((unsigned long)xrb >> 16);\
    unsigned char b1 = (unsigned char)((unsigned long)xrb >>  8);\
    unsigned char b0 = (unsigned char)((unsigned long)xrb >>  0);\
    unsigned char c3 = (unsigned char)((unsigned long)xrc >> 24);\
    unsigned char c2 = (unsigned char)((unsigned long)xrc >> 16);\
    unsigned char c1 = (unsigned char)((unsigned long)xrc >>  8);\
    unsigned char c0 = (unsigned char)((unsigned long)xrc >>  0);\
    short ah, al, dh, dl;\
    ah = b3 + c3;\
    al = b2 + c2;\
    dh = b1 + c1;\
    dl = b0 + c0;\
    if (&xra != mxu_xr) xra = ((unsigned long)ah << 16) | (unsigned short)al;\
    if (&xrd != mxu_xr) xrd = ((unsigned long)dh << 16) | (unsigned short)dl;\
}

#define Q16SAT(xra, xrb, xrc) \
{\
    short bh = xrb >> 16;\
    short bl = xrb & 0xFFFF;\
    short ch = xrc >> 16;\
    short cl = xrc & 0xFFFF;\
    if (bh > 255) bh = 255;\
    if (bh < 0) bh = 0;\
    if (bl > 255) bl = 255;\
    if (bl < 0) bl = 0;\
    if (ch > 255) ch = 255;\
    if (ch < 0) ch = 0;\
    if (cl > 255) cl = 255;\
    if (cl < 0) cl = 0;\
    if (&xra != mxu_xr) xra = ((unsigned)bh << 24) | ((unsigned)bl << 16) | ((unsigned)ch << 8) | (unsigned)cl;\
}

#define Q8SAD(xra, xrb, xrc, xrd)    \
{\
    short b3 = (unsigned char)((unsigned long)xrb >> 24);\
    short b2 = (unsigned char)((unsigned long)xrb >> 16);\
    short b1 = (unsigned char)((unsigned long)xrb >>  8);\
    short b0 = (unsigned char)((unsigned long)xrb >>  0);\
    short c3 = (unsigned char)((unsigned long)xrc >> 24);\
    short c2 = (unsigned char)((unsigned long)xrc >> 16);\
    short c1 = (unsigned char)((unsigned long)xrc >>  8);\
    short c0 = (unsigned char)((unsigned long)xrc >>  0);\
    int int0, int1, int2, int3;\
    int3 = labs(b3 - c3);\
    int2 = labs(b2 - c2);\
    int1 = labs(b1 - c1);\
    int0 = labs(b0 - c0);\
    if (&xra != mxu_xr) xra  = int0 + int1 + int2 + int3;\
    if (&xrd != mxu_xr) xrd += int0 + int1 + int2 + int3;\
}

#define Q8AVGR(xra, xrb, xrc) \
{\
    short b3 = (unsigned char)((unsigned long)xrb >> 24);\
    short b2 = (unsigned char)((unsigned long)xrb >> 16);\
    short b1 = (unsigned char)((unsigned long)xrb >>  8);\
    short b0 = (unsigned char)((unsigned long)xrb >>  0);\
    short c3 = (unsigned char)((unsigned long)xrc >> 24);\
    short c2 = (unsigned char)((unsigned long)xrc >> 16);\
    short c1 = (unsigned char)((unsigned long)xrc >>  8);\
    short c0 = (unsigned char)((unsigned long)xrc >>  0);\
    unsigned char a3, a2, a1, a0;\
    a3 = (unsigned char)((b3 + c3 + 1) >> 1);\
    a2 = (unsigned char)((b2 + c2 + 1) >> 1);\
    a1 = (unsigned char)((b1 + c1 + 1) >> 1);\
    a0 = (unsigned char)((b0 + c0 + 1) >> 1);\
    if (&xra != mxu_xr) xra = ((unsigned long)a3 << 24) | ((unsigned long)a2 << 16) | ((unsigned long)a1 << 8) | (unsigned long)a0;\
}

#define S32ALN(xra, xrb, xrc, rs) \
{\
    if (0 == rs)\
    {\
        if (&xra != mxu_xr) xra = xrb;\
    }\
    else if (1 == rs)\
    {\
        if (&xra != mxu_xr) xra = (xrb << 8) | ((unsigned long)xrc >> 24);\
    }\
    else if (2 == rs)\
    {\
        if (&xra != mxu_xr) xra = (xrb << 16) | ((unsigned long)xrc >> 16);\
    }\
    else if (3 == rs)\
    {\
        if (&xra != mxu_xr) xra = (xrb << 24) | ((unsigned long)xrc >> 8);\
    }\
    else if (4 == rs)\
    {\
        if (&xra != mxu_xr) xra = xrc;\
    }\
}

#else /* C_VERSION */

/***********************************LD/SD***********************************/
#define S32LDD(xra,rb,s12)                                        \
  do {                                                            \
    __asm__ __volatile ("S32LDD xr%0,%z1,%2"                    \
            :                                                    \
            :"K"(xra),"d" (rb),"I"(s12));                        \
  } while (0)

#define S32STD(xra,rb,s12)                                        \
  do {                                                            \
    __asm__ __volatile ("S32STD xr%0,%z1,%2"                    \
           :                                                    \
            :"K"(xra),"d" (rb),"I"(s12):"memory");                    \
  } while (0)

#define S32LDDV(xra,rb,rc,strd2)                                \
  do {                                                            \
    __asm__ __volatile ("S32LDDV xr%0,%z1,%z2,%3"                \
           :                                                    \
           :"K"(xra),"d" (rb),"d"(rc),"K"(strd2));                \
  } while (0)

#define S32STDV(xra,rb,rc,strd2)                                \
  do {                                                            \
    __asm__ __volatile ("S32STDV xr%0,%z1,%z2,%3"                \
           :                                                    \
            :"K"(xra),"d" (rb),"d"(rc),"K"(strd2):"memory");        \
  } while (0)

#define S32LDI(xra,rb,s12)                                        \
  do {                                                            \
    __asm__ __volatile ("S32LDI xr%1,%z0,%2"                    \
           :"+d" (rb)                                            \
           :"K"(xra),"I"(s12));                                    \
  } while (0)

#define S32SDI(xra,rb,s12)                                        \
  do {                                                            \
    __asm__ __volatile ("S32SDI xr%1,%z0,%2"                    \
           :"+d" (rb)                                            \
            :"K"(xra),"I"(s12):"memory");            \
  } while (0)

#define S32LDIV(xra,rb,rc,strd2)                                \
  do {                                                            \
    __asm__ __volatile ("S32LDIV xr%1,%z0,%z2,%3"                \
           :"+d" (rb)                                            \
           :"K"(xra),"d"(rc),"K"(strd2));                        \
  } while (0)

#define S32SDIV(xra,rb,rc,strd2)                                \
  do {                                                            \
    __asm__ __volatile ("S32SDIV xr%1,%z0,%z2,%3"                \
           :"+d" (rb)                                            \
            :"K"(xra),"d"(rc),"K"(strd2):"memory");                    \
  } while (0)

/***********************************D16MUL***********************************/
#define D16MUL_WW(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("D16MUL xr%0,xr%1,xr%2,xr%3,WW"         \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MUL_LW(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("D16MUL xr%0,xr%1,xr%2,xr%3,LW"         \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MUL_HW(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("D16MUL xr%0,xr%1,xr%2,xr%3,HW"         \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MUL_XW(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("D16MUL xr%0,xr%1,xr%2,xr%3,XW"         \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/**********************************D16MULF*******************************/
#define D16MULF_WW(xra,xrb,xrc)                                 \
    do {                                                         \
    __asm__ __volatile ("D16MULF xr%0,xr%1,xr%2,WW"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define D16MULF_LW(xra,xrb,xrc)                                 \
 do {                                                             \
    __asm__ __volatile ("D16MULF xr%0,xr%1,xr%2,LW"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define D16MULF_HW(xra,xrb,xrc)                                    \
 do {                                                             \
    __asm__ __volatile ("D16MULF xr%0,xr%1,xr%2,HW"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define D16MULF_XW(xra,xrb,xrc)                                 \
 do {                                                             \
    __asm__ __volatile ("D16MULF xr%0,xr%1,xr%2,XW"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

/***********************************D16MAC********************************/
#define D16MAC_AA_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AA,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AA_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AA,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AA_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AA,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AA_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AA,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AS_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AS,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AS_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AS,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AS_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AS,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_AS_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,AS,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SA_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SA,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SA_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SA,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SA_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SA,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SA_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SA,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SS_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SS,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SS_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SS,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SS_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SS,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MAC_SS_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("D16MAC xr%0,xr%1,xr%2,xr%3,SS,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/**********************************D16MACF*******************************/
#define D16MACF_AA_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AA,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AA_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AA,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AA_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AA,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AA_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AA,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AS_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AS,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AS_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AS,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AS_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AS,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_AS_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,AS,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SA_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SA,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SA_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SA,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SA_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SA,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SA_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SA,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SS_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SS,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SS_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SS,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SS_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SS,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MACF_SS_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MACF xr%0,xr%1,xr%2,xr%3,SS,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/**********************************D16MADL*******************************/
#define D16MADL_AA_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AA,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AA_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AA,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AA_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AA,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AA_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AA,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AS_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AS,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AS_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AS,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AS_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AS,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_AS_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,AS,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SA_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SA,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SA_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SA,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SA_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SA,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SA_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SA,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SS_WW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SS,WW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SS_LW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SS,LW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SS_HW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SS,HW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define D16MADL_SS_XW(xra,xrb,xrc,xrd)                          \
 do {                                                             \
    __asm__ __volatile ("D16MADL xr%0,xr%1,xr%2,xr%3,SS,XW"     \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/***********************************S16MAD*******************************/
#define S16MAD_A_HH(xra,xrb,xrc,xrd)                             \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,A,0"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_A_LL(xra,xrb,xrc,xrd)                            \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,A,1"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_A_HL(xra,xrb,xrc,xrd)                             \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,A,2"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_A_LH(xra,xrb,xrc,xrd)                             \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,A,3"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_S_HH(xra,xrb,xrc,xrd)                            \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,S,0"        \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_S_LL(xra,xrb,xrc,xrd)                            \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,S,1"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_S_HL(xra,xrb,xrc,xrd)                             \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,S,2"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define S16MAD_S_LH(xra,xrb,xrc,xrd)                             \
 do {                                                              \
    __asm__ __volatile ("S16MAD xr%0,xr%1,xr%2,xr%3,S,3"        \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/***********************************Q8MUL********************************/
#define Q8MUL(xra,xrb,xrc,xrd)                                    \
 do {                                                             \
    __asm__ __volatile ("Q8MUL xr%0,xr%1,xr%2,xr%3"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/***********************************Q8MAC********************************/
#define Q8MAC_AA(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q8MAC xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q8MAC_AS(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q8MAC xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q8MAC_SA(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q8MAC xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q8MAC_SS(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q8MAC xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/***********************************Q8MADL********************************/
#define Q8MADL_AA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8MADL xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8MADL_AS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8MADL xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8MADL_SA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8MADL xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8MADL_SS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8MADL xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/***********************************D32ADD********************************/
#define D32ADD_AA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ADD xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ADD_AS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ADD xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ADD_SA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ADD xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ADD_SS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ADD xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/***********************************D32ACC********************************/
#define D32ACC_AA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ACC xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ACC_AS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ACC xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ACC_SA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ACC xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define D32ACC_SS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("D32ACC xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/***********************************S32CPS********************************/
#define S32CPS(xra,xrb,xrc)                                        \
 do {                                                            \
    __asm__ __volatile ("S32CPS xr%0,xr%1,xr%2"                    \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

#define S32ABS(xra,xrb)                                            \
 do {                                                            \
    __asm__ __volatile ("S32CPS xr%0,xr%1,xr%2"                    \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrb));                    \
  } while (0)

/***********************************Q16ADD********************************/
#define Q16ADD_AA_WW(xra,xrb,xrc,xrd)                            \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AA,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AA_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AA,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AA_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AA,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AA_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AA,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)
#define Q16ADD_AS_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AS,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AS_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AS,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AS_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AS,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_AS_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,AS,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SA_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SA,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SA_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SA,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SA_HW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SA,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SA_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SA,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SS_WW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SS,WW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SS_LW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SS,LW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SS_HW(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SS,HW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ADD_SS_XW(xra,xrb,xrc,xrd)                           \
 do {                                                             \
    __asm__ __volatile ("Q16ADD xr%0,xr%1,xr%2,xr%3,SS,XW"      \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/***********************************Q16ACC********************************/
#define Q16ACC_AA(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q16ACC xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ACC_AS(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q16ACC xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ACC_SA(xra,xrb,xrc,xrd)                                \
 do {                                                             \
    __asm__ __volatile ("Q16ACC xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

#define Q16ACC_SS(xra,xrb,xrc,xrd)                              \
 do {                                                             \
    __asm__ __volatile ("Q16ACC xr%0,xr%1,xr%2,xr%3,SS"         \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));         \
  } while (0)

/***********************************D16CPS********************************/
#define D16CPS(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("D16CPS xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

#define D16ABS(xra,xrb)                                            \
 do {                                                             \
    __asm__ __volatile ("D16CPS xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrb));                    \
  } while (0)

/*******************************D16AVG/D16AVGR*****************************/
#define D16AVG(xra,xrb,xrc)                                        \
 do {                                                           \
    __asm__ __volatile ("D16AVG xr%0,xr%1,xr%2"                 \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)
#define D16AVGR(xra,xrb,xrc)                                    \
 do {                                                           \
    __asm__ __volatile ("D16AVGR xr%0,xr%1,xr%2"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

/************************************Q8ADD********************************/
#define Q8ADD_AA(xra,xrb,xrc)                                    \
    do {                                                        \
    __asm__ __volatile ("Q8ADD xr%0,xr%1,xr%2,AA"                \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

#define Q8ADD_AS(xra,xrb,xrc)                                    \
 do {                                                            \
    __asm__ __volatile ("Q8ADD xr%0,xr%1,xr%2,AS"                \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

#define Q8ADD_SA(xra,xrb,xrc)                                    \
 do {                                                            \
    __asm__ __volatile ("Q8ADD xr%0,xr%1,xr%2,SA"                \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

#define Q8ADD_SS(xra,xrb,xrc)                                    \
 do {                                                            \
    __asm__ __volatile ("Q8ADD xr%0,xr%1,xr%2,SS"                \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

/************************************Q8ADDE********************************/
#define Q8ADDE_AA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ADDE xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ADDE_AS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ADDE xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ADDE_SA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ADDE xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ADDE_SS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ADDE xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/************************************Q8ACCE********************************/
#define Q8ACCE_AA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ACCE xr%0,xr%1,xr%2,xr%3,AA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ACCE_AS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ACCE xr%0,xr%1,xr%2,xr%3,AS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ACCE_SA(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ACCE xr%0,xr%1,xr%2,xr%3,SA"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

#define Q8ACCE_SS(xra,xrb,xrc,xrd)                                \
 do {                                                            \
    __asm__ __volatile ("Q8ACCE xr%0,xr%1,xr%2,xr%3,SS"            \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/************************************Q8ABD********************************/
#define Q8ABD(xra,xrb,xrc)                                        \
 do {                                                            \
    __asm__ __volatile ("Q8ABD xr%0,xr%1,xr%2"                    \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

/************************************Q8SLT********************************/
#define Q8SLT(xra,xrb,xrc)                                        \
 do {                                                            \
    __asm__ __volatile ("Q8SLT xr%0,xr%1,xr%2"                    \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)

/************************************Q8SAD********************************/
#define Q8SAD(xra,xrb,xrc,xrd)                                    \
 do {                                                             \
    __asm__ __volatile ("Q8SAD xr%0,xr%1,xr%2,xr%3"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd));            \
  } while (0)

/********************************Q8AVG/Q8AVGR*****************************/
#define Q8AVG(xra,xrb,xrc)                                        \
 do {                                                            \
    __asm__ __volatile ("Q8AVG xr%0,xr%1,xr%2"                    \
                 :                                                \
                 :"K"(xra),"K"(xrb),"K"(xrc));                    \
  } while (0)
#define Q8AVGR(xra,xrb,xrc)                                       \
 do {                                                           \
    __asm__ __volatile ("Q8AVGR xr%0,xr%1,xr%2"                 \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

/**********************************D32SHIFT******************************/
#define D32SLL(xra,xrb,xrc,xrd,SFT4)                                \
    do {                                                             \
    __asm__ __volatile ("D32SLL xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));   \
  } while (0)

#define D32SLR(xra,xrb,xrc,xrd,SFT4)                                \
 do {                                                                 \
    __asm__ __volatile ("D32SLR xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));   \
  } while (0)

#define D32SAR(xra,xrb,xrc,xrd,SFT4)                                \
 do {                                                                 \
    __asm__ __volatile ("D32SAR xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));   \
  } while (0)

#define D32SARL(xra,xrb,xrc,SFT4)                                    \
 do {                                                                 \
    __asm__ __volatile ("D32SARL xr%0,xr%1,xr%2,%3"                    \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(SFT4));            \
  } while (0)

#define D32SLLV(xra,xrd,rb)                                            \
 do {                                                                 \
    __asm__ __volatile ("D32SLLV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

#define D32SLRV(xra,xrd,rb)                                            \
 do {                                                                 \
    __asm__ __volatile ("D32SLRV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

#define D32SARV(xra,xrd,rb)                                            \
 do {                                                                 \
    __asm__ __volatile ("D32SARV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

#define D32SARW(xra,xrb,xrc,rb)                                        \
 do {                                                                 \
    __asm__ __volatile ("D32SARW xr%0,xr%1,xr%2,%3"                    \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"d"(rb));                \
  } while (0)

/**********************************Q16SHIFT******************************/
#define Q16SLL(xra,xrb,xrc,xrd,SFT4)                                \
    do {                                                             \
    __asm__ __volatile ("Q16SLL xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));    \
  } while (0)

#define Q16SLR(xra,xrb,xrc,xrd,SFT4)                                \
    do {                                                             \
    __asm__ __volatile ("Q16SLR xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));    \
  } while (0)

#define Q16SAR(xra,xrb,xrc,xrd,SFT4)                                \
    do {                                                             \
    __asm__ __volatile ("Q16SAR xr%0,xr%1,xr%2,xr%3,%4"                \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(SFT4));    \
  } while (0)

#define Q16SLLV(xra,xrd,rb)                                            \
    do {                                                             \
    __asm__ __volatile ("Q16SLLV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

#define Q16SLRV(xra,xrd,rb)                                            \
    do {                                                             \
    __asm__ __volatile ("Q16SLRV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

#define Q16SARV(xra,xrd,rb)                                            \
    do {                                                             \
    __asm__ __volatile ("Q16SARV xr%0,xr%1,%z2"                        \
                 :                                                    \
                 :"K"(xra),"K"(xrd),"d"(rb));                        \
  } while (0)

/*********************************MAX/MIN*********************************/
#define S32MAX(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("S32MAX xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define S32MIN(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("S32MIN xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define D16MAX(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("D16MAX xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define D16MIN(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("D16MIN xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define Q8MAX(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("Q8MAX xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

#define Q8MIN(xra,xrb,xrc)                                        \
 do {                                                             \
    __asm__ __volatile ("Q8MIN xr%0,xr%1,xr%2"                    \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc));                  \
  } while (0)

/*************************************MOVE********************************/
#define S32I2M(xra,rb)                            \
  do {                                             \
    __asm__ __volatile ("S32I2M xr%0,%z1"        \
           :                                    \
           :"K"(xra),"d"(rb));                    \
  } while (0)

#define S32M2I(xra)                                \
__extension__ ({                                \
  int  __d;                                        \
  __asm__ __volatile ("S32M2I xr%1, %0"            \
       :"=d"(__d)                                \
       :"K"(xra));                              \
  __d;                                             \
})

/*********************************S32SFL**********************************/
#define S32SFL(xra,xrb,xrc,xrd,optn2)                                \
    do {                                                             \
    __asm__ __volatile ("S32SFL xr%0,xr%1,xr%2,xr%3,ptn%4"            \
                 :                                                    \
                 :"K"(xra),"K"(xrb),"K"(xrc),"K"(xrd),"K"(optn2));    \
  } while (0)

/*********************************S32ALN**********************************/
#define S32ALN(xra,xrb,xrc,rs)                                    \
 do {                                                             \
    __asm__ __volatile ("S32ALN xr%0,xr%1,xr%2,%z3"                \
                 :                                              \
                 :"K"(xra),"K"(xrb),"K"(xrc),"d"(rs));            \
  } while (0)

/*********************************Q16SAT**********************************/
#define Q16SAT(xra,xrb,xrc)                                    \
 do {                                                         \
    __asm__ __volatile ("Q16SAT xr%0,xr%1,xr%2"                \
                 :                                          \
                 :"K"(xra),"K"(xrb),"K"(xrc));                \
  } while (0)

// cache ops

// cache
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Load_Tag_I        0x04
#define Index_Load_Tag_D        0x05
#define Index_Store_Tag_I       0x08
#define Index_Store_Tag_D       0x09
#define Hit_Invalidate_I        0x10
#define Hit_Invalidate_D        0x11
#define Hit_Writeback_Inv_D     0x15
#define Hit_Writeback_I         0x18
#define Hit_Writeback_D         0x19

// pref
#define PrefLoad                0
#define PrefStore               1
#define PrefLoadStreamed        4
#define PrefStoreStreamed       5
#define PrefLoadRetained        6
#define PrefStoreRetained       7
#define PrefWBInval             25
#define PrefNudge               25
#define PrefPreForStore         30

#define mips_pref(base, offset, op)          \
    __asm__ __volatile__(                    \
    "    .set    noreorder        \n"        \
    "    pref    %1, %2(%0)       \n"        \
    "    .set    reorder"                    \
    :                                        \
    : "r" (base), "i" (op), "i" (offset))

#define cache_op(op, addr)                   \
    __asm__ __volatile__(                    \
    "    .set    noreorder        \n"        \
    "    cache    %0, %1          \n"        \
    "    .set    reorder"                    \
    :                                        \
    : "i" (op), "m" (*(unsigned char *)(addr)))

#define i_pref(hint,base,offset)    \
    ({ __asm__ __volatile__("pref %0,%2(%1)"::"i"(hint),"r"(base),"i"(offset):"memory");})    

struct unaligned_32 { unsigned int l; } __attribute__((packed));
#define LD32(a) (((const struct unaligned_32 *) (a))->l)
#define ST32(a, b) (((struct unaligned_32 *) (a))->l) = (b)

#define REVERSE_LD32(xra, xrb, rb, s12)             \
__extension__ ({                                    \
  int  __d;                                         \
    __asm__ __volatile ("S32LDD xr%1,%z3,%4\n\t"    \
                        "S32SFL  xr%1,xr%1, xr%1, xr%2, ptn0\n\t"    \
                        "S32SFL  xr%1,xr%2, xr%1, xr%2, ptn3\n\t"    \
                        "S32SFL  xr%1,xr%2, xr%1, xr%2, ptn2\n\t"    \
                        "S32M2I  xr%1,%0"                            \
         :"=d"(__d)                                 \
         :"K"(xra), "K"(xrb), "d"(rb), "I"(s12));   \
  __d;                                              \
})

#define IU_CLZ(rb)                              \
__extension__ ({                                \
  int  __d;                                     \
  __asm__ __volatile ("clz %0, %1"              \
       :"=d"(__d)                               \
       :"d"(rb));                               \
  __d;                                          \
})

#endif /* C_VERSION */

#endif /* JZ_MXU_H_ */
