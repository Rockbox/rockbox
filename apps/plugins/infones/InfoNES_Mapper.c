/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Rockbox port of InfoNES
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

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_Mapper.h"
#include "K6502.h"

/*-------------------------------------------------------------------*/
/*  Mapper resources                                                 */
/*-------------------------------------------------------------------*/

/* Disk System RAM */
BYTE DRAM[ DRAM_SIZE ];

/*-------------------------------------------------------------------*/
/*  Table of Mapper initialize function                              */
/*-------------------------------------------------------------------*/

struct MapperTable_tag MapperTable[] =
{
  {   0, Map0_Init   },
  {   1, Map1_Init   },
  {   2, Map2_Init   },
  {   3, Map3_Init   },
  {   4, Map4_Init   },
  {   5, Map5_Init   },
  {   6, Map6_Init   },
  {   7, Map7_Init   },
  {   8, Map8_Init   },
  {   9, Map9_Init   },
  {  10, Map10_Init  },
  {  11, Map11_Init  },
  {  13, Map13_Init  },
  {  15, Map15_Init  },
  {  16, Map16_Init  },
  {  17, Map17_Init  },
  {  18, Map18_Init  },
  {  19, Map19_Init  },
  {  21, Map21_Init  },
  {  22, Map22_Init  },
  {  23, Map23_Init  },
  {  24, Map24_Init  },
  {  25, Map25_Init  },
  {  26, Map26_Init  },
  {  32, Map32_Init  },
  {  33, Map33_Init  },
  {  34, Map34_Init  },
  {  40, Map40_Init  },/*
  {  41, Map41_Init  },
  {  42, Map42_Init  },
  {  43, Map43_Init  },
  {  44, Map44_Init  },
  {  45, Map45_Init  },
  {  46, Map46_Init  },
  {  47, Map47_Init  },
  {  48, Map48_Init  },
  {  49, Map49_Init  },
  {  50, Map50_Init  },
  {  51, Map51_Init  },
  {  57, Map57_Init  },
  {  58, Map58_Init  },
  {  60, Map60_Init  },
  {  61, Map61_Init  },
  {  62, Map62_Init  },
  {  64, Map64_Init  },
  {  65, Map65_Init  },
  {  66, Map66_Init  },
  {  67, Map67_Init  },
  {  68, Map68_Init  },
  {  69, Map69_Init  },
  {  70, Map70_Init  },
  {  71, Map71_Init  },
  {  72, Map72_Init  },
  {  73, Map73_Init  },
  {  74, Map74_Init  },
  {  75, Map75_Init  },
  {  76, Map76_Init  },
  {  77, Map77_Init  },
  {  78, Map78_Init  },
  {  79, Map79_Init  },
  {  80, Map80_Init  },
  {  82, Map82_Init  },
  {  83, Map83_Init  },
  {  85, Map85_Init  },
  {  86, Map86_Init  },
  {  87, Map87_Init  },
  {  88, Map88_Init  },
  {  89, Map89_Init  },
  {  90, Map90_Init  },
  {  91, Map91_Init  },
  {  92, Map92_Init  },
  {  93, Map93_Init  },
  {  94, Map94_Init  },
  {  95, Map95_Init  },
  {  96, Map96_Init  },
  {  97, Map97_Init  },
  {  99, Map99_Init  },
  { 100, Map100_Init },
  { 101, Map101_Init },
  { 105, Map105_Init },
  { 107, Map107_Init },
  { 108, Map108_Init },
  { 109, Map109_Init },
  { 110, Map110_Init },
  { 112, Map112_Init },
  { 113, Map113_Init },
  { 114, Map114_Init },
  { 115, Map115_Init },
  { 116, Map116_Init },
  { 117, Map117_Init },
  { 118, Map118_Init },
  { 119, Map119_Init },
  { 122, Map122_Init },
  { 133, Map133_Init },
  { 134, Map134_Init },
  { 135, Map135_Init },
  { 140, Map140_Init },
  { 151, Map151_Init },
  { 160, Map160_Init },
  { 180, Map180_Init },
  { 181, Map181_Init },
  { 182, Map182_Init },
  { 183, Map183_Init },
  { 185, Map185_Init },
  { 187, Map187_Init },
  { 188, Map188_Init },
  { 189, Map189_Init },
  { 191, Map191_Init },
  { 193, Map193_Init },
  { 194, Map194_Init },
  { 200, Map200_Init },
  { 201, Map201_Init },
  { 202, Map202_Init },
  { 222, Map222_Init },
  { 225, Map225_Init },
  { 226, Map226_Init },
  { 227, Map227_Init },
  { 228, Map228_Init },
  { 229, Map229_Init },
  { 230, Map230_Init },
  { 231, Map231_Init },
  { 232, Map232_Init },
  { 233, Map233_Init },
  { 234, Map234_Init },
  { 235, Map235_Init },
  { 236, Map236_Init },
  { 240, Map240_Init },
  { 241, Map241_Init },
  { 242, Map242_Init },
  { 243, Map243_Init },
  { 244, Map244_Init },
  { 245, Map245_Init },
  { 246, Map246_Init },
  { 248, Map248_Init },
  { 249, Map249_Init },
  { 251, Map251_Init },
  { 252, Map252_Init },
  { 255, Map255_Init },*/
  { -1, NULL }
};

/*-------------------------------------------------------------------*/
/*  body of Mapper functions                                         */
/*-------------------------------------------------------------------*/

#include "mapper/InfoNES_Mapper_000.c"
#include "mapper/InfoNES_Mapper_001.c"
#include "mapper/InfoNES_Mapper_002.c"
#include "mapper/InfoNES_Mapper_003.c"
#include "mapper/InfoNES_Mapper_004.c"
#include "mapper/InfoNES_Mapper_005.c"
#include "mapper/InfoNES_Mapper_006.c"
#include "mapper/InfoNES_Mapper_007.c"
#include "mapper/InfoNES_Mapper_008.c"
#include "mapper/InfoNES_Mapper_009.c"
#include "mapper/InfoNES_Mapper_010.c"
#include "mapper/InfoNES_Mapper_011.c"
#include "mapper/InfoNES_Mapper_013.c"
#include "mapper/InfoNES_Mapper_015.c"
#include "mapper/InfoNES_Mapper_016.c"
#include "mapper/InfoNES_Mapper_017.c"
#include "mapper/InfoNES_Mapper_018.c"
#include "mapper/InfoNES_Mapper_019.c"
#include "mapper/InfoNES_Mapper_021.c"
#include "mapper/InfoNES_Mapper_022.c"
#include "mapper/InfoNES_Mapper_023.c"
#include "mapper/InfoNES_Mapper_024.c"
#include "mapper/InfoNES_Mapper_025.c"
#include "mapper/InfoNES_Mapper_026.c"
#include "mapper/InfoNES_Mapper_032.c"
#include "mapper/InfoNES_Mapper_033.c"
#include "mapper/InfoNES_Mapper_034.c"
#include "mapper/InfoNES_Mapper_040.c"/*
#include "mapper/InfoNES_Mapper_041.c"
#include "mapper/InfoNES_Mapper_042.c"
#include "mapper/InfoNES_Mapper_043.c"
#include "mapper/InfoNES_Mapper_044.c"
#include "mapper/InfoNES_Mapper_045.c"
#include "mapper/InfoNES_Mapper_046.c"
#include "mapper/InfoNES_Mapper_047.c"
#include "mapper/InfoNES_Mapper_048.c"
#include "mapper/InfoNES_Mapper_049.c"
#include "mapper/InfoNES_Mapper_050.c"
#include "mapper/InfoNES_Mapper_051.c"
#include "mapper/InfoNES_Mapper_057.c"
#include "mapper/InfoNES_Mapper_058.c"
#include "mapper/InfoNES_Mapper_060.c"
#include "mapper/InfoNES_Mapper_061.c"
#include "mapper/InfoNES_Mapper_062.c"
#include "mapper/InfoNES_Mapper_064.c"
#include "mapper/InfoNES_Mapper_065.c"
#include "mapper/InfoNES_Mapper_066.c"
#include "mapper/InfoNES_Mapper_067.c"
#include "mapper/InfoNES_Mapper_068.c"
#include "mapper/InfoNES_Mapper_069.c"
#include "mapper/InfoNES_Mapper_070.c"
#include "mapper/InfoNES_Mapper_071.c"
#include "mapper/InfoNES_Mapper_072.c"
#include "mapper/InfoNES_Mapper_073.c"
#include "mapper/InfoNES_Mapper_074.c"
#include "mapper/InfoNES_Mapper_075.c"
#include "mapper/InfoNES_Mapper_076.c"
#include "mapper/InfoNES_Mapper_077.c"
#include "mapper/InfoNES_Mapper_078.c"
#include "mapper/InfoNES_Mapper_079.c"
#include "mapper/InfoNES_Mapper_080.c"
#include "mapper/InfoNES_Mapper_082.c"
#include "mapper/InfoNES_Mapper_083.c"
#include "mapper/InfoNES_Mapper_085.c"
#include "mapper/InfoNES_Mapper_086.c"
#include "mapper/InfoNES_Mapper_087.c"
#include "mapper/InfoNES_Mapper_088.c"
#include "mapper/InfoNES_Mapper_089.c"
#include "mapper/InfoNES_Mapper_090.c"
#include "mapper/InfoNES_Mapper_091.c"
#include "mapper/InfoNES_Mapper_092.c"
#include "mapper/InfoNES_Mapper_093.c"
#include "mapper/InfoNES_Mapper_094.c"
#include "mapper/InfoNES_Mapper_095.c"
#include "mapper/InfoNES_Mapper_096.c"
#include "mapper/InfoNES_Mapper_097.c"
#include "mapper/InfoNES_Mapper_099.c"
#include "mapper/InfoNES_Mapper_100.c"
#include "mapper/InfoNES_Mapper_101.c"
#include "mapper/InfoNES_Mapper_105.c"
#include "mapper/InfoNES_Mapper_107.c"
#include "mapper/InfoNES_Mapper_108.c"
#include "mapper/InfoNES_Mapper_109.c"
#include "mapper/InfoNES_Mapper_110.c"
#include "mapper/InfoNES_Mapper_112.c"
#include "mapper/InfoNES_Mapper_113.c"
#include "mapper/InfoNES_Mapper_114.c"
#include "mapper/InfoNES_Mapper_115.c"
#include "mapper/InfoNES_Mapper_116.c"
#include "mapper/InfoNES_Mapper_117.c"
#include "mapper/InfoNES_Mapper_118.c"
#include "mapper/InfoNES_Mapper_119.c"
#include "mapper/InfoNES_Mapper_122.c"
#include "mapper/InfoNES_Mapper_133.c"
#include "mapper/InfoNES_Mapper_134.c"
#include "mapper/InfoNES_Mapper_135.c"
#include "mapper/InfoNES_Mapper_140.c"
#include "mapper/InfoNES_Mapper_151.c"
#include "mapper/InfoNES_Mapper_160.c"
#include "mapper/InfoNES_Mapper_180.c"
#include "mapper/InfoNES_Mapper_181.c"
#include "mapper/InfoNES_Mapper_182.c"
#include "mapper/InfoNES_Mapper_183.c"
#include "mapper/InfoNES_Mapper_185.c"
#include "mapper/InfoNES_Mapper_187.c"
#include "mapper/InfoNES_Mapper_188.c"
#include "mapper/InfoNES_Mapper_189.c"
#include "mapper/InfoNES_Mapper_191.c"
#include "mapper/InfoNES_Mapper_193.c"
#include "mapper/InfoNES_Mapper_194.c"
#include "mapper/InfoNES_Mapper_200.c"
#include "mapper/InfoNES_Mapper_201.c"
#include "mapper/InfoNES_Mapper_202.c"
#include "mapper/InfoNES_Mapper_222.c"
#include "mapper/InfoNES_Mapper_225.c"
#include "mapper/InfoNES_Mapper_226.c"
#include "mapper/InfoNES_Mapper_227.c"
#include "mapper/InfoNES_Mapper_228.c"
#include "mapper/InfoNES_Mapper_229.c"
#include "mapper/InfoNES_Mapper_230.c"
#include "mapper/InfoNES_Mapper_231.c"
#include "mapper/InfoNES_Mapper_232.c"
#include "mapper/InfoNES_Mapper_233.c"
#include "mapper/InfoNES_Mapper_234.c"
#include "mapper/InfoNES_Mapper_235.c"
#include "mapper/InfoNES_Mapper_236.c"
#include "mapper/InfoNES_Mapper_240.c"
#include "mapper/InfoNES_Mapper_241.c"
#include "mapper/InfoNES_Mapper_242.c"
#include "mapper/InfoNES_Mapper_243.c"
#include "mapper/InfoNES_Mapper_244.c"
#include "mapper/InfoNES_Mapper_245.c"
#include "mapper/InfoNES_Mapper_246.c"
#include "mapper/InfoNES_Mapper_248.c"
#include "mapper/InfoNES_Mapper_249.c"
#include "mapper/InfoNES_Mapper_251.c"
#include "mapper/InfoNES_Mapper_252.c"
#include "mapper/InfoNES_Mapper_255.c"*/

/* End of InfoNES_Mapper.c */
