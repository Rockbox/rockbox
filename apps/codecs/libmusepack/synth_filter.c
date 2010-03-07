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
#include <string.h>
#include "mpcdec.h"
#include "decoder.h"
#include "mpcdec_math.h"

/* C O N S T A N T S */
#undef _

#if defined(MPC_FIXED_POINT)
    #if defined(CPU_ARM)
      // do not up-scale D-values to achieve higher speed in smull/mlal
      // operations. saves ~14/8 = 1.75 cycles per multiplication
      #define D(value)  (value)
      
      // in this configuration a post-shift by >>16 is needed after synthesis
    #else
      // saturate to +/- 2^31 (= value << (31-17)), D-values are +/- 2^17
      #define D(value)  (value << (14))
    #endif
#else
   // IMPORTANT: internal scaling is somehow strange for floating point, therefore we scale the coefficients Di_opt
   // by the correct amount to have proper scaled output
   #define D(value)  MAKE_MPC_SAMPLE((double)value*(double)(0x1000))
#endif
    
// Di_opt coefficients are +/- 2^17 (pre-shifted by <<16)
static const MPC_SAMPLE_FORMAT  Di_opt [512] ICONST_ATTR = {
/*           0        1        2         3         4         5          6          7         8         9       10        11       12       13      14     15  */
/*  0 */  D( 0), -D( 29),  D(213), -D( 459),  D(2037), -D(5153),  D( 6574), -D(37489), D(75038),  D(37489), D(6574),  D(5153), D(2037),  D(459), D(213), D(29),
/*  1 */ -D( 1), -D( 31),  D(218), -D( 519),  D(2000), -D(5517),  D( 5959), -D(39336), D(74992),  D(35640), D(7134),  D(4788), D(2063),  D(401), D(208), D(26),
/*  2 */ -D( 1), -D( 35),  D(222), -D( 581),  D(1952), -D(5879),  D( 5288), -D(41176), D(74856),  D(33791), D(7640),  D(4425), D(2080),  D(347), D(202), D(24),
/*  3 */ -D( 1), -D( 38),  D(225), -D( 645),  D(1893), -D(6237),  D( 4561), -D(43006), D(74630),  D(31947), D(8092),  D(4063), D(2087),  D(294), D(196), D(21),
/*  4 */ -D( 1), -D( 41),  D(227), -D( 711),  D(1822), -D(6589),  D( 3776), -D(44821), D(74313),  D(30112), D(8492),  D(3705), D(2085),  D(244), D(190), D(19),
/*  5 */ -D( 1), -D( 45),  D(228), -D( 779),  D(1739), -D(6935),  D( 2935), -D(46617), D(73908),  D(28289), D(8840),  D(3351), D(2075),  D(197), D(183), D(17),
/*  6 */ -D( 1), -D( 49),  D(228), -D( 848),  D(1644), -D(7271),  D( 2037), -D(48390), D(73415),  D(26482), D(9139),  D(3004), D(2057),  D(153), D(176), D(16),
/*  7 */ -D( 2), -D( 53),  D(227), -D( 919),  D(1535), -D(7597),  D( 1082), -D(50137), D(72835),  D(24694), D(9389),  D(2663), D(2032),  D(111), D(169), D(14),
/*  8 */ -D( 2), -D( 58),  D(224), -D( 991),  D(1414), -D(7910),  D(   70), -D(51853), D(72169),  D(22929), D(9592),  D(2330), D(2001),  D( 72), D(161), D(13),
/*  9 */ -D( 2), -D( 63),  D(221), -D(1064),  D(1280), -D(8209), -D(  998), -D(53534), D(71420),  D(21189), D(9750),  D(2006), D(1962),  D( 36), D(154), D(11),
/* 10 */ -D( 2), -D( 68),  D(215), -D(1137),  D(1131), -D(8491), -D( 2122), -D(55178), D(70590),  D(19478), D(9863),  D(1692), D(1919),  D(  2), D(147), D(10),
/* 11 */ -D( 3), -D( 73),  D(208), -D(1210),  D( 970), -D(8755), -D( 3300), -D(56778), D(69679),  D(17799), D(9935),  D(1388), D(1870), -D( 29), D(139), D( 9),
/* 12 */ -D( 3), -D( 79),  D(200), -D(1283),  D( 794), -D(8998), -D( 4533), -D(58333), D(68692),  D(16155), D(9966),  D(1095), D(1817), -D( 57), D(132), D( 8),
/* 13 */ -D( 4), -D( 85),  D(189), -D(1356),  D( 605), -D(9219), -D( 5818), -D(59838), D(67629),  D(14548), D(9959),  D( 814), D(1759), -D( 83), D(125), D( 7),
/* 14 */ -D( 4), -D( 91),  D(177), -D(1428),  D( 402), -D(9416), -D( 7154), -D(61289), D(66494),  D(12980), D(9916),  D( 545), D(1698), -D(106), D(117), D( 7),
/* 15 */ -D( 5), -D( 97),  D(163), -D(1498),  D( 185), -D(9585), -D( 8540), -D(62684), D(65290),  D(11455), D(9838),  D( 288), D(1634), -D(127), D(111), D( 6),
/* 16 */ -D( 5), -D(104),  D(146), -D(1567), -D(  45), -D(9727), -D( 9975), -D(64019), D(64019),  D( 9975), D(9727),  D(  45), D(1567), -D(146), D(104), D( 5),
/* 17 */ -D( 6), -D(111),  D(127), -D(1634), -D( 288), -D(9838), -D(11455), -D(65290), D(62684),  D( 8540), D(9585), -D( 185), D(1498), -D(163), D( 97), D( 5),
/* 18 */ -D( 7), -D(117),  D(106), -D(1698), -D( 545), -D(9916), -D(12980), -D(66494), D(61289),  D( 7154), D(9416), -D( 402), D(1428), -D(177), D( 91), D( 4),
/* 19 */ -D( 7), -D(125),  D( 83), -D(1759), -D( 814), -D(9959), -D(14548), -D(67629), D(59838),  D( 5818), D(9219), -D( 605), D(1356), -D(189), D( 85), D( 4),
/* 20 */ -D( 8), -D(132),  D( 57), -D(1817), -D(1095), -D(9966), -D(16155), -D(68692), D(58333),  D( 4533), D(8998), -D( 794), D(1283), -D(200), D( 79), D( 3),
/* 21 */ -D( 9), -D(139),  D( 29), -D(1870), -D(1388), -D(9935), -D(17799), -D(69679), D(56778),  D( 3300), D(8755), -D( 970), D(1210), -D(208), D( 73), D( 3),
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

// DCT32-coefficients were expanded (<<) by DCT32_COEFFICIENT_EXPAND
#define DCT32_COEFFICIENT_EXPAND 31

#if defined(MPC_FIXED_POINT)
   // define 64=32x32-multiplication for DCT-coefficients with samples. Via usage of MPC_FRACT highly optimized assembler might be used
   // MULTIPLY_FRACT will perform >>32 after multiplication, as coef were expanded by DCT32_COEFFICIENT_EXPAND we'll correct this on the result.
   // Will loose 4 bit accuracy on result in fract part without effect on final audio result
   #define MPC_DCT32_MUL(sample, coef)  (MPC_MULTIPLY_FRACT(sample,coef) << (32-DCT32_COEFFICIENT_EXPAND))
   #define MPC_DCT32_SHIFT(sample)      (sample)
#else
   // for floating point use the standard multiplication macro
   #define MPC_DCT32_MUL(sample, coef)  (MPC_MULTIPLY(sample, coef) )
   #define MPC_DCT32_SHIFT(sample)      (sample)
#endif

/******************************************************************************
 * mpc_dct32(const int *in, int *out)
 *
 * mpc_dct32 is a dct32 with in[32]->dct[32] that contains the mirroring from
 * dct[32] to the expected out[64]. The symmetry is 
 * out[16] = 0, 
 * out[ 0..15] =  dct[ 0..15], 
 * out[32..17] = -dct[ 0..15], 
 * out[33..48] = -dct[16..31],
 * out[63..48] = -dct[16..31].
 * The cos-tab has the format s0.31.
 *****************************************************************************/
void 
mpc_dct32(const MPC_SAMPLE_FORMAT *in, MPC_SAMPLE_FORMAT *v)
ICODE_ATTR_MPC_LARGE_IRAM;

void
mpc_dct32(const MPC_SAMPLE_FORMAT *in, MPC_SAMPLE_FORMAT *v)
{
  MPC_SAMPLE_FORMAT t0,   t1,   t2,   t3,   t4,   t5,   t6,   t7;
  MPC_SAMPLE_FORMAT t8,   t9,   t10,  t11,  t12,  t13,  t14,  t15;
  MPC_SAMPLE_FORMAT t16,  t17,  t18,  t19,  t20,  t21,  t22,  t23;
  MPC_SAMPLE_FORMAT t24,  t25,  t26,  t27,  t28,  t29,  t30,  t31;
  MPC_SAMPLE_FORMAT t32,  t33,  t34,  t35,  t36,  t37,  t38,  t39;
  MPC_SAMPLE_FORMAT t40,  t41,  t42,  t43,  t44,  t45,  t46,  t47;
  MPC_SAMPLE_FORMAT t48,  t49,  t50,  t51,  t52,  t53,  t54,  t55;
  MPC_SAMPLE_FORMAT t56,  t57,  t58,  t59,  t60,  t61,  t62,  t63;
  MPC_SAMPLE_FORMAT t64,  t65,  t66,  t67,  t68,  t69,  t70,  t71;
  MPC_SAMPLE_FORMAT t72,  t73,  t74,  t75,  t76,  t77,  t78,  t79;
  MPC_SAMPLE_FORMAT t80,  t81,  t82,  t83,  t84,  t85,  t86,  t87;
  MPC_SAMPLE_FORMAT t88,  t89,  t90,  t91,  t92,  t93,  t94,  t95;
  MPC_SAMPLE_FORMAT t96,  t97,  t98,  t99,  t100, t101, t102, t103;
  MPC_SAMPLE_FORMAT t104, t105, t106, t107, t108, t109, t110, t111;
  MPC_SAMPLE_FORMAT t112, t113, t114, t115, t116, t117, t118, t119;
  MPC_SAMPLE_FORMAT t120, t121, t122, t123, t124, t125, t126, t127;
  MPC_SAMPLE_FORMAT t128, t129, t130, t131, t132, t133, t134, t135;
  MPC_SAMPLE_FORMAT t136, t137, t138, t139, t140, t141, t142, t143;
  MPC_SAMPLE_FORMAT t144, t145, t146, t147, t148, t149, t150, t151;
  MPC_SAMPLE_FORMAT t152, t153, t154, t155, t156, t157, t158, t159;
  MPC_SAMPLE_FORMAT t160, t161, t162, t163, t164, t165, t166, t167;
  MPC_SAMPLE_FORMAT t168, t169, t170, t171, t172, t173, t174, t175;
  MPC_SAMPLE_FORMAT t176;

    /* costab[i] = cos(PI / (2 * 32) * i) */
#define costab01 (0x7fd8878e)  /* 0.998795456 */
#define costab02 (0x7f62368f)  /* 0.995184727 */
#define costab03 (0x7e9d55fc)  /* 0.989176510 */
#define costab04 (0x7d8a5f40)  /* 0.980785280 */
#define costab05 (0x7c29fbee)  /* 0.970031253 */
#define costab06 (0x7a7d055b)  /* 0.956940336 */
#define costab07 (0x78848414)  /* 0.941544065 */
#define costab08 (0x7641af3d)  /* 0.923879533 */
#define costab09 (0x73b5ebd1)  /* 0.903989293 */
#define costab10 (0x70e2cbc6)  /* 0.881921264 */
#define costab11 (0x6dca0d14)  /* 0.857728610 */
#define costab12 (0x6a6d98a4)  /* 0.831469612 */
#define costab13 (0x66cf8120)  /* 0.803207531 */
#define costab14 (0x62f201ac)  /* 0.773010453 */
#define costab15 (0x5ed77c8a)  /* 0.740951125 */
#define costab16 (0x5a82799a)  /* 0.707106781 */
#define costab17 (0x55f5a4d2)  /* 0.671558955 */
#define costab18 (0x5133cc94)  /* 0.634393284 */
#define costab19 (0x4c3fdff4)  /* 0.595699304 */
#define costab20 (0x471cece7)  /* 0.555570233 */
#define costab21 (0x41ce1e65)  /* 0.514102744 */
#define costab22 (0x3c56ba70)  /* 0.471396737 */
#define costab23 (0x36ba2014)  /* 0.427555093 */
#define costab24 (0x30fbc54d)  /* 0.382683432 */
#define costab25 (0x2b1f34eb)  /* 0.336889853 */
#define costab26 (0x25280c5e)  /* 0.290284677 */
#define costab27 (0x1f19f97b)  /* 0.242980180 */
#define costab28 (0x18f8b83c)  /* 0.195090322 */
#define costab29 (0x12c8106f)  /* 0.146730474 */
#define costab30 (0x0c8bd35e)  /* 0.098017140 */
#define costab31 (0x0647d97c)  /* 0.049067674 */

  t0   = in[ 0] + in[31];  t16  = MPC_DCT32_MUL(in[ 0] - in[31], costab01);
  t1   = in[15] + in[16];  t17  = MPC_DCT32_MUL(in[15] - in[16], costab31);

  t41  = t16 + t17;
  t59  = MPC_DCT32_MUL(t16 - t17, costab02);
  t33  = t0  + t1;
  t50  = MPC_DCT32_MUL(t0  - t1,  costab02);

  t2   = in[ 7] + in[24];  t18  = MPC_DCT32_MUL(in[ 7] - in[24], costab15);
  t3   = in[ 8] + in[23];  t19  = MPC_DCT32_MUL(in[ 8] - in[23], costab17);

  t42  = t18 + t19;
  t60  = MPC_DCT32_MUL(t18 - t19, costab30);
  t34  = t2  + t3;
  t51  = MPC_DCT32_MUL(t2  - t3,  costab30);

  t4   = in[ 3] + in[28];  t20  = MPC_DCT32_MUL(in[ 3] - in[28], costab07);
  t5   = in[12] + in[19];  t21  = MPC_DCT32_MUL(in[12] - in[19], costab25);

  t43  = t20 + t21;
  t61  = MPC_DCT32_MUL(t20 - t21, costab14);
  t35  = t4  + t5;
  t52  = MPC_DCT32_MUL(t4  - t5,  costab14);

  t6   = in[ 4] + in[27];  t22  = MPC_DCT32_MUL(in[ 4] - in[27], costab09);
  t7   = in[11] + in[20];  t23  = MPC_DCT32_MUL(in[11] - in[20], costab23);

  t44  = t22 + t23;
  t62  = MPC_DCT32_MUL(t22 - t23, costab18);
  t36  = t6  + t7;
  t53  = MPC_DCT32_MUL(t6  - t7,  costab18);

  t8   = in[ 1] + in[30];  t24  = MPC_DCT32_MUL(in[ 1] - in[30], costab03);
  t9   = in[14] + in[17];  t25  = MPC_DCT32_MUL(in[14] - in[17], costab29);

  t45  = t24 + t25;
  t63  = MPC_DCT32_MUL(t24 - t25, costab06);
  t37  = t8  + t9;
  t54  = MPC_DCT32_MUL(t8  - t9,  costab06);

  t10  = in[ 6] + in[25];  t26  = MPC_DCT32_MUL(in[ 6] - in[25], costab13);
  t11  = in[ 9] + in[22];  t27  = MPC_DCT32_MUL(in[ 9] - in[22], costab19);

  t46  = t26 + t27;
  t64  = MPC_DCT32_MUL(t26 - t27, costab26);
  t38  = t10 + t11;
  t55  = MPC_DCT32_MUL(t10 - t11, costab26);

  t12  = in[ 2] + in[29];  t28  = MPC_DCT32_MUL(in[ 2] - in[29], costab05);
  t13  = in[13] + in[18];  t29  = MPC_DCT32_MUL(in[13] - in[18], costab27);

  t47  = t28 + t29;
  t65  = MPC_DCT32_MUL(t28 - t29, costab10);
  t39  = t12 + t13;
  t56  = MPC_DCT32_MUL(t12 - t13, costab10);

  t14  = in[ 5] + in[26];  t30  = MPC_DCT32_MUL(in[ 5] - in[26], costab11);
  t15  = in[10] + in[21];  t31  = MPC_DCT32_MUL(in[10] - in[21], costab21);

  t48  = t30 + t31;
  t66  = MPC_DCT32_MUL(t30 - t31, costab22);
  t40  = t14 + t15;
  t57  = MPC_DCT32_MUL(t14 - t15, costab22);

  t69  = t33 + t34;  t89  = MPC_DCT32_MUL(t33 - t34, costab04);
  t70  = t35 + t36;  t90  = MPC_DCT32_MUL(t35 - t36, costab28);
  t71  = t37 + t38;  t91  = MPC_DCT32_MUL(t37 - t38, costab12);
  t72  = t39 + t40;  t92  = MPC_DCT32_MUL(t39 - t40, costab20);
  t73  = t41 + t42;  t94  = MPC_DCT32_MUL(t41 - t42, costab04);
  t74  = t43 + t44;  t95  = MPC_DCT32_MUL(t43 - t44, costab28);
  t75  = t45 + t46;  t96  = MPC_DCT32_MUL(t45 - t46, costab12);
  t76  = t47 + t48;  t97  = MPC_DCT32_MUL(t47 - t48, costab20);

  t78  = t50 + t51;  t100 = MPC_DCT32_MUL(t50 - t51, costab04);
  t79  = t52 + t53;  t101 = MPC_DCT32_MUL(t52 - t53, costab28);
  t80  = t54 + t55;  t102 = MPC_DCT32_MUL(t54 - t55, costab12);
  t81  = t56 + t57;  t103 = MPC_DCT32_MUL(t56 - t57, costab20);

  t83  = t59 + t60;  t106 = MPC_DCT32_MUL(t59 - t60, costab04);
  t84  = t61 + t62;  t107 = MPC_DCT32_MUL(t61 - t62, costab28);
  t85  = t63 + t64;  t108 = MPC_DCT32_MUL(t63 - t64, costab12);
  t86  = t65 + t66;  t109 = MPC_DCT32_MUL(t65 - t66, costab20);

  t113 = t69  + t70;
  t114 = t71  + t72;

  /*  0 */ v[48] = -MPC_DCT32_SHIFT(t113 + t114);
  /* 16 */ v[32] = -(v[ 0] = MPC_DCT32_SHIFT(MPC_DCT32_MUL(t113 - t114, costab16)));

  t115 = t73  + t74;
  t116 = t75  + t76;

  t32  = t115 + t116;

  /*  1 */ v[49] = v[47] = -MPC_DCT32_SHIFT(t32);

  t118 = t78  + t79;
  t119 = t80  + t81;

  t58  = t118 + t119;

  /*  2 */ v[50] = v[46] = -MPC_DCT32_SHIFT(t58);

  t121 = t83  + t84;
  t122 = t85  + t86;

  t67  = t121 + t122;

  t49  = (t67 * 2) - t32;

  /*  3 */ v[51] = v[45] = -MPC_DCT32_SHIFT(t49);

  t125 = t89  + t90;
  t126 = t91  + t92;

  t93  = t125 + t126;

  /*  4 */ v[52] = v[44] = -MPC_DCT32_SHIFT(t93);

  t128 = t94  + t95;
  t129 = t96  + t97;

  t98  = t128 + t129;

  t68  = (t98 * 2) - t49;

  /*  5 */ v[53] = v[43] = -MPC_DCT32_SHIFT(t68);

  t132 = t100 + t101;
  t133 = t102 + t103;

  t104 = t132 + t133;

  t82  = (t104 * 2) - t58;

  /*  6 */ v[54] = v[42] = -MPC_DCT32_SHIFT(t82);

  t136 = t106 + t107;
  t137 = t108 + t109;

  t110 = t136 + t137;

  t87  = (t110 * 2) - t67;

  t77  = (t87 * 2) - t68;

  /*  7 */ v[55] = v[41] = -MPC_DCT32_SHIFT(t77);

  t141 = MPC_DCT32_MUL(t69 - t70, costab08);
  t142 = MPC_DCT32_MUL(t71 - t72, costab24);
  t143 = t141 + t142;

  /*  8 */ v[56] = v[40] = -MPC_DCT32_SHIFT(t143);
  /* 24 */ v[24] = -(v[ 8] = MPC_DCT32_SHIFT((MPC_DCT32_MUL(t141 - t142, costab16) * 2) - t143));

  t144 = MPC_DCT32_MUL(t73 - t74, costab08);
  t145 = MPC_DCT32_MUL(t75 - t76, costab24);
  t146 = t144 + t145;

  t88  = (t146 * 2) - t77;

  /*  9 */ v[57] = v[39] = -MPC_DCT32_SHIFT(t88);

  t148 = MPC_DCT32_MUL(t78 - t79, costab08);
  t149 = MPC_DCT32_MUL(t80 - t81, costab24);
  t150 = t148 + t149;

  t105 = (t150 * 2) - t82;

  /* 10 */ v[58] = v[38] = -MPC_DCT32_SHIFT(t105);

  t152 = MPC_DCT32_MUL(t83 - t84, costab08);
  t153 = MPC_DCT32_MUL(t85 - t86, costab24);
  t154 = t152 + t153;

  t111 = (t154 * 2) - t87;

  t99  = (t111 * 2) - t88;

  /* 11 */ v[59] = v[37] = -MPC_DCT32_SHIFT(t99);

  t157 = MPC_DCT32_MUL(t89 - t90, costab08);
  t158 = MPC_DCT32_MUL(t91 - t92, costab24);
  t159 = t157 + t158;

  t127 = (t159 * 2) - t93;

  /* 12 */ v[60] = v[36] = -MPC_DCT32_SHIFT(t127);

  t160 = (MPC_DCT32_MUL(t125 - t126, costab16) * 2) - t127;

  /* 20 */ v[28] = -(v[ 4] = MPC_DCT32_SHIFT(t160));
  /* 28 */ v[20] = -(v[12] = MPC_DCT32_SHIFT((((MPC_DCT32_MUL(t157 - t158, costab16) * 2) - t159) * 2) - t160));

  t161 = MPC_DCT32_MUL(t94 - t95, costab08);
  t162 = MPC_DCT32_MUL(t96 - t97, costab24);
  t163 = t161 + t162;

  t130 = (t163 * 2) - t98;

  t112 = (t130 * 2) - t99;

  /* 13 */ v[61] = v[35] = -MPC_DCT32_SHIFT(t112);

  t164 = (MPC_DCT32_MUL(t128 - t129, costab16) * 2) - t130;

  t166 = MPC_DCT32_MUL(t100 - t101, costab08);
  t167 = MPC_DCT32_MUL(t102 - t103, costab24);
  t168 = t166 + t167;

  t134 = (t168 * 2) - t104;

  t120 = (t134 * 2) - t105;

  /* 14 */ v[62] = v[34] = -MPC_DCT32_SHIFT(t120);

  t135 = (MPC_DCT32_MUL(t118 - t119, costab16) * 2) - t120;

  /* 18 */ v[30] = -(v[ 2] = MPC_DCT32_SHIFT(t135));

  t169 = (MPC_DCT32_MUL(t132 - t133, costab16) * 2) - t134;

  t151 = (t169 * 2) - t135;

  /* 22 */ v[26] = -(v[ 6] = MPC_DCT32_SHIFT(t151));

  t170 = (((MPC_DCT32_MUL(t148 - t149, costab16) * 2) - t150) * 2) - t151;

  /* 26 */ v[22] = -(v[10] = MPC_DCT32_SHIFT(t170));
  /* 30 */ v[18] = -(v[14] = MPC_DCT32_SHIFT((((((MPC_DCT32_MUL(t166 - t167, costab16) * 2) - t168) * 2) - t169) * 2) - t170));

  t171 = MPC_DCT32_MUL(t106 - t107, costab08);
  t172 = MPC_DCT32_MUL(t108 - t109, costab24);
  t173 = t171 + t172;

  t138 = (t173 * 2) - t110;

  t123 = (t138 * 2) - t111;

  t139 = (MPC_DCT32_MUL(t121 - t122, costab16) * 2) - t123;

  t117 = (t123 * 2) - t112;

  /* 15 */ v[63] = v[33] =-MPC_DCT32_SHIFT(t117);

  t124 = (MPC_DCT32_MUL(t115 - t116, costab16) * 2) - t117;

  /* 17 */ v[31] = -(v[ 1] = MPC_DCT32_SHIFT(t124));

  t131 = (t139 * 2) - t124;

  /* 19 */ v[29] = -(v[ 3] = MPC_DCT32_SHIFT(t131));

  t140 = (t164 * 2) - t131;

  /* 21 */ v[27] = -(v[ 5] = MPC_DCT32_SHIFT(t140));

  t174 = (MPC_DCT32_MUL(t136 - t137, costab16) * 2) - t138;

  t155 = (t174 * 2) - t139;

  t147 = (t155 * 2) - t140;

  /* 23 */ v[25] = -(v[ 7] = MPC_DCT32_SHIFT(t147));

  t156 = (((MPC_DCT32_MUL(t144 - t145, costab16) * 2) - t146) * 2) - t147;

  /* 25 */ v[23] = -(v[ 9] = MPC_DCT32_SHIFT(t156));

  t175 = (((MPC_DCT32_MUL(t152 - t153, costab16) * 2) - t154) * 2) - t155;

  t165 = (t175 * 2) - t156;

  /* 27 */ v[21] = -(v[11] = MPC_DCT32_SHIFT(t165));

  t176 = (((((MPC_DCT32_MUL(t161 - t162, costab16) * 2) - t163) * 2) - t164) * 2) - t165;

  /* 29 */ v[19] = -(v[13] = MPC_DCT32_SHIFT(t176));
  /* 31 */ v[17] = -(v[15] = MPC_DCT32_SHIFT((((((((MPC_DCT32_MUL(t171 - t172, costab16) * 2) - t173) * 2) - t174) * 2) - t175) * 2) - t176));
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
#endif /* COLDFIRE */
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
            mpc_dct32(Y, V);
            mpc_decoder_windowing_D( OutData, V, Di_opt );
        }
    }
}

void
mpc_decoder_synthese_filter_float(mpc_decoder *d, MPC_SAMPLE_FORMAT *OutData,
                                  int num_channels) 
{
    (void)num_channels;
    
    /********* left channel ********/
    memmove(d->V_L + MPC_V_MEM, d->V_L, 960 * sizeof(MPC_SAMPLE_FORMAT) );
    mpc_full_synthesis_filter(OutData,
                              (MPC_SAMPLE_FORMAT *)(d->V_L + MPC_V_MEM),
                              (MPC_SAMPLE_FORMAT *)(d->Y_L));

    /******** right channel ********/
    memmove(d->V_R + MPC_V_MEM, d->V_R, 960 * sizeof(MPC_SAMPLE_FORMAT) );
    mpc_full_synthesis_filter((OutData == NULL ? NULL : OutData + MPC_FRAME_LENGTH),
                              (MPC_SAMPLE_FORMAT *)(d->V_R + MPC_V_MEM),
                              (MPC_SAMPLE_FORMAT *)(d->Y_R));
}

/*******************************************/
/*                                         */
/*            dithered synthesis           */
/*                                         */
/*******************************************/

static const unsigned char Parity [256] = {  // parity
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
