/*
  Copyright (c) 2005, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// \file synth_filter.c
/// Synthesis functions.
/// \todo document me

#include "musepack.h"
#include "internal.h"

/* C O N S T A N T S */
#undef _

#if defined(MPC_FIXED_POINT)
   #if defined(OPTIMIZE_FOR_SPEED)
      // round at compile time to +/- 2^14 as a pre-shift before 32=32x32-multiply
      #define D(value)  (MPC_SHR_RND(value, 3))
      
      // round at runtime to +/- 2^17 as a pre-shift before 32=32x32-multiply
      // samples are 18.14 fixed point. 30.2 after this shift, whereas the
      // 15.2 bits are significant (not including sign)
      #define MPC_V_PRESHIFT(X) MPC_SHR_RND(X, 12)
      
      // in this configuration a post-shift by >>1 is needed after synthesis
   #else
      #if defined(CPU_ARM)
          // do not up-scale D-values to achieve higher speed in smull/mlal
          // operations. saves ~14/8 = 1.75 cycles per multiplication
          #define D(value)  (value)
          
          // in this configuration a post-shift by >>16 is needed after synthesis
      #else
          // saturate to +/- 2^31 (= value << (31-17)), D-values are +/- 2^17
          #define D(value)  (value << (14))
      #endif
      // do not perform pre-shift
      #define MPC_V_PRESHIFT(X) (X)
   #endif
#else
   // IMPORTANT: internal scaling is somehow strange for floating point, therefore we scale the coefficients Di_opt
   // by the correct amount to have proper scaled output
   #define D(value)  MAKE_MPC_SAMPLE((double)value*(double)(0x1000))
   
   // do not perform pre-shift
   #define MPC_V_PRESHIFT(X) (X)
#endif
    
// Di_opt coefficients are +/- 2^17 (pre-shifted by <<16)
static const MPC_SAMPLE_FORMAT  Di_opt [512] ICONST_ATTR = {
/*           0        1        2         3         4         5          6          7         8         9       10        11       12       13      14     15  */
/*  0 */  D( 0), -D( 29), D( 213), -D( 459),  D(2037), -D(5153),  D( 6574), -D(37489), D(75038),  D(37489), D(6574),  D(5153), D(2037),  D(459), D(213), D(29),
/*  1 */ -D( 1), -D( 31), D( 218), -D( 519),  D(2000), -D(5517),  D( 5959), -D(39336), D(74992),  D(35640), D(7134),  D(4788), D(2063),  D(401), D(208), D(26),
/*  2 */ -D( 1), -D( 35), D( 222), -D( 581),  D(1952), -D(5879),  D( 5288), -D(41176), D(74856),  D(33791), D(7640),  D(4425), D(2080),  D(347), D(202), D(24),
/*  3 */ -D( 1), -D( 38), D( 225), -D( 645),  D(1893), -D(6237),  D( 4561), -D(43006), D(74630),  D(31947), D(8092),  D(4063), D(2087),  D(294), D(196), D(21),
/*  4 */ -D( 1), -D( 41), D( 227), -D( 711),  D(1822), -D(6589),  D( 3776), -D(44821), D(74313),  D(30112), D(8492),  D(3705), D(2085),  D(244), D(190), D(19),
/*  5 */ -D( 1), -D( 45), D( 228), -D( 779),  D(1739), -D(6935),  D( 2935), -D(46617), D(73908),  D(28289), D(8840),  D(3351), D(2075),  D(197), D(183), D(17),
/*  6 */ -D( 1), -D( 49), D( 228), -D( 848),  D(1644), -D(7271),  D( 2037), -D(48390), D(73415),  D(26482), D(9139),  D(3004), D(2057),  D(153), D(176), D(16),
/*  7 */ -D( 2), -D( 53), D( 227), -D( 919),  D(1535), -D(7597),  D( 1082), -D(50137), D(72835),  D(24694), D(9389),  D(2663), D(2032),  D(111), D(169), D(14),
/*  8 */ -D( 2), -D( 58), D( 224), -D( 991),  D(1414), -D(7910),  D(   70), -D(51853), D(72169),  D(22929), D(9592),  D(2330), D(2001),  D( 72), D(161), D(13),
/*  9 */ -D( 2), -D( 63), D( 221), -D(1064),  D(1280), -D(8209), -D(  998), -D(53534), D(71420),  D(21189), D(9750),  D(2006), D(1962),  D( 36), D(154), D(11),
/* 10 */ -D( 2), -D( 68), D( 215), -D(1137),  D(1131), -D(8491), -D( 2122), -D(55178), D(70590),  D(19478), D(9863),  D(1692), D(1919),  D(  2), D(147), D(10),
/* 11 */ -D( 3), -D( 73), D( 208), -D(1210),  D( 970), -D(8755), -D( 3300), -D(56778), D(69679),  D(17799), D(9935),  D(1388), D(1870), -D( 29), D(139), D( 9),
/* 12 */ -D( 3), -D( 79), D( 200), -D(1283),  D( 794), -D(8998), -D( 4533), -D(58333), D(68692),  D(16155), D(9966),  D(1095), D(1817), -D( 57), D(132), D( 8),
/* 13 */ -D( 4), -D( 85), D( 189), -D(1356),  D( 605), -D(9219), -D( 5818), -D(59838), D(67629),  D(14548), D(9959),  D( 814), D(1759), -D( 83), D(125), D( 7),
/* 14 */ -D( 4), -D( 91), D( 177), -D(1428),  D( 402), -D(9416), -D( 7154), -D(61289), D(66494),  D(12980), D(9916),  D( 545), D(1698), -D(106), D(117), D( 7),
/* 15 */ -D( 5), -D( 97), D( 163), -D(1498),  D( 185), -D(9585), -D( 8540), -D(62684), D(65290),  D(11455), D(9838),  D( 288), D(1634), -D(127), D(111), D( 6),
/* 16 */ -D( 5), -D(104), D( 146), -D(1567), -D(  45), -D(9727), -D( 9975), -D(64019), D(64019),  D( 9975), D(9727),  D(  45), D(1567), -D(146), D(104), D( 5),
/* 17 */ -D( 6), -D(111), D( 127), -D(1634), -D( 288), -D(9838), -D(11455), -D(65290), D(62684),  D( 8540), D(9585), -D( 185), D(1498), -D(163), D( 97), D( 5),
/* 18 */ -D( 7), -D(117), D( 106), -D(1698), -D( 545), -D(9916), -D(12980), -D(66494), D(61289),  D( 7154), D(9416), -D( 402), D(1428), -D(177), D( 91), D( 4),
/* 19 */ -D( 7), -D(125), D(  83), -D(1759), -D( 814), -D(9959), -D(14548), -D(67629), D(59838),  D( 5818), D(9219), -D( 605), D(1356), -D(189), D( 85), D( 4),
/* 20 */ -D( 8), -D(132), D(  57), -D(1817), -D(1095), -D(9966), -D(16155), -D(68692), D(58333),  D( 4533), D(8998), -D( 794), D(1283), -D(200), D( 79), D( 3),
/* 21 */ -D( 9), -D(139), D(  29), -D(1870), -D(1388), -D(9935), -D(17799), -D(69679), D(56778),  D( 3300), D(8755), -D( 970), D(1210), -D(208), D( 73), D( 3),
/* 22 */ -D(10), -D(147), -D(  2), -D(1919), -D(1692), -D(9863), -D(19478), -D(70590), D(55178),  D( 2122), D(8491), -D(1131), D(1137), -D(215), D( 68), D( 2),
/* 23 */ -D(11), -D(154), -D( 36), -D(1962), -D(2006), -D(9750), -D(21189), -D(71420), D(53534),  D(  998), D(8209), -D(1280), D(1064), -D(221), D( 63), D( 2),
/* 24 */ -D(13), -D(161), -D( 72), -D(2001), -D(2330), -D(9592), -D(22929), -D(72169), D(51853), -D(   70), D(7910), -D(1414), D( 991), -D(224), D( 58), D( 2),
/* 25 */ -D(14), -D(169), -D(111), -D(2032), -D(2663), -D(9389), -D(24694), -D(72835), D(50137), -D( 1082), D(7597), -D(1535), D( 919), -D(227), D( 53), D( 2),
/* 26 */ -D(16), -D(176), -D(153), -D(2057), -D(3004), -D(9139), -D(26482), -D(73415), D(48390), -D( 2037), D(7271), -D(1644), D( 848), -D(228), D( 49), D( 1),
/* 27 */ -D(17), -D(183), -D(197), -D(2075), -D(3351), -D(8840), -D(28289), -D(73908), D(46617), -D( 2935), D(6935), -D(1739), D( 779), -D(228), D( 45), D( 1),
/* 28 */ -D(19), -D(190), -D(244), -D(2085), -D(3705), -D(8492), -D(30112), -D(74313), D(44821), -D( 3776), D(6589), -D(1822), D( 711), -D(227), D( 41), D( 1),
/* 29 */ -D(21), -D(196), -D(294), -D(2087), -D(4063), -D(8092), -D(31947), -D(74630), D(43006), -D( 4561), D(6237), -D(1893), D( 645), -D(225), D( 38), D( 1),
/* 30 */ -D(24), -D(202), -D(347), -D(2080), -D(4425), -D(7640), -D(33791), -D(74856), D(41176), -D( 5288), D(5879), -D(1952), D( 581), -D(222), D( 35), D( 1),
/* 31 */ -D(26), -D(208), -D(401), -D(2063), -D(4788), -D(7134), -D(35640), -D(74992), D(39336), -D( 5959), D(5517), -D(2000), D( 519), -D(218), D( 31), D( 1)
};

#undef  D

// needed to prevent from internal overflow in calculate_V
#define OVERFLOW_FIX 1

// V-coefficients were expanded (<<) by V_COEFFICIENT_EXPAND
#define V_COEFFICIENT_EXPAND 27

#if defined(MPC_FIXED_POINT)
   #if defined(OPTIMIZE_FOR_SPEED)
      // define 32=32x32-multiplication for DCT-coefficients with samples, vcoef will be pre-shifted on creation
      // samples are rounded to +/- 2^19 as pre-shift before 32=32x32-multiply
      #define MPC_MULTIPLY_V(sample, vcoef) ( MPC_SHR_RND(sample, 12) * vcoef )
      
      // pre- and postscale are used to avoid internal overflow in synthesis calculation
      #define MPC_MULTIPLY_V_PRESCALE(sample, vcoef)  ( MPC_SHR_RND(sample, (12+OVERFLOW_FIX)) * vcoef )
      #define MPC_MULTIPLY_V_POSTSCALE(sample, vcoef) ( MPC_SHR_RND(sample, (12-OVERFLOW_FIX)) * vcoef )
      #define MPC_V_POSTSCALE(sample) (sample<<OVERFLOW_FIX)
      
      // round to +/- 2^16 as pre-shift before 32=32x32-multiply
      #define MPC_MAKE_INVCOS(value) (MPC_SHR_RND(value, 15))
   #else
      // define 64=32x32-multiplication for DCT-coefficients with samples. Via usage of MPC_FRACT highly optimized assembler might be used
      // MULTIPLY_FRACT will do >>32 after multiplication, as V-coef were expanded by V_COEFFICIENT_EXPAND we'll correct this on the result.
      // Will loose 5bit accuracy on result in fract part without effect on final audio result
      #define MPC_MULTIPLY_V(sample, vcoef) ( (MPC_MULTIPLY_FRACT(sample, vcoef)) << (32-V_COEFFICIENT_EXPAND) )
      
      // pre- and postscale are used to avoid internal overflow in synthesis calculation
      #define MPC_MULTIPLY_V_PRESCALE(sample, vcoef)  ( (MPC_MULTIPLY_FRACT(sample, vcoef)) << (32-V_COEFFICIENT_EXPAND-OVERFLOW_FIX) )
      #define MPC_MULTIPLY_V_POSTSCALE(sample, vcoef) ( (MPC_MULTIPLY_FRACT(sample, vcoef)) << (32-V_COEFFICIENT_EXPAND+OVERFLOW_FIX) )
      #define MPC_V_POSTSCALE(sample) (sample<<OVERFLOW_FIX)
      
      // directly use accurate 32bit-coefficients
      #define MPC_MAKE_INVCOS(value) (value)
   #endif
#else
   // for floating point use the standard multiplication macro
   #define MPC_MULTIPLY_V          (sample, vcoef) ( MPC_MULTIPLY(sample, vcoef) )
   #define MPC_MULTIPLY_V_PRESCALE (sample, vcoef) ( MPC_MULTIPLY(sample, vcoef) )
   #define MPC_MULTIPLY_V_POSTSCALE(sample, vcoef) ( MPC_MULTIPLY(sample, vcoef) )
   #define MPC_V_POSTSCALE(sample) (sample)
   
   // downscale the accurate 32bit-coefficients and convert to float
   #define MPC_MAKE_INVCOS(value) MAKE_MPC_SAMPLE((double)value/(double)(1<<V_COEFFICIENT_EXPAND))
#endif

// define constants for DCT-synthesis
// INVCOSxx = (0.5 / cos(xx*PI/64)) << 27, <<27 to saturate to +/- 2^31
#define INVCOS01 MPC_MAKE_INVCOS(  67189797)
#define INVCOS02 MPC_MAKE_INVCOS(  67433575)
#define INVCOS03 MPC_MAKE_INVCOS(  67843164)
#define INVCOS04 MPC_MAKE_INVCOS(  68423604)
#define INVCOS05 MPC_MAKE_INVCOS(  69182167)
#define INVCOS06 MPC_MAKE_INVCOS(  70128577)
#define INVCOS07 MPC_MAKE_INVCOS(  71275330)
#define INVCOS08 MPC_MAKE_INVCOS(  72638111)
#define INVCOS09 MPC_MAKE_INVCOS(  74236348)
#define INVCOS10 MPC_MAKE_INVCOS(  76093940)
#define INVCOS11 MPC_MAKE_INVCOS(  78240207)
#define INVCOS12 MPC_MAKE_INVCOS(  80711144)
#define INVCOS13 MPC_MAKE_INVCOS(  83551089)
#define INVCOS14 MPC_MAKE_INVCOS(  86814950)
#define INVCOS15 MPC_MAKE_INVCOS(  90571242)
#define INVCOS16 MPC_MAKE_INVCOS(  94906266)
#define INVCOS17 MPC_MAKE_INVCOS(  99929967)
#define INVCOS18 MPC_MAKE_INVCOS( 105784321)
#define INVCOS19 MPC_MAKE_INVCOS( 112655602)
#define INVCOS20 MPC_MAKE_INVCOS( 120792764)
#define INVCOS21 MPC_MAKE_INVCOS( 130535899)
#define INVCOS22 MPC_MAKE_INVCOS( 142361749)
#define INVCOS23 MPC_MAKE_INVCOS( 156959571)
#define INVCOS24 MPC_MAKE_INVCOS( 175363913)
#define INVCOS25 MPC_MAKE_INVCOS( 199201203)
#define INVCOS26 MPC_MAKE_INVCOS( 231182936)
#define INVCOS27 MPC_MAKE_INVCOS( 276190692)
#define INVCOS28 MPC_MAKE_INVCOS( 343988688)
#define INVCOS29 MPC_MAKE_INVCOS( 457361460)
#define INVCOS30 MPC_MAKE_INVCOS( 684664578)
#define INVCOS31 MPC_MAKE_INVCOS(1367679739)

void 
mpc_calculate_new_V ( const MPC_SAMPLE_FORMAT * Sample, MPC_SAMPLE_FORMAT * V )
ICODE_ATTR_MPC_LARGE_IRAM;

void 
mpc_calculate_new_V ( const MPC_SAMPLE_FORMAT * Sample, MPC_SAMPLE_FORMAT * V )
{
    // Calculating new V-buffer values for left channel
    // calculate new V-values (ISO-11172-3, p. 39)
    // based upon fast-MDCT algorithm by Byeong Gi Lee
    MPC_SAMPLE_FORMAT A[16];
    MPC_SAMPLE_FORMAT B[16];
    MPC_SAMPLE_FORMAT tmp;

    A[ 0] = Sample[ 0] + Sample[31];
    A[ 1] = Sample[ 1] + Sample[30];
    A[ 2] = Sample[ 2] + Sample[29];
    A[ 3] = Sample[ 3] + Sample[28];
    A[ 4] = Sample[ 4] + Sample[27];
    A[ 5] = Sample[ 5] + Sample[26];
    A[ 6] = Sample[ 6] + Sample[25];
    A[ 7] = Sample[ 7] + Sample[24];
    A[ 8] = Sample[ 8] + Sample[23];
    A[ 9] = Sample[ 9] + Sample[22];
    A[10] = Sample[10] + Sample[21];
    A[11] = Sample[11] + Sample[20];
    A[12] = Sample[12] + Sample[19];
    A[13] = Sample[13] + Sample[18];
    A[14] = Sample[14] + Sample[17];
    A[15] = Sample[15] + Sample[16];
    // 16 adds

    B[ 0] = A[ 0] + A[15];
    B[ 1] = A[ 1] + A[14];
    B[ 2] = A[ 2] + A[13];
    B[ 3] = A[ 3] + A[12];
    B[ 4] = A[ 4] + A[11];
    B[ 5] = A[ 5] + A[10];
    B[ 6] = A[ 6] + A[ 9];
    B[ 7] = A[ 7] + A[ 8];;
    B[ 8] = MPC_MULTIPLY_V((A[ 0] - A[15]), INVCOS02);
    B[ 9] = MPC_MULTIPLY_V((A[ 1] - A[14]), INVCOS06);
    B[10] = MPC_MULTIPLY_V((A[ 2] - A[13]), INVCOS10);
    B[11] = MPC_MULTIPLY_V((A[ 3] - A[12]), INVCOS14);
    B[12] = MPC_MULTIPLY_V((A[ 4] - A[11]), INVCOS18);
    B[13] = MPC_MULTIPLY_V((A[ 5] - A[10]), INVCOS22);
    B[14] = MPC_MULTIPLY_V((A[ 6] - A[ 9]), INVCOS26);
    B[15] = MPC_MULTIPLY_V((A[ 7] - A[ 8]), INVCOS30);
    // 8 adds, 8 subs, 8 muls, 8 shifts

    A[ 0] = B[ 0] + B[ 7];
    A[ 1] = B[ 1] + B[ 6];
    A[ 2] = B[ 2] + B[ 5];
    A[ 3] = B[ 3] + B[ 4];
    A[ 4] = MPC_MULTIPLY_V((B[ 0] - B[ 7]), INVCOS04);
    A[ 5] = MPC_MULTIPLY_V((B[ 1] - B[ 6]), INVCOS12);
    A[ 6] = MPC_MULTIPLY_V((B[ 2] - B[ 5]), INVCOS20);
    A[ 7] = MPC_MULTIPLY_V((B[ 3] - B[ 4]), INVCOS28);
    A[ 8] = B[ 8] + B[15];
    A[ 9] = B[ 9] + B[14];
    A[10] = B[10] + B[13];
    A[11] = B[11] + B[12];
    A[12] = MPC_MULTIPLY_V((B[ 8] - B[15]), INVCOS04);
    A[13] = MPC_MULTIPLY_V((B[ 9] - B[14]), INVCOS12);
    A[14] = MPC_MULTIPLY_V((B[10] - B[13]), INVCOS20);
    A[15] = MPC_MULTIPLY_V((B[11] - B[12]), INVCOS28);
    // 8 adds, 8 subs, 8 muls, 8 shifts

    B[ 0] = A[ 0] + A[ 3];
    B[ 1] = A[ 1] + A[ 2];
    B[ 2] = MPC_MULTIPLY_V((A[ 0] - A[ 3]), INVCOS08);
    B[ 3] = MPC_MULTIPLY_V((A[ 1] - A[ 2]), INVCOS24);
    B[ 4] = A[ 4] + A[ 7];
    B[ 5] = A[ 5] + A[ 6];
    B[ 6] = MPC_MULTIPLY_V((A[ 4] - A[ 7]), INVCOS08);
    B[ 7] = MPC_MULTIPLY_V((A[ 5] - A[ 6]), INVCOS24);
    B[ 8] = A[ 8] + A[11];
    B[ 9] = A[ 9] + A[10];
    B[10] = MPC_MULTIPLY_V((A[ 8] - A[11]), INVCOS08);
    B[11] = MPC_MULTIPLY_V((A[ 9] - A[10]), INVCOS24);
    B[12] = A[12] + A[15];
    B[13] = A[13] + A[14];
    B[14] = MPC_MULTIPLY_V((A[12] - A[15]), INVCOS08);
    B[15] = MPC_MULTIPLY_V((A[13] - A[14]), INVCOS24);
    // 8 adds, 8 subs, 8 muls, 8 shifts

    A[ 0] = B[ 0] + B[ 1];
    A[ 1] = MPC_MULTIPLY_V((B[ 0] - B[ 1]), INVCOS16);
    A[ 2] = B[ 2] + B[ 3];
    A[ 3] = MPC_MULTIPLY_V((B[ 2] - B[ 3]), INVCOS16);
    A[ 4] = B[ 4] + B[ 5];
    A[ 5] = MPC_MULTIPLY_V((B[ 4] - B[ 5]), INVCOS16);
    A[ 6] = B[ 6] + B[ 7];
    A[ 7] = MPC_MULTIPLY_V((B[ 6] - B[ 7]), INVCOS16);
    A[ 8] = B[ 8] + B[ 9];
    A[ 9] = MPC_MULTIPLY_V((B[ 8] - B[ 9]), INVCOS16);
    A[10] = B[10] + B[11];
    A[11] = MPC_MULTIPLY_V((B[10] - B[11]), INVCOS16);
    A[12] = B[12] + B[13];
    A[13] = MPC_MULTIPLY_V((B[12] - B[13]), INVCOS16);
    A[14] = B[14] + B[15];
    A[15] = MPC_MULTIPLY_V((B[14] - B[15]), INVCOS16);
    // 8 adds, 8 subs, 8 muls, 8 shifts

    // multiple used expressions: -(A[12] + A[14] + A[15])
    V[48] = -A[ 0];
    V[ 0] =  A[ 1];
    V[40] = -A[ 2] - (V[ 8] = A[ 3]);
    V[36] = -((V[ 4] = A[ 5] + (V[12] = A[ 7])) + A[ 6]);
    V[44] = - A[ 4] - A[ 6] - A[ 7];
    V[ 6] = (V[10] = A[11] + (V[14] = A[15])) + A[13];
    V[38] = (V[34] = -(V[ 2] = A[ 9] + A[13] + A[15]) - A[14]) + A[ 9] - A[10] - A[11];
    V[46] = (tmp = -(A[12] + A[14] + A[15])) - A[ 8];
    V[42] = tmp - A[10] - A[11];
    // 9 adds, 9 subs

    A[ 0] = MPC_MULTIPLY_V_PRESCALE((Sample[ 0] - Sample[31]), INVCOS01);
    A[ 1] = MPC_MULTIPLY_V_PRESCALE((Sample[ 1] - Sample[30]), INVCOS03);
    A[ 2] = MPC_MULTIPLY_V_PRESCALE((Sample[ 2] - Sample[29]), INVCOS05);
    A[ 3] = MPC_MULTIPLY_V_PRESCALE((Sample[ 3] - Sample[28]), INVCOS07);
    A[ 4] = MPC_MULTIPLY_V_PRESCALE((Sample[ 4] - Sample[27]), INVCOS09);
    A[ 5] = MPC_MULTIPLY_V_PRESCALE((Sample[ 5] - Sample[26]), INVCOS11);
    A[ 6] = MPC_MULTIPLY_V_PRESCALE((Sample[ 6] - Sample[25]), INVCOS13);
    A[ 7] = MPC_MULTIPLY_V_PRESCALE((Sample[ 7] - Sample[24]), INVCOS15);
    A[ 8] = MPC_MULTIPLY_V_PRESCALE((Sample[ 8] - Sample[23]), INVCOS17);
    A[ 9] = MPC_MULTIPLY_V_PRESCALE((Sample[ 9] - Sample[22]), INVCOS19);
    A[10] = MPC_MULTIPLY_V_PRESCALE((Sample[10] - Sample[21]), INVCOS21);
    A[11] = MPC_MULTIPLY_V_PRESCALE((Sample[11] - Sample[20]), INVCOS23);
    A[12] = MPC_MULTIPLY_V_PRESCALE((Sample[12] - Sample[19]), INVCOS25);
    A[13] = MPC_MULTIPLY_V_PRESCALE((Sample[13] - Sample[18]), INVCOS27);
    A[14] = MPC_MULTIPLY_V_PRESCALE((Sample[14] - Sample[17]), INVCOS29);
    A[15] = MPC_MULTIPLY_V_PRESCALE((Sample[15] - Sample[16]), INVCOS31);
    // 16 subs, 16 muls, 16 shifts

    B[ 0] = A[ 0] + A[15];
    B[ 1] = A[ 1] + A[14];
    B[ 2] = A[ 2] + A[13];
    B[ 3] = A[ 3] + A[12];
    B[ 4] = A[ 4] + A[11];
    B[ 5] = A[ 5] + A[10];
    B[ 6] = A[ 6] + A[ 9];
    B[ 7] = A[ 7] + A[ 8];
    B[ 8] = MPC_MULTIPLY_V((A[ 0] - A[15]), INVCOS02);
    B[ 9] = MPC_MULTIPLY_V((A[ 1] - A[14]), INVCOS06);
    B[10] = MPC_MULTIPLY_V((A[ 2] - A[13]), INVCOS10);
    B[11] = MPC_MULTIPLY_V((A[ 3] - A[12]), INVCOS14);
    B[12] = MPC_MULTIPLY_V((A[ 4] - A[11]), INVCOS18);
    B[13] = MPC_MULTIPLY_V((A[ 5] - A[10]), INVCOS22);
    B[14] = MPC_MULTIPLY_V((A[ 6] - A[ 9]), INVCOS26);
    B[15] = MPC_MULTIPLY_V((A[ 7] - A[ 8]), INVCOS30);
    // 8 adds, 8 subs, 8 muls, 8 shift

    A[ 0] = B[ 0] + B[ 7];
    A[ 1] = B[ 1] + B[ 6];
    A[ 2] = B[ 2] + B[ 5];
    A[ 3] = B[ 3] + B[ 4];
    A[ 4] = MPC_MULTIPLY_V((B[ 0] - B[ 7]), INVCOS04);
    A[ 5] = MPC_MULTIPLY_V((B[ 1] - B[ 6]), INVCOS12);
    A[ 6] = MPC_MULTIPLY_V((B[ 2] - B[ 5]), INVCOS20);
    A[ 7] = MPC_MULTIPLY_V((B[ 3] - B[ 4]), INVCOS28);
    A[ 8] = B[ 8] + B[15];
    A[ 9] = B[ 9] + B[14];
    A[10] = B[10] + B[13];
    A[11] = B[11] + B[12];
    A[12] = MPC_MULTIPLY_V((B[ 8] - B[15]), INVCOS04);
    A[13] = MPC_MULTIPLY_V((B[ 9] - B[14]), INVCOS12);
    A[14] = MPC_MULTIPLY_V((B[10] - B[13]), INVCOS20);
    A[15] = MPC_MULTIPLY_V((B[11] - B[12]), INVCOS28);
    // 8 adds, 8 subs, 8 muls, 8 shift

    B[ 0] = A[ 0] + A[ 3];
    B[ 1] = A[ 1] + A[ 2];
    B[ 2] = MPC_MULTIPLY_V((A[ 0] - A[ 3]), INVCOS08);
    B[ 3] = MPC_MULTIPLY_V((A[ 1] - A[ 2]), INVCOS24);
    B[ 4] = A[ 4] + A[ 7];
    B[ 5] = A[ 5] + A[ 6];
    B[ 6] = MPC_MULTIPLY_V((A[ 4] - A[ 7]), INVCOS08);
    B[ 7] = MPC_MULTIPLY_V((A[ 5] - A[ 6]), INVCOS24);
    B[ 8] = A[ 8] + A[11];
    B[ 9] = A[ 9] + A[10];
    B[10] = MPC_MULTIPLY_V((A[ 8] - A[11]), INVCOS08);
    B[11] = MPC_MULTIPLY_V((A[ 9] - A[10]), INVCOS24);
    B[12] = A[12] + A[15];
    B[13] = A[13] + A[14];
    B[14] = MPC_MULTIPLY_V((A[12] - A[15]), INVCOS08);
    B[15] = MPC_MULTIPLY_V((A[13] - A[14]), INVCOS24);
    // 8 adds, 8 subs, 8 muls, 8 shift

    A[ 0] = MPC_V_POSTSCALE((B[ 0] + B[ 1]));
    A[ 1] = MPC_MULTIPLY_V_POSTSCALE((B[ 0] - B[ 1]), INVCOS16);
    A[ 2] = MPC_V_POSTSCALE((B[ 2] + B[ 3]));
    A[ 3] = MPC_MULTIPLY_V_POSTSCALE((B[ 2] - B[ 3]), INVCOS16);
    A[ 4] = MPC_V_POSTSCALE((B[ 4] + B[ 5]));
    A[ 5] = MPC_MULTIPLY_V_POSTSCALE((B[ 4] - B[ 5]), INVCOS16);
    A[ 6] = MPC_V_POSTSCALE((B[ 6] + B[ 7]));
    A[ 7] = MPC_MULTIPLY_V_POSTSCALE((B[ 6] - B[ 7]), INVCOS16);
    A[ 8] = MPC_V_POSTSCALE((B[ 8] + B[ 9]));
    A[ 9] = MPC_MULTIPLY_V_POSTSCALE((B[ 8] - B[ 9]), INVCOS16);
    A[10] = MPC_V_POSTSCALE((B[10] + B[11]));
    A[11] = MPC_MULTIPLY_V_POSTSCALE((B[10] - B[11]), INVCOS16);
    A[12] = MPC_V_POSTSCALE((B[12] + B[13]));
    A[13] = MPC_MULTIPLY_V_POSTSCALE((B[12] - B[13]), INVCOS16);
    A[14] = MPC_V_POSTSCALE((B[14] + B[15]));
    A[15] = MPC_MULTIPLY_V_POSTSCALE((B[14] - B[15]), INVCOS16);
    // 8 adds, 8 subs, 8 muls, 8 shift

    // multiple used expressions: A[ 4]+A[ 6]+A[ 7], A[ 9]+A[13]+A[15]
    V[ 5] = (V[11] = (V[13] = A[ 7] + (V[15] = A[15])) + A[11]) + A[ 5] + A[13];
    V[ 7] = (V[ 9] = A[ 3] + A[11] + A[15]) + A[13];
    V[33] = -(V[ 1] = A[ 1] + A[ 9] + A[13] + A[15]) - A[14];
    V[35] = -(V[ 3] = A[ 5] + A[ 7] + A[ 9] + A[13] + A[15]) - A[ 6] - A[14];
    V[37] = (tmp = -(A[10] + A[11] + A[13] + A[14] + A[15])) - A[ 5] - A[ 6] - A[ 7];
    V[39] = tmp - A[ 2] - A[ 3];
    V[41] = (tmp += A[13] - A[12]) - A[ 2] - A[ 3];
    V[43] = tmp - A[ 4] - A[ 6] - A[ 7];
    V[47] = (tmp = -(A[ 8] + A[12] + A[14] + A[15])) - A[ 0];
    V[45] = tmp - A[ 4] - A[ 6] - A[ 7];
    // 22 adds, 18 subs

    V[32] = -(V[ 0] = MPC_V_PRESHIFT(V[ 0]));
    V[31] = -(V[ 1] = MPC_V_PRESHIFT(V[ 1]));
    V[30] = -(V[ 2] = MPC_V_PRESHIFT(V[ 2]));
    V[29] = -(V[ 3] = MPC_V_PRESHIFT(V[ 3]));
    V[28] = -(V[ 4] = MPC_V_PRESHIFT(V[ 4]));
    V[27] = -(V[ 5] = MPC_V_PRESHIFT(V[ 5]));
    V[26] = -(V[ 6] = MPC_V_PRESHIFT(V[ 6]));
    V[25] = -(V[ 7] = MPC_V_PRESHIFT(V[ 7]));
    V[24] = -(V[ 8] = MPC_V_PRESHIFT(V[ 8]));
    V[23] = -(V[ 9] = MPC_V_PRESHIFT(V[ 9]));
    V[22] = -(V[10] = MPC_V_PRESHIFT(V[10]));
    V[21] = -(V[11] = MPC_V_PRESHIFT(V[11]));
    V[20] = -(V[12] = MPC_V_PRESHIFT(V[12]));
    V[19] = -(V[13] = MPC_V_PRESHIFT(V[13]));
    V[18] = -(V[14] = MPC_V_PRESHIFT(V[14]));
    V[17] = -(V[15] = MPC_V_PRESHIFT(V[15]));
    // 16 adds, 16 shifts (OPTIMIZE_FOR_SPEED only)

    V[63] =  (V[33] = MPC_V_PRESHIFT(V[33]));
    V[62] =  (V[34] = MPC_V_PRESHIFT(V[34]));
    V[61] =  (V[35] = MPC_V_PRESHIFT(V[35]));
    V[60] =  (V[36] = MPC_V_PRESHIFT(V[36]));
    V[59] =  (V[37] = MPC_V_PRESHIFT(V[37]));
    V[58] =  (V[38] = MPC_V_PRESHIFT(V[38]));
    V[57] =  (V[39] = MPC_V_PRESHIFT(V[39]));
    V[56] =  (V[40] = MPC_V_PRESHIFT(V[40]));
    V[55] =  (V[41] = MPC_V_PRESHIFT(V[41]));
    V[54] =  (V[42] = MPC_V_PRESHIFT(V[42]));
    V[53] =  (V[43] = MPC_V_PRESHIFT(V[43]));
    V[52] =  (V[44] = MPC_V_PRESHIFT(V[44]));
    V[51] =  (V[45] = MPC_V_PRESHIFT(V[45]));
    V[50] =  (V[46] = MPC_V_PRESHIFT(V[46]));
    V[49] =  (V[47] = MPC_V_PRESHIFT(V[47]));
    V[48] =  (V[48] = MPC_V_PRESHIFT(V[48]));
    // 16 adds, 16 shifts (OPTIMIZE_FOR_SPEED only)
    
    // OPTIMIZE_FOR_SPEED total: 143 adds, 107 subs, 80 muls, 112 shifts
    //                    total: 111 adds, 107 subs, 80 muls,  80 shifts
}

#if defined(CPU_ARM)
extern void
mpc_decoder_windowing_D(MPC_SAMPLE_FORMAT * Data, 
                        const MPC_SAMPLE_FORMAT * V,
                        const MPC_SAMPLE_FORMAT * D);
#else
static void 
mpc_decoder_windowing_D(MPC_SAMPLE_FORMAT * Data, 
                        const MPC_SAMPLE_FORMAT * V,
                        const MPC_SAMPLE_FORMAT * D)
{
   mpc_int32_t k;
    
   #if defined(OPTIMIZE_FOR_SPEED)
      // 32=32x32-multiply (FIXED_POINT)
      for ( k = 0; k < 32; k++, D += 16, V++ ) 
      {
         *Data = V[  0]*D[ 0] + V[ 96]*D[ 1] + V[128]*D[ 2] + V[224]*D[ 3]
               + V[256]*D[ 4] + V[352]*D[ 5] + V[384]*D[ 6] + V[480]*D[ 7]
               + V[512]*D[ 8] + V[608]*D[ 9] + V[640]*D[10] + V[736]*D[11]
               + V[768]*D[12] + V[864]*D[13] + V[896]*D[14] + V[992]*D[15];
         *Data >>= 1; // post shift to compensate for pre-shifting
         Data += 1;
         // total: 32 * (16 muls, 15 adds)
      }
   #else
      #if defined(CPU_COLDFIRE)
         // 64=32x32-multiply assembler for Coldfire
         for ( k = 0; k < 32; k++, D += 16, V++ ) 
         {
            asm volatile (
               "movem.l (%[D]), %%d0-%%d3                    \n\t"
               "move.l (%[V]), %%a5                          \n\t"
               "mac.l %%d0, %%a5, (96*4, %[V]), %%a5, %%acc0 \n\t"
               "mac.l %%d1, %%a5, (128*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d2, %%a5, (224*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d3, %%a5, (256*4, %[V]), %%a5, %%acc0\n\t"
               "movem.l (4*4, %[D]), %%d0-%%d3               \n\t"
               "mac.l %%d0, %%a5, (352*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d1, %%a5, (384*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d2, %%a5, (480*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d3, %%a5, (512*4, %[V]), %%a5, %%acc0\n\t"
               "movem.l (8*4, %[D]), %%d0-%%d3               \n\t"
               "mac.l %%d0, %%a5, (608*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d1, %%a5, (640*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d2, %%a5, (736*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d3, %%a5, (768*4, %[V]), %%a5, %%acc0\n\t"
               "movem.l (12*4, %[D]), %%d0-%%d3              \n\t"
               "mac.l %%d0, %%a5, (864*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d1, %%a5, (896*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d2, %%a5, (992*4, %[V]), %%a5, %%acc0\n\t"
               "mac.l %%d3, %%a5, %%acc0                     \n\t"
               "movclr.l %%acc0, %%d0                        \n\t"
               "lsl.l #1, %%d0                               \n\t"
               "move.l %%d0, (%[Data])+                      \n"
               : [Data] "+a" (Data)
               : [V] "a" (V), [D] "a" (D)
               : "d0", "d1", "d2", "d3", "a5");
         }
      #else
         // 64=64x64-multiply (FIXED_POINT) or float=float*float (!FIXED_POINT) in C
         for ( k = 0; k < 32; k++, D += 16, V++ )
         {
            *Data = MPC_MULTIPLY_EX(V[  0],D[ 0],30) + MPC_MULTIPLY_EX(V[ 96],D[ 1],30)
                  + MPC_MULTIPLY_EX(V[128],D[ 2],30) + MPC_MULTIPLY_EX(V[224],D[ 3],30)
                  + MPC_MULTIPLY_EX(V[256],D[ 4],30) + MPC_MULTIPLY_EX(V[352],D[ 5],30)
                  + MPC_MULTIPLY_EX(V[384],D[ 6],30) + MPC_MULTIPLY_EX(V[480],D[ 7],30)
                  + MPC_MULTIPLY_EX(V[512],D[ 8],30) + MPC_MULTIPLY_EX(V[608],D[ 9],30)
                  + MPC_MULTIPLY_EX(V[640],D[10],30) + MPC_MULTIPLY_EX(V[736],D[11],30)
                  + MPC_MULTIPLY_EX(V[768],D[12],30) + MPC_MULTIPLY_EX(V[864],D[13],30)
                  + MPC_MULTIPLY_EX(V[896],D[14],30) + MPC_MULTIPLY_EX(V[992],D[15],30);
            Data += 1;
            // total: 16 muls, 15 adds, 16 shifts
         }
      #endif
   #endif
}
#endif /* CPU_ARM */

static void 
mpc_full_synthesis_filter(MPC_SAMPLE_FORMAT *OutData, MPC_SAMPLE_FORMAT *V, const MPC_SAMPLE_FORMAT *Y)
{
    mpc_uint32_t n;
    
    if (NULL != OutData)
    {    
        for ( n = 0; n < 36; n++, Y += 32, OutData += 32 ) 
        {
            V -= 64;
            mpc_calculate_new_V ( Y, V );
            mpc_decoder_windowing_D( OutData, V, Di_opt );
        }
     }
}

void
mpc_decoder_synthese_filter_float(mpc_decoder *d, MPC_SAMPLE_FORMAT *OutData) 
{
    /********* left channel ********/
    memmove(d->V_L + MPC_V_MEM, d->V_L, 960 * sizeof(MPC_SAMPLE_FORMAT) );

    mpc_full_synthesis_filter(
        OutData,
        (MPC_SAMPLE_FORMAT *)(d->V_L + MPC_V_MEM),
        (MPC_SAMPLE_FORMAT *)(d->Y_L [0]));

    /******** right channel ********/
    memmove(d->V_R + MPC_V_MEM, d->V_R, 960 * sizeof(MPC_SAMPLE_FORMAT) );

    mpc_full_synthesis_filter(
        (OutData == NULL ? NULL : OutData + MPC_FRAME_LENGTH),
        (MPC_SAMPLE_FORMAT *)(d->V_R + MPC_V_MEM),
        (MPC_SAMPLE_FORMAT *)(d->Y_R [0]));
}

/*******************************************/
/*                                         */
/*            dithered synthesis           */
/*                                         */
/*******************************************/

static const unsigned char Parity [256] ICONST_ATTR = {  // parity
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

/*
 *  This is a simple random number generator with good quality for audio purposes.
 *  It consists of two polycounters with opposite rotation direction and different
 *  periods. The periods are coprime, so the total period is the product of both.
 *
 *     -------------------------------------------------------------------------------------------------
 * +-> |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0|
 * |   -------------------------------------------------------------------------------------------------
 * |                                                                          |  |  |  |     |        |
 * |                                                                          +--+--+--+-XOR-+--------+
 * |                                                                                      |
 * +--------------------------------------------------------------------------------------+
 *
 *     -------------------------------------------------------------------------------------------------
 *     |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0| <-+
 *     -------------------------------------------------------------------------------------------------   |
 *       |  |           |  |                                                                               |
 *       +--+----XOR----+--+                                                                               |
 *                |                                                                                        |
 *                +----------------------------------------------------------------------------------------+
 *
 *
 *  The first has an period of 3*5*17*257*65537, the second of 7*47*73*178481,
 *  which gives a period of 18.410.713.077.675.721.215. The result is the
 *  XORed values of both generators.
 */
mpc_uint32_t
mpc_random_int(mpc_decoder *d) 
{
#if 1
    mpc_uint32_t  t1, t2, t3, t4;

    t3   = t1 = d->__r1;   t4   = t2 = d->__r2;  // Parity calculation is done via table lookup, this is also available
    t1  &= 0xF5;        t2 >>= 25;               // on CPUs without parity, can be implemented in C and avoid unpredictable
    t1   = Parity [t1]; t2  &= 0x63;             // jumps and slow rotate through the carry flag operations.
    t1 <<= 31;          t2   = Parity [t2];

    return (d->__r1 = (t3 >> 1) | t1 ) ^ (d->__r2 = (t4 + t4) | t2 );
#else
    return (d->__r1 = (d->__r1 >> 1) | ((mpc_uint32_t)Parity [d->__r1 & 0xF5] << 31) ) ^
        (d->__r2 = (d->__r2 << 1) |  (mpc_uint32_t)Parity [(d->__r2 >> 25) & 0x63] );
#endif
}
