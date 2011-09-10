/*
  Copyright (c) 2005-2009, The Musepack Development Team
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
/// \file requant.c
/// Requantization function implementations.
/// \todo document me
#include "mpcdec.h"
#include "requant.h"
#include "mpcdec_math.h"
#include "decoder.h"
#include "internal.h"
#include <string.h>

/* C O N S T A N T S */
// Bits per sample for chosen quantizer
const mpc_uint8_t  Res_bit [18] ICONST_ATTR = {
    0,  0,  0,  0,  0,  0,  0,  0,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16
};

// Requantization coefficients
// 65536/step bzw. 65536/(2*D+1)

#define _(X) MAKE_MPC_SAMPLE_EX(X,14)

const MPC_SAMPLE_FORMAT  __Cc [1 + 18] ICONST_ATTR = {
    _(111.285962475327f),   // 32768/2/255*sqrt(3)
    _(65536.000000000000f), _(21845.333333333332f), _(13107.200000000001f), _(9362.285714285713f),
    _(7281.777777777777f),  _(4369.066666666666f),  _(2114.064516129032f),  _(1040.253968253968f),
    _(516.031496062992f),   _(257.003921568627f),   _(128.250489236790f),   _(64.062561094819f),
    _(32.015632633121f),    _(16.003907203907f),    _(8.000976681723f),     _(4.000244155527f),
    _(2.000061037018f),     _(1.000015259021f)
};

#undef _

// Requantization offset
// 2*D+1 = steps of quantizer
const mpc_int16_t  __Dc [1 + 18] ICONST_ATTR = {
      2,
      0,     1,     2,     3,     4,     7,    15,    31,    63,
    127,   255,   511,  1023,  2047,  4095,  8191, 16383, 32767
};

// Table'ized SCF calculated from mpc_decoder_scale_output(d, 1.0)
static const mpc_uint32_t SCF[] = {
    1289035711, 1073741824, 1788812356, 1490046106, 1241179595, 1033878604,  861200887,  717363687,
     597550081,  497747664,  414614180,  345365595,  287682863,  239634262,  199610707,  166271859,
     138501244,  115368858,   96100028,   80049465,   66679657,   55542865,   46266132,   38538793,
      32102070,   26740403,   22274239,   18554010,   15455132,   12873826,   10723648,    8932591,
       7440676,    6197939,    5162763,    4300482,    3582218,    2983918,    2485546,    2070412,
       1724613,    1436569,    1196634,     996773,     830293,     691618,     576104,     479883,
        399734,     332970,     277358,     231034,     192446,     160304,     133530,     111228,
         92651,      77176,      64286,      53549,      44605,      37155,      30949,      25780,
         21474,      17888,      14900,      12411,      10338,       8612,       7173,       5975,
          4977,       4146,       3453,       2876,       2396,       1996,       1662,       1385,
          1153,        961,        800,        666,        555,        462,        385,        321,
           267,        222,        185,        154,        128,        107,         89,         74,
            61,         51,         43,         35,         29,         24,         20,         17,
            14,         11,          9,          8,          6,          5,          4,          3,
             3,          2,          2,          1,          1,          1,          1,          0,
             0,          0,          0,          0,          0,          0,          0,          0,
             0, 2147483648, 2147483648, 2147483648, 2147483648, 2147483648, 2147483648, 2147483648,
    2147483648, 2147483648, 2147483648, 2147483648, 1930697728, 1608233877, 1339627724, 1115883992,
    1859019579, 1548527365, 1289893354, 1074456223, 1790002518, 1491037488, 1242005398, 2069132964,
    1723547752, 1435681952, 1195895306, 1992315335, 1659560152, 1382381519, 1151497076, 1918349601,
    1597948125, 1331059892, 1108747153, 1847129882, 1538623477, 1281643607, 2135168687, 1778554232,
    1481501287, 1234061927, 2055899448, 1712524489, 1426499787, 1188246741, 1979573121, 1648946134,
    1373540247, 1144132468, 1906080447, 1587728158, 1322546856, 1101655960, 1835316227, 1528782931,
    1273446622, 2121512828, 1767179166, 1472026076, 1226169259, 2042750570, 1701571728, 1417376349,
    1180647093, 1966912401, 1638400000, 1364755521, 1136814961, 1893889764, 1577573554, 1314088268,
    1094610119, 1823578129, 1519005322, 1265302063, 2107944308, 1755876851, 1462611466, 1218327071,
    2029685788, 1690689017, 1408311261, 1173096050, 1954332656, 1627921315, 1356026979, 1129544254,
    1881777048, 1567483896, 1305683778, 1087609341, 1811915104, 1509290248, 1257209594, 2094462567,
    1744646821, 1453257069, 1210535039, 2016704564, 1679875908, 1399304151, 1165593302, 1941833367,
    1617509648, 1347354262, 1122320049, 1869741801, 1557458768, 1297333040, 1080653338, 1800326672,
    1499637308, 1249168882, 2081067051, 1733488616, 1443962500, 1202792843, 2003806364, 1669131957,
    1390354647, 1158138538, 1929414019, 1607164572, 1338737013, 1115142047, 1857783528, 1547497758
};

// Table'ized SCF_shift calculated from mpc_decoder_scale_output(d, 1.0)
static const mpc_uint8_t SCF_shift[] = {
    30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
    31,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     1,  1,  1,  1,  2,  2,  2,  3,  3,  3,  3,  4,  4,  4,  4,  5,
     5,  5,  5,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,
     9,  9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13,
    13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17, 17,
    18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 22,
    22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26,
    26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30
};

/* F U N C T I O N S */
/* not used anymore, tables from above are used
#ifdef MPC_FIXED_POINT
static mpc_uint32_t find_shift(double fval)
{
    mpc_int64_t  val = (mpc_int64_t) fval;
    mpc_uint32_t ptr = 0;
    if(val<0)
        val = -val;
    while(val)
    {
        val >>= 1;
        ptr++;
    }
    return ptr > 31 ? 0 : 31 - ptr;
}
#endif

#define SET_SCF(N,X) d->SCF[N] = MAKE_MPC_SAMPLE_EX(X,d->SCF_shift[N] = (mpc_uint8_t) find_shift(X));

static void
mpc_decoder_scale_output(mpc_decoder *d, double factor)
{
    mpc_int32_t n; double f1, f2;

#ifndef MPC_FIXED_POINT
    factor *= 1.0 / (double) (1<<(MPC_FIXED_POINT_SHIFT-1));
#else
    factor *= 1.0 / (double) (1<<(16-MPC_FIXED_POINT_SHIFT));
#endif
    f1 = f2 = factor;

    // handles +1.58...-98.41 dB, where's scf[n] / scf[n-1] = 1.20050805774840750476

    SET_SCF(1,factor);

    f1 *=   0.83298066476582673961;
    f2 *= 1/0.83298066476582673961;

    for ( n = 1; n <= 128; n++ ) {
        SET_SCF((mpc_uint8_t)(1+n),f1);
        SET_SCF((mpc_uint8_t)(1-n),f2);
        f1 *=   0.83298066476582673961;
        f2 *= 1/0.83298066476582673961;
    }
}
*/
void
mpc_decoder_init_quant(mpc_decoder *d, MPC_SAMPLE_FORMAT factor)
{
    //mpc_decoder_scale_output(d, (double)factor / MPC_FIXED_POINT_SHIFT)
    (void)factor;
    memcpy(d->SCF, SCF, sizeof(d->SCF));
    memcpy(d->SCF_shift, SCF_shift, sizeof(d->SCF_shift));
}
